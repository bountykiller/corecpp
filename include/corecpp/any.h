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
	: m_data(other.m_data), m_deleter(std::move(other.m_deleter))
	{
		other.m_data = nullptr;
	};
	/* using perfect-forwarding can add extra-reference to T,
	 * which becomes then non new-constructible.
	 * The remove_reference is here to remove this extra-reference
	 * in order to instanciate the right type.
	 */
	template<typename T, typename RealT = typename std::remove_reference<T>::type>
	any(T&& data)
	: m_data(new RealT (std::forward<T>(data))), m_deleter(make_deleter<RealT >())
	{
	}
	~any()
	{
		clear();
	}
	any& operator = (any&& other)
	{
		m_data = other.m_data;
		m_deleter = std::move(other.m_deleter);
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

	bool operator < (const any& other) const
	{
		return m_data < other.m_data;
	}

	template<typename T>
	T& get()
	{
		return *(static_cast<T*>(m_data));
	}

	template<typename T>
	const T& get() const
	{
		return *(static_cast<const T*>(m_data));
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
