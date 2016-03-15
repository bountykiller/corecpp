#ifndef CORE_CPP_ANY_H
#define CORE_CPP_ANY_H

#include <functional>
#include <memory>

namespace corecpp
{

class any final
{
	void* m_data;
	std::function<void(void*)> m_deleter;
	template<typename T>
	std::function<void(void*)> make_deleter()
	{
		return [](void *ptr) { std::default_delete<T>()(static_cast<T*>(ptr)); };
	}
public:
	any(std::nullptr_t = nullptr) : m_data(nullptr), m_deleter()
	{}
	any(const any&) = delete;
	any(any&& other)
	: m_data(other.m_data)
	{
		other.m_data = nullptr;
	};
	template<typename T>
	any(const T& data) 
	: m_data(new T(data)), m_deleter(make_deleter<T>())//TODO use std::rebind or something like that
	{
	}
	template<typename T>
	any(T&& data)
	: m_data(new T(std::forward<T>(data))), m_deleter(make_deleter<T>())
	{
	}
	~any()
	{
		clear();
	}
	any& operator = (any&& other)
	{
		m_data = other.m_data;
		other.m_data = nullptr;
		return *this;
	}
	template<typename T>
	any& operator = (T&& data)
	{
		clear();
		m_data = new T(std::forward<T>(data));
		m_deleter = make_deleter<T>();
		return *this;
	}

	template<typename T>
	any& operator = (const T& data)
	{
		clear();
		m_data = new T(data);
		m_deleter = make_deleter<T>();
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
	void clear()
	{
		if (m_data)
		{
			m_deleter(m_data);
			m_data = nullptr;
		}
	}
};

}

#endif
