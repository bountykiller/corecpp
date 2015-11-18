#ifndef CORE_CPP_ANY_H
#define CORE_CPP_ANY_H

#include <memory>

namespace corecpp
{

class any final
{
	void* m_data;
public:
	any() : m_data(0)
	{}
	any(const any&) = delete;
	any(any&& other) : m_data(other.m_data)
	{
		other.m_data = 0;
	};
	template<typename T>
	any(const T& data) : m_data(new T(data))
	{
	}
	template<typename T>
	any(T&& data) : m_data(new T(std::forward<T>(data)))
	{
	}
	~any()
	{
		delete m_data;
		m_data = 0;
	}
	any& operator = (any&& other)
	{
		delete m_data;
		m_data = other.m_data;
		other.m_data = 0;
	}
	
	template<typename T>
	any& operator = (T&& data)
	{
		delete m_data;
		m_data = new T(std::forward<T>(data));
		return *this;
	}

	template<typename T>
	any& operator = (const T& data)
	{
		delete m_data;
		m_data = new T(data);
		return *this;
	}

	template<typename T>
	T& get()
	{
		return *(static_cast<T*>(m_data));
	}

	template<typename T>
	const T& get() const
	{
		return *(static_cast<T*>(m_data));
	}
};

}

#endif
