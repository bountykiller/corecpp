#ifndef CORE_CPP_BUFFER_H
#define CORE_CPP_BUFFER_H


#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <format>
#include <limits>

extern "C" {
	#include <sys/socket.h>
}

#include <new>
#include <numeric>
#include <span>
#include <type_traits>
#include <utility>

#include <corecpp/algorithm.h>
#include <corecpp/expected.h>
#include <corecpp/io/socket.h>

namespace corecpp
{

_internal corecpp::diagnostic::event_producer& io_logger();

/*********/
/* UNITS */
/*********/

struct bytes final
{
	using value_type = unsigned long long;
	constexpr bytes(value_type value) : m_value { value }
	{}
	value_type value() const noexcept
	{
		return m_value;
	}
private:
	value_type m_value;
};

struct kilobytes final
{
	using value_type = unsigned long long;
	constexpr kilobytes(value_type value) : m_value { value }
	{}
	constexpr bytes to_bytes() const noexcept
	{
		return m_value * 1024;
	}
private:
	value_type m_value;
};

struct megabytes final
{
	using value_type = unsigned long long;
	constexpr megabytes(value_type value) : m_value { value }
	{}
	constexpr bytes to_bytes() const noexcept
	{
		return m_value * 1024 * 1024;
	}
	constexpr kilobytes to_kilobytes() const noexcept
	{
		return m_value * 1024;
	}
private:
	value_type m_value;
};

struct gigabytes final
{
	using value_type = unsigned long long;
	constexpr gigabytes(value_type value) : m_value { value }
	{}
	constexpr bytes to_bytes() const noexcept
	{
		return m_value * 1024 * 1024 * 1024;
	}
	constexpr kilobytes to_kilobytes() const noexcept
	{
		return m_value * 1024 * 1024;
	}
	constexpr megabytes to_megabytes() const noexcept
	{
		return m_value * 1024;
	}
private:
	value_type m_value;
};

constexpr bytes operator ""_B(unsigned long long value) noexcept
{
	return bytes { value };
}
constexpr kilobytes operator ""_KB(unsigned long long value) noexcept
{
	return kilobytes { value };
}
constexpr megabytes operator ""_MB(unsigned long long value) noexcept
{
	return megabytes { value };
}
constexpr gigabytes operator ""_GB(unsigned long long value) noexcept
{
	return gigabytes { value };
}

/*********************/
/* BASE BUFFER CLASS */
/*********************/


class basic_buffer
{
public:
	using value_type = std::byte;
	using size_type = std::size_t;

private:
	value_type* m_buffer = nullptr;
	size_type m_capacity = 0;

protected:
	basic_buffer() noexcept = default;
	basic_buffer(size_type capacity) :
	m_buffer { static_cast<value_type*>(std::malloc(next_power_of_2(capacity))) },
	m_capacity { next_power_of_2(capacity) }
	{
		if ( m_buffer == nullptr )
			corecpp::throws<std::bad_alloc>();
	}

	~basic_buffer()
	{
		std::free(m_buffer);
	}

	basic_buffer(const basic_buffer&) = delete;
	basic_buffer(basic_buffer&& other) noexcept :
	m_buffer { other.m_buffer },
	m_capacity { other.m_capacity }
	{
		other.m_buffer = nullptr;
		other.m_capacity = 0;
	}

	basic_buffer& operator=(const basic_buffer&) = delete;
	basic_buffer& operator=(basic_buffer&& other) noexcept
	{
		if (this == &other) [[unlikely]]
			return *this;
		free(m_buffer);
		m_buffer = other.m_buffer;
		m_capacity = other.m_capacity;
		other.m_buffer = nullptr;
		other.m_capacity = 0;
		return *this;
	}

	std::byte* data() noexcept
	{
		return m_buffer;
	}

	const std::byte* data() const noexcept
	{
		return m_buffer;
	}

	size_type capacity() const noexcept
	{
		return m_capacity;
	}

