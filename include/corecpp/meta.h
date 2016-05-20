#ifndef CORE_CPP_META_H
#define CORE_CPP_META_H

#include <typeinfo>
#include <type_traits>
#include <initializer_list>
#include <stdexcept>
#include <string>


namespace corecpp
{

namespace
{
	template<typename THead, typename... TTail>
	struct variadic;

	template<typename THead>
	struct variadic<THead>
	{
		typedef THead head_type;

		template<typename T, size_t index = 0>
		struct index_of
		{
		};

		template<size_t index>
		struct index_of<head_type, index>
		{
			static constexpr size_t value = index;
		};

		template<size_t index, typename TResult = head_type>
		struct at
		{
		};

		template<typename TResult>
		struct at<0, TResult>
		{
			typedef TResult type;
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
	struct variadic : protected variadic<TTail...>
	{
		typedef THead head_type;
		typedef variadic<TTail...> tail_type;

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

		template<size_t index, typename TResult = head_type>
		struct at
		{
			typedef typename tail_type::template at<index - 1>::type type;
		};

		template<typename TResult>
		struct at<0, TResult>
		{
			typedef TResult type;
		};

		static const std::type_info& type(size_t i)
		{
			static const std::type_info& result = typeid(head_type);
			if(!i)
				return result;
			return tail_type::type(i-1);
		}
	};


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

template<typename T, typename... TArgs>
struct type_index
{
	static constexpr size_t value = variadic<TArgs...>::template index_of<T>::value;
};

template<size_t index, typename... TArgs>
struct type_at
{
	typedef typename variadic<TArgs...>::template at<index>::type type;
};
template<size_t index, typename... TArgs> using type_at_t = typename type_at<index, TArgs...>::type;

template<typename... ArgsT>
struct mem_fn
{
	template<typename ClassT, typename ResultT>
	static auto fn(ResultT (ClassT::*arg)(ArgsT...))
	{
		return arg;
	}
	template<typename ClassT, typename ResultT>
	static auto const_fn(ResultT (ClassT::*arg)(ArgsT...) const)
	{
		return arg;
	}
};

template<typename... TArgs>
const std::type_info& type(size_t index)
{
	return variadic<TArgs...>::type(index);
}


template<typename T>
using lvalue = typename std::add_lvalue_reference<typename type_resolver<T>::type>::type;

template<typename T>
using rvalue = typename std::add_rvalue_reference<typename type_resolver<T>::type>::type;

template<typename T>
using pvalue = typename std::add_pointer<typename type_resolver<T>::type>::type;

template<typename T>
using concrete = typename type_resolver<T>::type;


}

#endif
