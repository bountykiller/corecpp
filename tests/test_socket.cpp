#include <atomic>
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


constexpr size_t datagram_size = 1024;
constexpr const char* server_dgram_path = "/tmp/corecpp_test_dgram_server.sock";
constexpr const char* client_dgram_path = "/tmp/corecpp_test_dgram_client.sock";

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
		conn.async_recv(
			0,
			[this] (socket_type& conn, std::span<const std::byte> buffer)
			{
				return on_recv(conn, buffer);
			});
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
		conn.async_send(buffer,
						[](socket_type&, int err)
						{
							ensure_equal(0, err);
						});
		return buffer.size();
	}

	ioloop& m_loop;
	async_socketacceptor<family::local> m_acceptor;
	std::set<socket_type> m_conns;
};

class test_socket : public test_fixture
{

	struct test
	{
		size_t packet_size;
	};

	test_case_result test_echo()
	{
		static const test_cases<test> cases
		{
			{ 1 },  { 4 }, { 1024 }, { 8192 }, { 32657 }
		};

		::unlink(socket_path);
		echo_stream_server server { m_loop };
		server.start(m_socket_addr);

		return run(
			cases,
			[this] (const test &t)
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
								client_connected(client, t);
							});
					});

				m_loop.run_timeout(
					std::chrono::seconds(5),
					[this](void)
					{
						assert_equal("Timeout reached!"sv, "packet should have been received before"sv);
					}
				);
			});
	}

	void client_connected(async_streamsocket<family::local> &client, const test &t)
	{
		logger().trace("client connected, will send data");
		std::vector<std::byte> packet(t.packet_size);
		for (size_t i = 0; i < packet.size(); ++i)
			packet[i] = static_cast<std::byte>(i * 131 + 17); //TODO: use randon datas

		client.async_send(
			packet,
			[this, packet](async_streamsocket<family::local>& client, int err)
			{
				fatal_equal(err, 0);
				packet_sent(client, packet);
			});
	}

	void packet_sent(async_streamsocket<family::local>& client, std::vector<std::byte> sent_packet)
	{
		logger().trace("packet sent");
		client.async_recv(
			sent_packet.size(),
			[this, sent_packet](async_streamsocket<family::local>&, std::span<const std::byte> received)
			{
				return packet_received(sent_packet, received);
			}
		);
	}

	size_t packet_received(std::span<const std::byte> sent, std::span<const std::byte> received)
	{
		logger().trace("packet received");
		fatal_equal(received.size(), sent.size());
		for (size_t i = 0; i < sent.size(); ++i)
			assert_equal(sent[i], received[i]);
		m_loop.exit();
		return received.size();
	}

	test_case_result test_echo_datagram()
	{
		::unlink(server_dgram_path);
		::unlink(client_dgram_path);

		std::vector<std::byte> sent(datagram_size);
		for (size_t i = 0; i < sent.size(); ++i)
			sent[i] = static_cast<std::byte>(i * 97 + 5);

		std::atomic<bool> server_ready { false };
		std::string server_error;
		bool server_ok = false;

		std::thread server { [&] {
			try
			{
				async_datagramsocket<family::local> dgram { m_loop, 0 };
				dgram.bind(make_address(server_dgram_path));

				std::vector<std::byte> received(datagram_size);

				/* exit the loop if nothing happens, so the test never hangs */
				dgram.async_recvfrom(
					std::span<std::byte> { received },
					[this, &dgram, &received] (ssize_t n, const family::local::address_type& src)
					{
						if (n > 0)
							dgram.sendto(std::span<std::byte> { received }.first(static_cast<size_t>(n)), msgflags::none, src);
						m_loop.exit();
					});

				server_ready.store(true);
				m_loop.run_timeout(
					std::chrono::seconds(5),
					[this](void)
					{
						assert_equal("Timeout reached!"sv, "packet should be received before"sv);
					}
				);
				server_ok = true;
			}
			catch (const std::exception& e)
			{
				server_error = e.what();
				server_ready.store(true);
			}
		} };

		while (!server_ready.load())
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

		std::vector<std::byte> received(datagram_size);
		bool echoed = false;
		if (server_error.empty()) [[likely]]
		{
			async_datagramsocket<family::local> dgram { m_loop, 0 };
			dgram.bind(make_address(client_dgram_path));

			dgram.async_recvfrom(std::span<std::byte> { received },
				[&] (ssize_t n, const family::local::address_type&) {
					echoed = (n == static_cast<ssize_t>(datagram_size) && received == sent);
					m_loop.exit();
				});

			dgram.sendto(std::span<const std::byte> { sent }, msgflags::none,
			             make_address(server_dgram_path));
			m_loop.run_timeout(
				std::chrono::seconds(5),
				[this](void)
				{
					assert_equal("Timeout reached!"sv, "packet should be received before"sv);
				}
			);
		}

		server.join();
		::unlink(server_dgram_path);
		::unlink(client_dgram_path);

		assert_equal(server_ok, true);
		assert_equal(echoed, true);

		return {};
	}


	static constexpr const char* socket_path = "/tmp/corecpp_test_socket.sock";

	ioloop m_loop;
	const family::local::address_type m_socket_addr = make_address(socket_path);

public:
	tests_type tests() override
	{
		return {
			{ "test_echo", [&] () { return test_echo(); } },
			{ "test_echo_datagram", [&] () { return test_echo_datagram(); } },
		};
	}
};

int main(int argc, char** argv)
{
	test_unit unit { "Socket" };
	unit.add_fixture<test_socket>("test_socket");

	return unit.run(argc, argv);
}