	void reserve(size_type new_capacity)
	{
		if ( new_capacity <= m_capacity )
			return;
		value_type* new_buffer = static_cast<value_type*>(std::realloc(m_buffer, new_capacity));
		if ( !new_buffer ) [[unlikely]]
			throw std::bad_alloc();
		m_buffer = new_buffer;
		m_capacity = new_capacity;
	}
};


/******************/
/* STREAM BUFFERS */
/******************/

class istream_buffer : public basic_buffer
{
	size_type m_filled = 0;
	size_type m_consumed = 0;

public:
	using value_type = std::byte;
	using size_type = std::size_t;

	istream_buffer() = default;

	explicit istream_buffer(size_type capacity)
	: basic_buffer { capacity },
	  m_filled { 0 },
	  m_consumed { 0 }
	{
	}

	istream_buffer(const istream_buffer&) = delete;
	istream_buffer& operator=(const istream_buffer&) = delete;

	istream_buffer(istream_buffer&& other) noexcept
	: basic_buffer { std::move(other) },
	  m_consumed { other.m_consumed }
	{
		other.clear();
	}

	istream_buffer& operator=(istream_buffer&& other) noexcept
	{
		if ( this == &other ) [[unlikely]]
			return *this;

		basic_buffer::operator=(std::move(other));
		m_filled = other.m_filled;
		m_consumed = other.m_consumed;
		other.clear();
		return *this;
	}

	bool empty() const noexcept
	{
		return (m_filled == m_consumed);
	}

	size_type in_avail() const noexcept
	{
		ensure(m_filled >= m_consumed);
		return m_filled - m_consumed;
	}

	std::span<const std::byte> in() const noexcept
	{
		return { &data()[m_consumed], m_filled - m_consumed };
	}

	void clear() noexcept
	{
		m_consumed = m_filled = 0;
	}

	std::span<std::byte> consume(size_type consumed) noexcept
	{
		ensure(capacity() >= m_filled);
		ensure(m_filled >= m_consumed);

		m_consumed += consumed;
		ensure(m_filled >= m_consumed, "consumed more bytes than available");
		value_type* start = &data()[m_filled];
		return { start, consumed };
	}

	ssize_t read(int fd)
	{
		ensure(capacity() >= m_filled);
		reclaim(); //TODO sub-optimal as we will move data on every read
		size_type unfilled = capacity() - m_filled;
		ssize_t res;
		while (true)
		{
			res = ::read(fd, &data()[m_filled], unfilled);
			if ( res < 0) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
						return res;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category(), std::format("fd {}", fd));
				}
			}
		}

		m_filled += res;
		ensure(capacity() >= m_filled);
		return res;
	}

	/**
	 * Receives data from a socket and stores it in the buffer.
	 *
	 * @param fd The file descriptor of the socket.
	 * @param flags The flags to use for the receive operation.
	 * @param minread The minimmum amount of data to read
	 *
	 * @retval 0 data have been received
	 * @retval EAGAIN no or not enough data available, try later
	 * @retval ECONNRESET The connection have been reset by the peer
	 * @retval EPIPE The connection have been lost
	 *
	 * @throw system_error if an unexpected error happen
	 */
	int receive(int fd, const flags<msgflags> flags, size_type minread = 0)
	{
		ensure(capacity() >= m_filled);

		int available;
		int ret = ::ioctl(fd, FIONREAD, &available);
		if (ret < 0) [[unlikely]]
			corecpp::throw_system_error(errno, std::system_category(), std::format("fd {}", fd));

		size_type unfilled = ensure_capacity(std::max(static_cast<size_type>(available), minread));
		io_logger().debug(std::format("fd {}: ensure {} bytes available => got {})", fd, available, unfilled));
		ensure(unfilled >= minread);

		ssize_t res;
		while (true)
		{
			res = ::recv(fd, &data()[m_filled], unfilled, flags.get());
			if ( res < 0 ) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
					case EPIPE:
					case ECONNRESET:
						return errno;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category(), std::format("fd {}", fd));
				}
			}
			break;
		}

		m_filled += res;
		io_logger().debug(std::format("fd {}: received {} bytes", fd, res),
						  std::format("({}/{} available)", m_filled - m_consumed, capacity()));
		ensure(capacity() >= m_filled);

		if (m_filled + available < minread)
			return EAGAIN;

		return 0;
	}

