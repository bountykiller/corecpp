#ifndef CORECPP_EXTENSIONS_H
#define CORECPP_EXTENSIONS_H

#include <functional>
#include <tuple>
#include <type_traits>
#include <corecpp/meta/reflection.h>

namespace corecpp
{

namespace
{
	template <std::size_t I, class F, class Tuple, typename... ArgsT>
	struct tuple_apply_impl
	{
		void operator() (F &&f, Tuple &&t, ArgsT&&... args)
		{
			f(std::get<I>(std::forward<Tuple>(t)), std::forward<ArgsT>(args)...);
			tuple_apply_impl<I-1, F, Tuple> impl;
			impl(std::forward<F>(f), std::forward<Tuple>(t), std::forward<ArgsT>(args)...);
		}
	};
	template <class F, class Tuple, typename... ArgsT>
	struct tuple_apply_impl<0, F, Tuple, ArgsT...>
	{
		void operator() (F &&f, Tuple &&t, ArgsT&&... args)
		{
			f(std::get<0>(std::forward<Tuple>(t)), std::forward<ArgsT>(args)...);
		}
	};
}

template <class F, class Tuple, typename... ArgsT>
void tuple_apply(F&& f, Tuple&& t, ArgsT&&... args)
{
	tuple_apply_impl<std::tuple_size<typename std::remove_reference<Tuple>::type>::value - 1, F, Tuple, ArgsT...> impl;
	impl(std::forward<F>(f), std::forward<Tuple>(t), std::forward<ArgsT>(args)...);
}


template<typename T>
using lvalue = typename std::add_lvalue_reference<typename type_resolver<T>::type>::type;

template<typename T>
using rvalue = typename std::add_rvalue_reference<typename type_resolver<T>::type>::type;

template<typename T>
using pvalue = typename std::add_pointer<typename type_resolver<T>::type>::type;

template<typename T>
using concrete = typename type_resolver<T>::type;

/* std::equal_to does not follow noexcept specification, so this class is needed  */
template<typename T>
struct is_equal
{
	constexpr bool operator()(const T &lhs, const T &rhs) const noexcept(noexcept(lhs == rhs))
	{
		return lhs == rhs;
	}
};

/**
 * @brief allows to know if a type is an strongly-typed enum
 * @note will be obsolete in c++23, see std::is_scoped_enum
 */
template<typename T, typename Enable = void>
struct is_strong_enum final
: public std::false_type
{
};

template<typename T>
struct is_strong_enum<T, std::enable_if_t<std::is_enum_v<T>
	&& !std::is_constructible_v<std::underlying_type_t<T>, T>
	>> final
: public std::true_type
{
};

/**
 * @brief allows to know if a class can be browsed through an iterator
 */
template<typename T, typename Enable = void>
struct is_iterable final
: public std::false_type
{
};

template<typename T>
struct is_iterable<T, std::enable_if_t<!std::is_void<typename T::iterator>::value>> final
: public std::true_type
{
	/* NOTE:
	 * the real thing to do here would be to tests if a given type is a Container (using the Container concept)
	 * But that's way to much work for my needs...
	 */
};


/**
 * @brief allows to know if a class is an associative container
 */
template<typename T, typename Enable = void>
struct is_associative final
: public std::false_type
{
};

template<typename T>
struct is_associative<T, std::enable_if_t<!std::is_void<typename T::key_type>::value
	&& !std::is_void<typename T::mapped_type>::value>> final
: public std::true_type
{
};

template<typename T>
inline constexpr bool is_associative_v = is_associative<T>::value;


/**
 * @brief allows to know of a class can be derefenced through the * or the -> operators
 * TODO: is_dereferencable should also be convertible to bool
 */
template<typename T, typename Enable = void>
struct is_dereferencable final
: public std::false_type
{
};
template<typename T>
struct is_dereferencable<T, std::enable_if_t<std::is_pointer<T>::value>> final
: public std::true_type
{
};
template<typename T>
struct is_dereferencable<T, std::enable_if_t<
	std::is_reference_v<decltype(static_cast<T*>(nullptr)->operator*())>
	&& std::is_pointer_v<decltype(static_cast<T*>(nullptr)->operator->())>
	>> final
: public std::true_type
{
};

template<typename T>
inline constexpr bool is_dereferencable_v = is_dereferencable<T>::value;


/**
 * @brief allows to know if a class can be compared for equality
 */
template<typename T, typename Enable = void>
struct is_equality_comparable final
: public std::false_type
{
};

template<typename T>
struct is_equality_comparable<T,
std::enable_if_t<std::is_same<bool, std::invoke_result_t<corecpp::is_equal<T>, const T&, const T&>>::value>> final
: public std::true_type
{
};

template<typename T>
inline constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;


/**
 * @brief allows to know if a class can be compared for equality without throwing
 */
template<typename T, typename Enable = void>
struct is_nothrow_equality_comparable_impl
: public std::false_type
{
};

template<typename T>
struct is_nothrow_equality_comparable_impl<T,
	std::enable_if_t<corecpp::is_equality_comparable<T>::value
	&& std::is_nothrow_invocable<corecpp::is_equal<T>, const T&, const T&>::value
	>>
: public std::true_type
{
};

template<typename T>
struct is_nothrow_equality_comparable final : public is_nothrow_equality_comparable_impl<T>
{};

template<typename T>
inline constexpr bool is_nothrow_equality_comparable_v = is_nothrow_equality_comparable<T>::value;


/**
 * @brief allows to know if a class is a visitable (typically variant)
 */
template<typename T, typename VisitorT, typename Enable = void>
struct is_visitable final
: public std::false_type
{
};

template<typename T, typename VisitorT>
struct is_visitable<T, VisitorT,
std::enable_if_t<std::is_member_function_pointer<decltype(&T::valueless_by_exception)>::value>> final
: public std::true_type
{
};

template<typename T, typename V>
inline constexpr bool is_visitable_v = is_visitable<T,V>::value;


/**
 * @brief allows to know if a class represent a point in time
 */
template<typename T, typename Enable = void>
struct is_time_point final
: public std::false_type
{
};

template<typename T>
struct is_time_point<T,
	std::enable_if_t<std::is_arithmetic<typename T::rep>::value
	&& std::is_class<typename T::period>::value
	&& std::is_class<typename T::duration>::value
	&& std::is_class<typename T::clock>::value
	&& std::is_member_function_pointer<decltype(&T::time_since_epoch)>::value
	>> final
	: public std::true_type
{
};

template<typename T>
inline constexpr bool is_time_point_v = is_time_point<T>::value;




template <template<typename T> typename CondT, typename HeadT, typename... TailT>
struct all_type;

template <template<typename T> typename CondT, typename HeadT>
struct all_type<CondT, HeadT>
{
	static constexpr bool value = CondT<HeadT>::value;
};

template <template<typename T> typename CondT, typename HeadT, typename... TailT>
struct all_type
{
	static constexpr bool value = CondT<HeadT>::value && all_type<CondT, TailT...>::value;
};

template<template<typename T> typename CondT, typename HeadT, typename... TailT>
inline constexpr bool all_type_v = all_type<CondT, HeadT, TailT...>::value;



struct null_function
{
	template <typename... ArgsT>
	void operator ()([[maybe_unused]] ArgsT ...args)
	{
		return;
	}
};

}



#endif
