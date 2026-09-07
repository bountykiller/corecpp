#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <set>
#include <string_view>
#include <thread>

#include <corecpp/io/domain.h>
#include <corecpp/io/event.h>
#include <corecpp/io/socket.h>
#include <corecpp/unittest.h>

using namespace corecpp;
using namespace std::string_view_literals;

namespace
{

static corecpp::diagnostic::event_producer& test_logger()
{
	static auto& channel = corecpp::diagnostic::manager::get_channel("test_socket");
	static auto logger = corecpp::diagnostic::event_producer(channel);
	return logger;
}

ssize_t send_all(streamsocket<family::local>& sock, const std::span<const std::byte> buffer)
{
	size_t total = 0;
	while (total < buffer.size())
	{
		ssize_t sent = sock.send(buffer.subspan(total), msgflags::none);
		if (sent <= 0) [[unlikely]]
			return -1;
		total += sent;
	}
	return static_cast<ssize_t>(total);
}

ssize_t recv_all(streamsocket<family::local>& sock, const std::span<std::byte> buffer)
{
	size_t total = 0;
	while (total < buffer.size())
	{
		ssize_t received = sock.receive(buffer.subspan(total), msgflags::none);
		if (received <= 0) [[unlikely]]
			return -1;
		total += received;
	}
	return static_cast<ssize_t>(total);
}

family::local::address_type make_address(const char* path)
{
	family::local::address_type addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	std::strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
	return addr;
}

}

class echo_stream_server
{
public:

	using socket_type = async_streamsocket<family::local>;

	echo_stream_server(ioloop& loop) :
		m_loop { loop },
		m_acceptor { loop, 0 },
		m_conns {}
	{
	}

	void start(const sockaddr_un &sock_file)
	{
		m_acceptor.bind(sock_file);
		m_acceptor.listen(16); /* arbitrary value */
		m_acceptor.async_accept(
			[this] (socket_type &&conn)
			{
				auto inserted = m_conns.emplace(std::move(conn));
				//HACK: casting here because the iterator gives us a CONST reference, but why?
				echo_loop(const_cast<socket_type&>(*inserted.first));
			});
	}

private:

	void echo_loop(socket_type &conn)
	{
		conn.on_async_recv(
			[this] (socket_type& conn, std::span<const std::byte> buffer)
			{
				return on_recv(conn, buffer);
			});
		conn.on_async_sent(
			[](socket_type&, int err)
			{
				ensure_equal(0, err);
			});

		conn.async_recv(0, false);
	}

	size_t on_recv(socket_type& conn, std::span<const std::byte> buffer)
	{
		if (buffer.empty())
		{
			test_logger().trace("client disconnected");
			m_loop.post(
				[this, &conn]()
				{
					m_conns.erase(conn);
				});
			return 0;
		}

		test_logger().debug(std::format("received {} bytes, will reemit back", buffer.size()));
		conn.async_send(buffer);
		return buffer.size();
	}

	ioloop& m_loop;
	async_socketacceptor<family::local> m_acceptor;
	std::set<socket_type> m_conns;
};

class echo_dgram_server
{

public:
	using socket_type = async_datagramsocket<family::local>;

private:
	ioloop& m_loop;
	socket_type m_socket;

public:
	explicit echo_dgram_server(ioloop& loop) :
		m_loop { loop },
		m_socket { loop, 0 }
	{
	}

	void start(const sockaddr_un &sock_file)
	{
		m_socket.bind(sock_file);
		/* m_socket.listen(16); /* arbitrary value */
		m_socket.on_async_recvfrom(
			[this] (socket_type&, const family::local::address_type& src, std::span<const std::byte> buffer)
			{
				on_recv(src, buffer);
			});
		/*
		m_socket.on_async_sent(
			[](socket_type&, int err)
			{
				ensure_equal(0, err);
			});
		*/
	}

private:

	size_t on_recv(const family::local::address_type& src, std::span<const std::byte> buffer)
	{
		test_logger().debug(std::format("received dgram of {} bytes, will reemit back", buffer.size()));
		m_socket.async_sendto(buffer, src);
		return buffer.size();
	}
};


class test_stream_socket : public test_fixture
{
	struct echo_test
	{
		size_t packet_size;
	};

	static constexpr const char* socket_path = "/tmp/corecpp_test_socket.sock";

	ioloop m_loop;
	const family::local::address_type m_socket_addr = make_address(socket_path);
	std::vector<std::byte> m_packet = {};

	test_case_result test_echo()
	{
		static const test_cases<echo_test> cases
		{
			{ 1 },  { 4 }, { 1024 }, { 8192 }, { 32657 }
		};

		::unlink(socket_path);
		echo_stream_server server { m_loop };
		server.start(m_socket_addr);

		return run(
			cases,
			[this] (const echo_test &t)
			{
				run_echo_test(t);
			});
	}