private:

	void reclaim() noexcept
	{
		m_filled -= m_consumed;
		::memmove(data(), &data()[m_consumed], m_filled);
		m_consumed = 0;
	}

	size_type ensure_capacity(size_type required)
	{
		size_type unfilled = capacity() - m_filled;
		if (unfilled < required)
		{
			if (m_consumed > 0)
			{
				reclaim();
				unfilled = capacity() - m_filled;
			}

			if (unfilled < required)
			{
				reserve(std::max(corecpp::next_power_of_2(m_filled + required), size_type { 1024 }));
				unfilled = capacity() - m_filled;
			}
		}

		return unfilled;
	}
};


class ostream_buffer : basic_buffer
{
public:
	using value_type = std::byte;
	using size_type = std::size_t;

	ostream_buffer() = default;

	explicit ostream_buffer(size_type capacity)
	: basic_buffer { capacity },
	m_filled { 0 },
	m_consumed { 0 }
	{
	}

	ostream_buffer(const ostream_buffer&) = delete;
	ostream_buffer& operator=(const ostream_buffer&) = delete;

	ostream_buffer(ostream_buffer&& other) noexcept :
		basic_buffer { std::move(other) },
		m_filled { other.m_filled },
		m_consumed { other.m_consumed }
	{
		other.clear();
	}

	ostream_buffer& operator=(ostream_buffer&& other) noexcept
	{
		if ( this == &other ) [[unlikely]]
			return *this;

		basic_buffer::operator=(std::move(other));
		m_filled = other.m_filled;
		m_consumed = other.m_consumed;
		other.clear();
		return *this;
	}

	bool empty() const noexcept
	{
		return (m_filled == m_consumed);
	}

	size_type out_avail() const noexcept
	{
		ensure(m_filled >= m_consumed);
		return m_filled - m_consumed;
	}

	std::span<const std::byte> out() const noexcept
	{
		return { &data()[m_consumed], m_filled - m_consumed };
	}

	void enqueue(std::span<const std::byte> in_data)
	{
		ensure(capacity() >= m_filled);
		size_type free_data = ensure_capacity(in_data.size());

		ensure(free_data >= in_data.size());
		memcpy(&data()[m_filled], in_data.data(), in_data.size());
		m_filled += in_data.size();
	}

	void clear() noexcept
	{
		m_filled = m_consumed = 0;
	}

	ssize_t write(int fd)
	{
		ensure(m_filled >= m_consumed);
		size_type unconsumed = m_filled - m_consumed;
		ssize_t res;
		while (true)
		{
			res = ::write(fd, &data()[m_consumed], unconsumed);
			if ( res < 0) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN:
						return res;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category());
				}
			}
			break;
		}

		m_consumed += res;
		ensure(m_filled >= m_consumed);
		return res;
	}

	/**
	 * send data in the buffer.
	 *
	 * @param fd The file descriptor of the socket.
	 * @param flags The flags to use for the receive operation.
	 *
	 * @retval 0 data have been received
	 * @retval EAGAIN no data available, try later
	 * @retval ECONNRESET The connection have been reset by the peer
	 * @retval EPIPE The connection have been lost
	 *
	 * @throw system_error if an unexpected error happen
	 */
	int send(int fd, const flags<msgflags> flags)
	{
		ensure(m_filled >= m_consumed);
		if ( m_filled == 0 )
		{
			io_logger().info(std::format("fd {}: Nothing to send", fd));
			return 0;
		}

		size_type unconsumed = m_filled - m_consumed;
		ssize_t res;
		while (true)
		{
			res = ::send(fd, &data()[m_consumed], unconsumed, flags.get());
			if ( res < 0 ) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
					case EPIPE:
					case ECONNRESET:
						return errno;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category(), std::format("fd {}", fd));
				}
			}
			break;
		}

		m_consumed += res;
		io_logger().debug(std::format("fd {}: sent {} bytes", fd, res),
						  std::format("{}/{} sent", m_consumed, m_filled));
		ensure(m_filled >= m_consumed);
		return 0;
	}

	/**
	 * Receives data from a socket and stores it in the buffer.
	 *
	 * @param fd The file descriptor of the socket.
	 * @param flags The flags to use for the receive operation.
	 * @param dst address of the destination
	 *
	 * @retval 0 data have been received
	 * @retval EAGAIN no data available, try later
	 * @retval ECONNRESET The connection have been reset by the peer
	 * @retval EPIPE The connection have been lost
	 *
	 * @throw system_error if an unexpected error happen
	 */
	template <typename AddressT>
	requires (std::is_convertible_v<AddressT, sockaddr> &&  !std::is_same_v<sockaddr, AddressT>)
	int sendto(int fd, const flags<msgflags> flags, const AddressT& dst)
	{
		ensure(m_filled >= m_consumed);
		if ( m_filled == 0 )
		{
			io_logger().info(std::format("fd {}: Nothing to send", fd));
			return 0;
		}

		size_type unconsumed = m_filled - m_consumed;
		ssize_t res;
		while (true)
		{
			res = ::sendto(fd, &data()[m_consumed], unconsumed, flags.get(), &dst, address_len(dst));
			if ( res < 0 ) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
					case EPIPE:
					case ECONNRESET:
						return errno;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category(), std::format("fd {}", fd));
				}
			}
			break;
		}

		m_consumed += res;
		io_logger().debug(std::format("fd {}: sent {} bytes", fd, res),
						  std::format("{}/{} sent", m_consumed, m_filled));
		ensure(m_filled >= m_consumed);
		return 0;
	}

