#ifndef CORECPP_REFLECTON_H
#define CORECPP_REFLECTON_H

#include <codecvt>
#include <iostream>
#include <typeinfo>
#include <typeinfo>
#include <type_traits>
#include <initializer_list>
#include <locale>
#include <stdexcept>
#include <string>

namespace corecpp
{

namespace
{
	template<class T, class enable = void>
	struct type_resolver
	{
		using type = T;
	};

	template<class T>
	struct type_resolver<T, typename std::enable_if<std::is_rvalue_reference<T>::value >::type>
	{
		using type = typename std::remove_reference<T>::type;
	};

	template<class T>
	struct type_resolver<T, typename std::enable_if<std::is_lvalue_reference<T>::value >::type>
	{
		using type = typename std::remove_reference<T>::type;
	};

	template<class T>
	struct type_resolver<T, typename std::enable_if<std::is_pointer<T>::value >::type>
	{
		using type = typename std::remove_pointer<T>::type;
	};
}


template<typename THead, typename... TTail>
struct typelist;

template<typename THead>
struct typelist<THead>
{
	using head_type = THead;

	template<typename T, size_t index = 0>
	struct index_of
	{
	};

	template<size_t index>
	struct index_of<head_type, index>
	{
		static constexpr size_t value = index;
	};
	template<typename T>
	static inline constexpr auto index_of_v = index_of<T>::value;

	template<size_t index, typename TResult = head_type>
	struct at
	{
	};

	template<typename TResult>
	struct at<0, TResult>
	{
		using type = TResult;
	};

	template<typename T>
	struct append
	{
		using type = typelist<T, head_type>;
	};

	template<template <typename T> class TransformerT>
	struct transform
	{
		using type = typelist<TransformerT<head_type>>;
	};

	template<template <typename T> class ApplyT>
	struct apply //TODO: find a better name
	{
		using type = ApplyT<head_type>;
	};

	static const std::type_info& type(size_t i)
	{
		static const std::type_info& result = typeid(head_type);
		if(i)
			throw std::out_of_range(std::to_string(i));
		return result;
	}
};

template<typename THead, typename... TTail>
struct typelist : protected typelist<TTail...>
{
	using head_type = THead;
	using tail_type = typelist<TTail...>;

	template<typename T, size_t index = 0>
	struct index_of
	{
		static constexpr size_t value = tail_type::template index_of<T>::value + 1;
	};

	template<size_t index>
	struct index_of<head_type, index>
	{
		static constexpr size_t value = index;
	};
	template<typename T>
	static inline constexpr auto index_of_v = index_of<T>::value;

	template<size_t index, typename TResult = head_type>
	struct at
	{
		using type = typename tail_type::template at<index - 1>::type;
	};

	template<typename TResult>
	struct at<0, TResult>
	{
		using type = TResult;
	};

	template<typename T>
	struct append
	{
		using type = typelist<T, head_type, TTail...>;
	};

	template<template <typename T> class TransformerT>
	struct transform
	{
		using head = TransformerT<head_type>;
		using tail = typename tail_type::template transform<TransformerT>::type;
		using type = typename tail::template append<head>::type;
	};

	template<template <typename... T> class ApplyT>
	struct apply //TODO: find a better name
	{
		using type = ApplyT<head_type, TTail...>;
	};

	//TODO: add the method to transform typelist<a,b,c> into std::tuple<a,b,c> for example

	static const std::type_info& type(size_t i)
	{
		static const std::type_info& result = typeid(head_type);
		if(!i)
			return result;
		return tail_type::type(i-1);
	}
};

template<typename T>
struct value_type
{
	using type = typename T::value_type;
};
template<typename T> using value_type_t = typename value_type<T>::type;

template<typename T, typename... TArgs>
struct type_index
{
	static constexpr size_t value = typelist<TArgs...>::template index_of<T>::value;
};
template<typename T, typename... TArgs>
inline constexpr std::size_t type_index_v = type_index<T, TArgs...>::value;

template<size_t index, typename... TArgs>
struct type_at
{
	typedef typename typelist<TArgs...>::template at<index>::type type;
};
template<size_t index, typename... TArgs> using type_at_t = typename type_at<index, TArgs...>::type;

template<typename... TArgs>
const std::type_info& type(size_t index)
{
	return typelist<TArgs...>::type(index);
}



template<class T>
struct mem_val{};

template<typename TClass, typename TType>
struct mem_val<TType TClass::*>
{
	using class_type = TClass;
	using value_type = TType;
	using const_value_type = const TType;
};

template<typename... ArgsT>
struct mem_fn
{
	template<typename ClassT, typename ResultT>
	static auto fn(ResultT (ClassT::*arg)(ArgsT...)) -> decltype(arg)
	{
		return arg;
	}
	template<typename ClassT, typename ResultT>
	static auto const_fn(ResultT (ClassT::*arg)(ArgsT...) const) -> decltype(arg)
	{
		return arg;
	}
};

class property_name
{
	std::string m_value;
	std::wstring m_wide_value;
public:
	property_name(const char* value)
	: m_value(value)
	{
		m_wide_value = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(value);
	}
	operator const std::string& () const
	{
		return m_value;
	}
	operator const std::wstring& () const
	{
		return m_wide_value;
	}
	bool operator == (const std::string& str) const
	{
		return str == m_value;
	}
	bool operator == (const std::wstring& str) const
	{
		return str == m_wide_value;
	}
	const std::string& str() const
	{
		return m_value;
	}
	const std::wstring& wstr() const
	{
		return m_wide_value;
	}
};

template <typename ValueT>
struct property
{
	using class_type = typename mem_val<ValueT>::class_type;
	using value_type = typename mem_val<ValueT>::value_type;
	using const_value_type = typename mem_val<ValueT>::const_value_type;

private:
	property_name m_name;
	const ValueT m_value;

public:
	template <typename T>
	property(const char* n, const T v)
	: m_name(n), m_value(v)
	{
	}
	value_type& get(class_type& obj) const
	{
		return obj.*m_value;
	}
	const_value_type& get(const class_type& obj) const
	{
		return obj.*m_value;
	}
	const_value_type& cget(const class_type& obj) const
	{
		return obj.*m_value;
	}
	const std::wstring& name() const
	{
		return m_name.wstr();
	}
};

template <typename StringT, typename ValueT>
auto make_property(StringT&& name, ValueT&& value)
{
	return property<ValueT>(std::forward<StringT>(name), std::forward<ValueT>(value));
}

template <typename EnumT>
auto underlying_value(EnumT e)
{
	return static_cast<std::underlying_type_t<EnumT>>(e);
}

}

#endif