	void run_echo_test(const echo_test &t)
	{
		logger().info(std::format("Test {} bytes", t.packet_size));

		async_streamsocket<family::local> client { m_loop, 0 };
		m_loop.post(
			[this, &client, &t]()
			{
				client.async_connect(
					m_socket_addr,
					[this, &t](async_streamsocket<family::local>& client, int)
					{
						echo_client_connected(client, t);
					});
			});

		m_loop.run_timeout(
			std::chrono::seconds(5),
			[this](void)
			{
				assert_equal("Timeout reached!"sv, "packet should have been received before"sv);
			}
		);
	}

	void echo_client_connected(async_streamsocket<family::local> &client, const echo_test &t)
	{
		logger().trace("client connected, will send data");
		m_packet.resize(t.packet_size);
		for (size_t i = 0; i < m_packet.size(); ++i)
			m_packet[i] = static_cast<std::byte>(i * 131 + 17); //TODO: use randon datas

		client.on_async_sent(
			[this](async_streamsocket<family::local>& client, int err)
			{
				logger().trace("packet sent");
				fatal_equal(err, 0);
				client.async_recv(m_packet.size(), true);
			});

		client.on_async_recv(
			[this](async_streamsocket<family::local>&, std::span<const std::byte> received)
			{
				return echo_packet_received(received);
			});

		client.async_send(m_packet);
	}

	size_t echo_packet_received(std::span<const std::byte> received)
	{
		logger().trace("packet received");
		fatal_equal(received.size(), m_packet.size());
		for (size_t i = 0; i < m_packet.size(); ++i)
			assert_equal(m_packet[i], received[i]);
		m_loop.exit();
		return received.size();
	}

public:
	tests_type tests() override
	{
		return {
			{ "echo", [&] () { return test_echo(); } },
		};
	}
};

/* TODO: check if I do not need tu use local ip adresses */
class test_datagram_socket : public test_fixture
{
	struct echo_test
	{
		size_t dgram_size;
	};

	static constexpr const char* dgram_path = "/tmp/corecpp_test_dgram_server.sock";
	static constexpr const char* client_path = "/tmp/corecpp_test_dgram_client.sock";

	ioloop m_loop;
	const family::local::address_type m_socket_addr = make_address(dgram_path);
	std::vector<std::byte> m_packet = {};

	test_case_result test_echo()
	{
		static const test_cases<echo_test> cases
		{
			{ 1 },  { 4 }, { 1024 }, { 8192 }, { 32657 }
		};

		::unlink(dgram_path);
		echo_dgram_server server { m_loop };
		server.start(m_socket_addr);

		return run(
			cases,
			 [this] (const echo_test &t)
			 {
				 run_echo_test(t);
			 });
	}

	void run_echo_test(const echo_test &t)
	{
		logger().info(std::format("Test {} bytes", t.dgram_size));

		m_packet.resize(t.dgram_size);
		for (size_t i = 0; i < m_packet.size(); ++i)
			m_packet[i] = static_cast<std::byte>(i * 97 + 5);

		async_datagramsocket<family::local> client { m_loop, 0 };
		::unlink(client_path);
		client.bind(make_address(client_path));
		/*
		client.on_async_sent(
			[this](async_datagramsocket<family::local>& client, int err)
			{
				logger().trace("dgram sent");
				fatal_equal(err, 0);
				client.async_recvfrom(true);
			});
		*/
		client.on_async_recvfrom(
			[this](async_datagramsocket<family::local>&, const family::local::address_type&, std::span<const std::byte> received)
			{
				return echo_packet_received(received);
			});
		m_loop.post(
			[this, &client]()
			{
				client.async_sendto(m_packet, m_socket_addr);
			});

		m_loop.run_timeout(
			std::chrono::seconds(5),
			[this](void)
			{
				assert_equal("Timeout reached!"sv, "packet should have been received before"sv);
			}
		);
	}

	size_t echo_packet_received(std::span<const std::byte> received)
	{
		logger().trace("dgram received");
		fatal_equal(received.size(), m_packet.size());
		for (size_t i = 0; i < m_packet.size(); ++i)
			assert_equal(m_packet[i], received[i]);
		m_loop.exit();
		return received.size();
	}

public:
	tests_type tests() override
	{
		return {
			{ "echo", [&] () { return test_echo(); } },
		};
	}
};

int main(int argc, char** argv)
{
	test_unit unit { "Socket" };
	unit.add_fixture<test_stream_socket>("test_stream_socket");
	unit.add_fixture<test_datagram_socket>("test_datagram_socket");

	return unit.run(argc, argv);
}