private:

	size_type ensure_capacity(size_type required)
	{
		size_type free_data = capacity() - m_filled;
		if (required > free_data)
		{
			reclaim();
			free_data = capacity() - m_filled;

			if (required > free_data)
			{
				reserve(std::max(corecpp::next_power_of_2(required + m_filled), size_type { 1024 }));
				free_data = capacity() - m_filled;
			}
		}
		return free_data;
	}

	void reclaim()
	{
		size_type unconsumed = m_filled - m_consumed;
		::memmove(data(), &data()[m_consumed], unconsumed);
		m_consumed = 0;
		m_filled = unconsumed;
	}

	size_type m_filled = 0;
	size_type m_consumed = 0;
};

/*****************/
/* DGRAM BUFFERS */
/*****************/

class idgram_buffer : basic_buffer
{
	struct sockaddr_storage m_addr = {};
	size_type m_filled = std::numeric_limits<size_type>::max();

public:

	bool empty() const noexcept
	{
		return (m_filled == std::numeric_limits<size_type>::max());
	}

	size_type in_avail() const noexcept
	{
		return m_filled;
	}

	template <typename AddressT>
	// requires (std::is_convertible_v<AddressT, sockaddr> &&  !std::is_same_v<sockaddr, AddressT>)
	std::span<const std::byte> in(AddressT &src) const noexcept
	{
		::memcpy(&src, &m_addr, sizeof(src));
		if (m_filled != std::numeric_limits<size_type>::max()) [[likely]]
			return { data(), m_filled };
		return { };
	}

	/**
	 * mark the buffer as consumed
	 * WARNING: unlike stream buffer, the dgram buffer only owns one dgram at a time (no caching)
	 */
	void consume() noexcept
	{
		m_filled = std::numeric_limits<size_type>::max();
	}

