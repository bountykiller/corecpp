#ifndef VARIANT_H
#define VARIANT_H

#include <utility>
#include <cassert>
#include "meta.h"
#include "any.h"

namespace corecpp
{

template<typename... TArgs>
class variant
{
	any m_data;
	uint m_type_index;
public:
	template<typename T>
	struct index_of
	{
		static constexpr uint value = corecpp::type_index<T, TArgs...>::value;
	};

	variant(const variant&) = delete;
	variant(variant&& other)
	: m_data(std::move(other.m_data)), m_type_index(other.m_type_index)
	{
	}

	template<typename T>
	variant(T&& data)
	: m_data(std::forward<T>(data)), m_type_index(index_of<T>::value)
	{}

	template<typename T>
	variant(const T& data)
	: m_data(data), m_type_index(index_of<T>::value)
	{}

	variant& operator = (const variant& data) = delete;
	variant& operator = (variant&& other)
	{
		if(this == std::addressof(other)) return *this;
		m_data = std::move(other.m_data);
		m_type_index = other.m_type_index;
		return *this;
	}

	template<typename T>
	variant& operator = (T&& data)
	{
		m_data = std::forward<T>(data);
		m_type_index = index_of<T>::value;
		return *this;
	}

	template<typename T>
	variant& operator = (const T& data)
	{
		m_data = data;
		m_type_index = index_of<T>::value;
		return *this;
	}

	uint which() const
	{
		return m_type_index;
	}

	template<typename T>
	T& get()
	{
		assert(m_type_index == index_of<T>::value);
		return m_data.get<T>();
	}

	template<typename T>
	const T& get() const
	{
		assert(m_type_index == index_of<T>::value);
		return m_data.get<T>();
	}
};

}

#endif
