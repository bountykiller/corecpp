#ifndef CORE_CPP_SOCKET_H
#define CORE_CPP_SOCKET_H

#include <cerrno>
#include <cstddef>
#include <span>
#include <system_error>

#include <sys/socket.h>

#include <corecpp/except.h>
#include <corecpp/file.h>
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

enum struct msgflags
{
	none        = 0,       /*<! process out-of-band data */
	out_of_band = MSG_OOB,       /*<! process out-of-band data */
	dont_route  = MSG_DONTROUTE, /*<! bypass routing, use direct interface */
	end_of_rec	= MSG_EOR,       /*<! data completes record	*/
	dont_wait   = MSG_DONTWAIT,  /*<! do not block */
	nosignal    = MSG_NOSIGNAL   /*<! do not generate SIGPIPE on EOF */
};

class socket : public filedesc
{
public:
	socket(int domain, int type, int protocol)
	: filedesc { ::socket(domain, type, protocol) }
	{
		if (m_fd < 0)
			corecpp::throws<std::system_error>(errno, std::system_category());
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

	template<typename ValueT>
	void set_option(int level, int option, const ValueT& value, socklen_t value_len = sizeof(ValueT))
	{
		if (setsockopt(m_fd, level, option, &value, value_len)) [[unlikely]]
			corecpp::throws<std::system_error>(errno, std::system_category());
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
		return ::connect(m_fd, reinterpret_cast<const struct sockaddr*>(&dest),
		                 DomainT::address_len(dest));
	}

	ssize_t send(const std::span<const std::byte> buffer, msgflags flags)
	{
		if (buffer.empty()) [[unlikely]]
			return 0;
		return ::send(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags));
	}

	ssize_t send(const std::span<const std::byte> buffer, msgflags flags, std::chrono::milliseconds timeout)
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

		return ::send(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags));
	}

	ssize_t recv(std::span<std::byte> buffer, msgflags flags)
	{
		return ::recv(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags));
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
		int ret = ::bind(m_fd, reinterpret_cast<const struct sockaddr*>(&addr),
		                 DomainT::address_len(addr));
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

	ssize_t sendto(const std::span<const std::byte> buffer, msgflags flags, const DomainT::address_type &dest)
	{
		if (buffer.empty()) [[unlikely]]
			return 0;

		return ::sendto(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags), dest, DomainT::address_len(dest));
	}

	ssize_t sendto(const std::span<const std::byte> buffer, msgflags flags, const DomainT::address_type &dest, std::chrono::milliseconds timeout)
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

		return ::sendto(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags), dest, DomainT::address_len(dest));
	}

	ssize_t recvfrom(std::span<std::byte> buffer, msgflags flags, const DomainT::address_type &src)
	{
		return ::recvfrom(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags), src, sizeof(src));
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
