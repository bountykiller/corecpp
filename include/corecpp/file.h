#ifndef CORE_CPP_FILE_H
#define CORE_CPP_FILE_H

extern "C" {
	#include <unistd.h>
}

namespace corecpp
{

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


protected:
	int m_fd;
};

}

#endif
