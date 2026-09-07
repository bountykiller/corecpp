#ifndef CORE_CPP_SOCKET_H
#define CORE_CPP_SOCKET_H

#include <cerrno>
#include <cstddef>
#include <span>
#include <system_error>

#include <sys/socket.h>

#include <algorithm>

#include <corecpp/except.h>
#include <corecpp/file.h>
#include <corecpp/flags.h>
#include <corecpp/meta/reflection.h>
#include <corecpp/io/domain.h>

namespace corecpp
{

/*
enum class socktype : int16_t
{
	Stream    = SOCK_STREAM,
	Datagram  = SOCK_DGRAM,
	Raw       = SOCK_RAW,
	SeqPacket = SOCK_SEQPACKET
};
*/

static inline socklen_t address_len(const sockaddr_un& addr) noexcept
{
	return SUN_LEN(&addr);
}
static inline std::string to_string(const sockaddr_un& addr) noexcept
{
	return addr.sun_path;
}

static inline constexpr socklen_t address_len(const sockaddr_in& addr) noexcept
{
	return sizeof(addr);
}

static inline constexpr socklen_t address_len(const sockaddr_in6& addr) noexcept
{
	return sizeof(addr);
}

static inline constexpr socklen_t address_len(const sockaddr_nl& addr) noexcept
{
	return sizeof(addr);
}

template <int LevelT, int OptionT, typename ValueT>
struct socket_option
{
	using value_type = ValueT;
	static constexpr int level  = LevelT;
	static constexpr int option = OptionT;
	ValueT value;
};

using so_passcred = socket_option<SOL_SOCKET, SO_PASSCRED, int>;

template<typename T>
concept SocketOption = requires (T opt)
{
	(std::is_same_v<int, typeof(T::level)>);
	(std::is_same_v<int, typeof(T::option)>);
	opt.value;
};

class socket : public filedesc
{
public:
	socket(int domain, int type, int protocol)
	: filedesc { ::socket(domain, type, protocol) }
	{
		if (m_fd < 0)
			corecpp::throw_system_error(errno, std::system_category());
	}
	socket(const socket& other) = delete;
	socket& operator=(const socket &) = delete;

protected:
	socket(filedesc&& fd) noexcept
	: filedesc(std::move(fd))
	{
	}

	socket(socket&& other) noexcept
	: filedesc(std::move(other))
	{}

	socket& operator=(socket &&other) noexcept
	{
		return static_cast<socket&>(filedesc::operator=(std::move(other)));
	}

	template<SocketOption OptionT>
	void set_option(const OptionT& opt)
	{
		if (setsockopt(m_fd, OptionT::level, OptionT::option, &opt.value, sizeof(opt.value))) [[unlikely]]
			corecpp::throw_system_error(errno, std::system_category());
	}
};


template <typename DomainT>
class socket_acceptor;

template <typename DomainT>
class streamsocket : public socket
{
friend class socket_acceptor<DomainT>;
public:
	streamsocket(int protocol)
	: socket(DomainT::domain, SOCK_STREAM, protocol)
	{}
	streamsocket(const streamsocket& other) = delete;
	streamsocket& operator=(const streamsocket &) = delete;

	streamsocket(streamsocket&& other) noexcept
	: socket(std::move(other))
	{}

	streamsocket& operator=(streamsocket &&other) noexcept
	{
		return static_cast<streamsocket&>(socket::operator=(std::move(other)));
	}

	ssize_t connect(const DomainT::address_type &dest)
	{
		return ::connect(m_fd, reinterpret_cast<const struct sockaddr*>(&dest), address_len(dest));
	}

	ssize_t send(const std::span<const std::byte> buffer, const flags<msgflags> flags)
	{
		if (buffer.empty()) [[unlikely]]
			return 0;
		return ::send(m_fd, buffer.data(), buffer.size(), flags.get());
	}

	ssize_t send(const std::span<const std::byte> buffer, const flags<msgflags> flags, std::chrono::milliseconds timeout)
	{
		if (buffer.empty()) [[unlikely]]
			return 0;

		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(m_fd, &writefds);

		timeval tv;
		tv.tv_sec = timeout.count() / 1000;
		tv.tv_usec = (timeout.count() % 1000) * 1000;

		int result = select(m_fd + 1, nullptr, &writefds, nullptr, &tv);
		if (result == -1)
			return -1;

		if (result == 0)
		{
			errno = ETIMEDOUT;
			return -1;
		}

		return ::send(m_fd, buffer.data(), buffer.size(), flags.get());
	}