	/**
	 * Receives data from a socket and stores it in the buffer.
	 *
	 * @param fd The file descriptor of the socket.
	 * @param flags The flags to use for the receive operation.
	 * @param minread The minimmum amount of data to read
	 *
	 * @retval 0 data have been received
	 * @retval EAGAIN no or not enough data available, try later
	 * @retval ECONNRESET The connection have been reset by the peer
	 * @retval EPIPE The connection have been lost
	 *
	 * @throw system_error if an unexpected error happen
	 */
	int receive(int fd, const flags<msgflags> flags, size_type minread = 0)
	{
		int available;
		// NOTE: we get the 1st dgram size on Linux, the size of all waiting dgram on other oses
		int ret = ::ioctl(fd, FIONREAD, &available);
		if (ret < 0) [[unlikely]]
			corecpp::throw_system_error(errno, std::system_category(), std::format("fd {}", fd));

		if (available < minread)
			return EAGAIN;

		reserve(std::max(next_power_of_2(available), size_type { 1024 }));
		ensure(capacity() >= minread);

		struct iovec vec = { data(), capacity() };
		char cmsgbuf[CMSG_SPACE(sizeof(struct ucred))];
		struct msghdr message =
		{
			.msg_name = &m_addr,
			.msg_namelen = sizeof(m_addr),
			.msg_iov = &vec,
			.msg_iovlen = 1,
			.msg_control = cmsgbuf,
			.msg_controllen = sizeof(cmsgbuf),
			.msg_flags = 0
		};
		ssize_t res;
		while (true)
		{
			res = ::recvmsg(fd, &message, flags.get());
			if ( res < 0 ) [[unlikely]]
			{
				switch (errno)
				{
					case EAGAIN: /* aka EWOULDBLOCK */
					case EPIPE:
					case ECONNRESET:
						return errno;
					case EINTR:
						continue;
					default:
						corecpp::throw_system_error(errno, std::system_category(), std::format("fd {}", fd));
				}
			}
			break;
		}

		io_logger().debug(std::format("fd {}: received dgram of {} bytes", fd, res));

		/* debug code
		for (struct cmsghdr *hdr = CMSG_FIRSTHDR(&message); hdr; hdr = CMSG_NXTHDR(&message, hdr))
		{
			if (hdr->cmsg_level == SOL_SOCKET && hdr->cmsg_type == SCM_CREDENTIALS)
			{
				struct ucred *creds = reinterpret_cast<struct ucred *>(CMSG_DATA(hdr));
				io_logger().debug(std::format("fd {}: from {} / {} / {} bytes", fd, creds->gid, creds->uid, creds->pid));
			}
		}
		*/

		ensure(capacity() >= res);
		if (m_filled == std::numeric_limits<size_type>::max())
			m_filled = res;
		else
			m_filled += res;
		return 0;
	}
};

/**
 * Queue of datagrams waiting to be sent.
 *
 * All the payloads are copied in a single contiguous memory block, one after
 * the other, so the caller does not have to keep its data alive after a call
 * to enqueue(). The queue only holds the boundaries (offset + length) and the
 * destination address of each pending datagram. Offsets are used instead of
 * pointers because the memory block may be reallocated by a subsequent
 * enqueue().
 *
 * \remarks datagrams are always sent in the order they have been enqueued, and
 *          a datagram is never splitted: a partial send simply leaves the
 *          datagram at the head of the queue until it is fully sent.
 */
class odgram_buffer : basic_buffer
{
public:
	using value_type = std::byte;
	using size_type = std::size_t;

private:

	struct pending_datagram
	{
		size_type offset;           // start of the payload in the unique buffer
		size_type length;           // size of the payload
		struct sockaddr_storage dst; // destination address
		socklen_t dstlen;
	};

	std::deque<struct pending_datagram> m_pending;
	size_type m_filled;

public:

	explicit odgram_buffer() :
		basic_buffer {},
		m_pending {},
		m_filled { 0 }
	{
	}

	explicit odgram_buffer(size_type capacity) :
		basic_buffer { capacity },
		m_pending {},
		m_filled { 0 }
	{
	}

	odgram_buffer(const odgram_buffer&) = delete;
	odgram_buffer& operator=(const odgram_buffer&) = delete;

	odgram_buffer(odgram_buffer&& other) noexcept :
		basic_buffer { std::move(other) },
		m_pending { std::move(other.m_pending) },
		m_filled { other.m_filled }
	{
		other.m_filled = 0;
	}

	odgram_buffer& operator=(odgram_buffer&& other) noexcept
	{
		if ( this == &other ) [[unlikely]]
			return *this;

		basic_buffer::operator=(std::move(other));
		m_pending = std::move(other.m_pending);
		m_filled = other.m_filled;
		other.m_filled = 0;
		return *this;
	}

	/**
	 * \return true if no datagram is waiting to be sent.
	 */
	bool empty() const noexcept
	{
		return m_pending.empty();
	}

	/**
	 * \return the number of datagrams waiting to be sent.
	 */
	size_type queued() const noexcept
	{
		return m_pending.size();
	}

