#ifndef CORE_CPP_FILE_H
#define CORE_CPP_FILE_H

extern "C" {
	#include <unistd.h>
	#include <sys/socket.h>
}

namespace corecpp
{

enum struct msgflags
{
	none        = 0,
	out_of_band = MSG_OOB,       /*<! process out-of-band data */
	dont_route  = MSG_DONTROUTE, /*<! bypass routing, use direct interface */
	end_of_rec	= MSG_EOR,       /*<! data completes record	*/
	dont_wait   = MSG_DONTWAIT,  /*<! do not block */
	nosignal    = MSG_NOSIGNAL   /*<! do not generate SIGPIPE on EOF */
};

class filedesc
{
public:
	filedesc(int fd = -1) noexcept
	: m_fd(fd)
	{}
	virtual ~filedesc()
	{
		if (m_fd >= 0)
			::close(m_fd);
	}
	filedesc(const filedesc &) = delete;
	filedesc& operator=(const filedesc &) = delete;

	std::strong_ordering  operator <=>(const filedesc& other) const noexcept
	{
		return m_fd <=> other.m_fd;
	}

	int get() const noexcept
	{
		return m_fd;
	}

protected:
	filedesc(filedesc &&other) noexcept
	: m_fd(other.m_fd)
	{
		other.m_fd = -1;
	}

	filedesc& operator=(filedesc &&other) noexcept
	{
		if ( this == &other ) [[unlikely]]
			return *this;
		m_fd = other.m_fd;
		other.m_fd = -1;
		return *this;
	}

	template <typename... TArgs>
	int fcntl(int cmd, TArgs... args)
	{
		return ::fcntl(m_fd, cmd, args...);
	}

	template <typename... TArgs>
	int ioctl(int cmd, TArgs... args)
	{
		while(true)
		{
			int res = ::ioctl(m_fd, cmd, args...);
			if (res == 0) [[likely]]
				return res;
			if (errno != EINTR)
				corecpp::throw_system_error(errno, std::system_category());
		}
	}

protected:
	int m_fd; /* !< the undelying fd */
};

}

#endif
