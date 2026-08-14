#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

#include <corecpp/io/domain.h>
#include <corecpp/io/event.h>
#include <corecpp/io/socket.h>
#include <corecpp/unittest.h>

using namespace corecpp;

namespace
{

constexpr size_t packet_size = 8192;
constexpr const char* socket_path = "/tmp/corecpp_test_socket.sock";

constexpr size_t datagram_size = 1024;
constexpr const char* server_dgram_path = "/tmp/corecpp_test_dgram_server.sock";
constexpr const char* client_dgram_path = "/tmp/corecpp_test_dgram_client.sock";

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
		ssize_t received = sock.recv(buffer.subspan(total), msgflags::none);
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

class test_socket : public test_fixture
{
	test_case_result test_echo() const
	{
		::unlink(socket_path);

		std::vector<std::byte> sent(packet_size);
		for (size_t i = 0; i < sent.size(); ++i)
			sent[i] = static_cast<std::byte>(i * 131 + 17);

		std::atomic<bool> server_ready { false };
		std::string server_error;
		bool server_ok = false;

		std::thread server { [&] {
			try
			{
				ioloop loop;
				async_socket_acceptor<family::local> acceptor { loop, 0 };
				acceptor.bind(make_address(socket_path));
				acceptor.listen(16);

				/* exit the loop if nothing happens, so the test never hangs */
				timer_event watchdog { loop, [&loop] (uint64_t) { loop.exit(); } };
				watchdog.expires_in(std::chrono::seconds(5));

				acceptor.async_accept([&loop] (streamsocket<family::local> conn) {
					std::vector<std::byte> received(packet_size);
					if (recv_all(conn, received) == static_cast<ssize_t>(packet_size))
						send_all(conn, received);
					loop.exit();
				});

				server_ready.store(true);
				loop.run();
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

		std::vector<std::byte> received(packet_size);
		bool echoed = false;
		if (server_error.empty()) [[likely]]
		{
			try
			{
				streamsocket<family::local> client { 0 };
				if ( client.connect(make_address(socket_path)) == 0 &&
				     send_all(client, sent) == static_cast<ssize_t>(packet_size) &&
				     recv_all(client, received) == static_cast<ssize_t>(packet_size) )
					echoed = (received == sent);
			}
			catch (const std::exception&)
			{
			}
		}

		server.join();
		::unlink(socket_path);

		assert_equal(server_ok, true);
		assert_equal(echoed, true);

		return {};
	}

	test_case_result test_echo_datagram() const
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
				ioloop loop;
				async_datagramsocket<family::local> dgram { loop, 0 };
				dgram.bind(make_address(server_dgram_path));

				std::vector<std::byte> received(datagram_size);

				/* exit the loop if nothing happens, so the test never hangs */
				timer_event watchdog { loop, [&loop] (uint64_t) { loop.exit(); } };
				watchdog.expires_in(std::chrono::seconds(5));

				dgram.async_recvfrom(std::span<std::byte> { received },
					[&loop, &dgram, &received] (ssize_t n, const family::local::address_type& src) {
						if (n > 0)
							dgram.sendto(std::span<std::byte> { received }.first(static_cast<size_t>(n)), msgflags::none, src);
						loop.exit();
					});

				server_ready.store(true);
				loop.run();
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
			ioloop loop;
			async_datagramsocket<family::local> dgram { loop, 0 };
			dgram.bind(make_address(client_dgram_path));

			timer_event watchdog { loop, [&loop] (uint64_t) { loop.exit(); } };
			watchdog.expires_in(std::chrono::seconds(5));

			dgram.async_recvfrom(std::span<std::byte> { received },
				[&] (ssize_t n, const family::local::address_type&) {
					echoed = (n == static_cast<ssize_t>(datagram_size) && received == sent);
					loop.exit();
				});

			dgram.sendto(std::span<const std::byte> { sent }, msgflags::none,
			             make_address(server_dgram_path));

			loop.run();
		}

		server.join();
		::unlink(server_dgram_path);
		::unlink(client_dgram_path);

		assert_equal(server_ok, true);
		assert_equal(echoed, true);

		return {};
	}

public:
	tests_type tests() const override
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
