#ifndef CORE_CPP_SOCKET_H
#define CORE_CPP_SOCKET_H

#include <cerrno>
#include <cstddef>
#include <span>
#include <system_error>

#include <sys/socket.h>

#include <corecpp/except.h>
#include <corecpp/file.h>
#include <corecpp/reflection.h>
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
	out_of_band = MSG_OOB,       /*<! process out-of-band data */
	dont_route  = MSG_DONTROUTE, /*<! bypass routing, use direct interface */
	end_of_rec	= MSG_EOR,       /*<! data completes record	*/
	dont_wait   = MSG_DONTWAIT,  /*<! do not block */
	end_of_file = MSG_EOF,       /*<! data completes transaction */
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
class streamsocket : public socket
{
public:
	streamsocket(int protocol)
	: Socket(DomainT::domain, SOCK_STREAM, protocol)
	{}
	streamsocket(const streamsocket& other) = delete;
	streamsocket& operator=(const streamsocket &) = delete;

	ssize_t send(const std::span<const std::byte> buffer, msgflags flags)
	{
		if (buffer.empty()) [[unlikely]]
			return 0;
		return ::send(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags););
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
	streamsocket(streamsocket&& other) noexcept
	: socket(std::move(other))
	{}

	streamsocket& operator=(streamsocket &&other) noexcept
	{
		return static_cast<streamsocket&>(socket::operator=(std::move(other)));
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

        return ::sendto(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags), dest, DomainT::address_len(addr));
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

        return ::sendto(m_fd, buffer.data(), buffer.size(), corecpp::to_underlying(flags), dest, DomainT::address_len(addr));
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