	/**
	 * \return the amount of bytes still waiting to be sent, including the
	 * unsent part of the datagram at the head of the queue.
	 */
	size_type out_avail() const noexcept
	{
		return std::accumulate(m_pending.begin(), m_pending.end(), 0,
			[](size_type accumulator, const pending_datagram& dg)
			{
				return accumulator + dg.length;
			});
	}

	/**
	 * Append a datagram at the end of the queue, copying its payload in the
	 * buffer just after the previous one.
	 *
	 * @param buffer The datagram to send.
	 * @param dst The destination address of the datagram.
	 */

	template <typename AddressT>
	// requires (std::is_convertible_v<AddressT, sockaddr> &&  !std::is_same_v<sockaddr, AddressT>)
	void enqueue(const std::span<const std::byte> buffer, const AddressT& dst)
	{
		ensure_capacity(buffer.size());
		ensure(capacity() - m_filled >= buffer.size());

		pending_datagram dg { };
		dg.offset = m_filled;
		dg.length = buffer.size();
		::memcpy(&data()[m_filled], buffer.data(), buffer.size());

		dg.dstlen = address_len(dst);
		ensure(sizeof(m_pending.front().dst) >= dg.dstlen, "address too long");
		::memcpy(&dg.dst, &dst, dg.dstlen);
		io_logger().debug(std::format("queued a datagram of {} bytes (dst: {})",
		                              buffer.size(), to_string(dst)));

		m_filled += buffer.size();
		m_pending.push_back(dg);
	}


	void clear() noexcept
	{
		m_pending.clear();
		m_filled = 0;
	}

	/**
	 * Send as many queued datagrams as possible, without ever splitting one.
	 *
	 * @param fd The file descriptor of the socket.
	 * @param flags The flags to use for the send operation.
	 *
	 * @retval 0 all the datagrams have been sent
	 * @retval EAGAIN the socket send buffer is full, try later
	 * @retval ECONNRESET The connection have been reset by the peer
	 * @retval EPIPE The connection have been lost
	 *
	 * @throw system_error if an unexpected error happen
	 */
	int send(int fd, const flags<msgflags> flags)
	{
		size_type sent = 0;
		while ( !m_pending.empty() )
		{
			const pending_datagram& dg = m_pending.front();
			std::span<const std::byte> payload = { &data()[sent], dg.length };

			ssize_t res;
			while (true)
			{
				io_logger().debug(std::format("send a datagram of {} bytes", payload.size()));
				res = ::sendto(fd, payload.data(), payload.size(), flags.get(),
				               reinterpret_cast<const struct sockaddr*>(&dg.dst),
				               dg.dstlen);
				if ( res < 0 ) [[unlikely]]
				{
					switch (errno)
					{
						case EAGAIN: /* aka EWOULDBLOCK */
						case EPIPE:
						case ECONNRESET:
							io_logger().debug(std::format("fd {}: send again, {}/{} datagrams sent",
														  fd, queued() - m_pending.size(), queued()));

							return errno;
						case EINTR:
							continue;
						default:
							corecpp::throw_system_error(errno, std::system_category(), std::format("fd {}", fd));
					}
				}
				else
					sent += payload.size();
				break;
			}

			io_logger().debug(std::format("fd {}: sent a datagram of {} bytes", fd, res));
			m_pending.pop_front();
		}

		/**
		 * Move the still queued datagrams at the beginning of the buffer, so the
		 * space consumed by the already sent ones is given back.
		 */
		::memmove(data(), &data()[sent], m_filled - sent);
		for (pending_datagram& dg : m_pending)
			dg.offset -= sent;
		m_filled -= sent;

		return 0;
	}

private:
	/**
	 * Ensure that at least 'required' bytes are writable after the last queued
	 * datagram, compacting then growing the memory block if needed.
	 *
	 * \return the free space available at the end of the buffer.
	 */
	size_type ensure_capacity(size_type required)
	{
		if (required > capacity() - m_filled)
		{
			reserve(std::max(corecpp::next_power_of_2(required + m_filled), size_type { 1024 }));
		}
		return capacity() - m_filled;
	}
};

}

#endif