	ssize_t receive(std::span<std::byte> buffer, const flags<msgflags> flags)
	{
		return ::recv(m_fd, buffer.data(), buffer.size(), flags.get());
	}

protected:

	streamsocket(filedesc&& fd) noexcept
	: socket(std::move(fd))
	{}
};



template <typename DomainT>
class socket_acceptor : public socket
{
public:
	socket_acceptor(int protocol)
	: socket(DomainT::domain, SOCK_STREAM, protocol)
	{}
	socket_acceptor(const socket_acceptor& other) = delete;
	socket_acceptor& operator=(const socket_acceptor &) = delete;

	void bind(const DomainT::address_type &addr) const
	{
		int ret = ::bind(m_fd, reinterpret_cast<const struct sockaddr*>(&addr), address_len(addr));
		if ( ret )
			corecpp::throw_system_error(errno, std::system_category());
	}

	void listen(int backlog = SOMAXCONN) const
	{
		int ret = ::listen(m_fd, backlog);
		if ( ret )
			corecpp::throw_system_error(errno, std::system_category());
	}

	streamsocket<DomainT> accept(DomainT::address_type* peer_addr = nullptr)
	{
		socklen_t addr_len = sizeof(typename DomainT::address_type);
		socklen_t* len_ptr = peer_addr == nullptr ? nullptr : &addr_len;

		int fd = ::accept(m_fd, reinterpret_cast<struct sockaddr*>(peer_addr), len_ptr);
		if (fd < 0) [[unlikely]]
			corecpp::throw_system_error(errno, std::system_category());

		return streamsocket<DomainT> { filedesc { fd } };
	}

protected:
	socket_acceptor(socket_acceptor&& other) noexcept
	: socket(std::move(other))
	{}

	socket_acceptor& operator=(socket_acceptor &&other) noexcept
	{
		return static_cast<socket_acceptor&>(socket::operator=(std::move(other)));
	}
};



template <typename DomainT>
class datagramsocket : public socket
{
public:
	datagramsocket(int protocol)
	: socket(DomainT::domain, SOCK_DGRAM, protocol)
	{}
	datagramsocket(const datagramsocket& other) = delete;
	datagramsocket& operator=(const datagramsocket &) = delete;

	void bind(const DomainT::address_type &addr)
	{
		int ret = ::bind(m_fd, reinterpret_cast<const struct sockaddr*>(&addr), address_len(addr));
		if ( ret )
			corecpp::throw_system_error(errno, std::system_category());
		/*
		if constexpr ( std::is_same_v<DomainT, corecpp::family::local> )
		{
			//get credentials
			so_passcred opt { 1 };
			set_option<so_passcred>(opt);
		}
		*/
	}

	ssize_t sendto(const std::span<const std::byte> buffer, const flags<msgflags> flags, const DomainT::address_type &dest)
	{
		if (buffer.empty()) [[unlikely]]
			return 0;

		return ::sendto(m_fd, buffer.data(), buffer.size(), flags.get(),
		                reinterpret_cast<const struct sockaddr*>(&dest), address_len(dest));
	}

	ssize_t sendto(const std::span<const std::byte> buffer, const flags<msgflags> flags, const DomainT::address_type &dest, std::chrono::milliseconds timeout)
	{
		if (buffer.empty()) [[unlikely]]
			return 0;

		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(m_fd, &writefds);

		timeval tv;
		tv.tv_sec = timeout.count() / 1000;
		tv.tv_usec = (timeout.count() % 1000) * 1000;

		int result = select(m_fd + 1, nullptr, &writefds, nullptr, &tv);
		if (result == -1)
			return -1;

		if (result == 0)
		{
			errno = ETIMEDOUT;
			return -1;
		}

		return ::sendto(m_fd, buffer.data(), buffer.size(), flags.get(),
		                reinterpret_cast<const struct sockaddr*>(&dest), address_len(dest));
	}

	ssize_t recvfrom(std::span<std::byte> buffer, const flags<msgflags> flags, DomainT::address_type* src = nullptr)
	{
		socklen_t src_len = sizeof(typename DomainT::address_type);
		socklen_t* len_ptr = src == nullptr ? nullptr : &src_len;
		return ::recvfrom(m_fd, buffer.data(), buffer.size(), flags.get(),
		                  reinterpret_cast<struct sockaddr*>(src), len_ptr);
	}

protected:
	datagramsocket(datagramsocket&& other) noexcept
	: socket(std::move(other))
	{}

	datagramsocket& operator=(datagramsocket &&other) noexcept
	{
		return static_cast<datagramsocket&>(socket::operator=(std::move(other)));
	}
};

}

#endif
