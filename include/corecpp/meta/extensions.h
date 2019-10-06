#ifndef CORECPP_EXTENSIONS_H
#define CORECPP_EXTENSIONS_H

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


/**
 * @class is_iterable
 * @brief allows to know if a class can be browsed through an iterator
 */
template<typename T, typename Enable = void>
struct is_iterable
{
	static constexpr bool value = false;
};

template<typename T>
struct is_iterable<T, std::enable_if_t<!std::is_void<typename T::iterator>::value>>
{
	/* NOTE:
	 * the real thing to do here would be to tests if a given type is a Container (using the Container concept)
	 * But that's way to much work for my needs...
	 */
	static constexpr bool value = true;
};


/**
 * @class is_associative
 * @brief allows to know if a class is an associative container
 */
template<typename T, typename Enable = void>
struct is_associative
{
	static constexpr bool value = false;
};

template<typename T>
struct is_associative<T, std::enable_if_t<!std::is_void<typename T::key_type>::value
	&& !std::is_void<typename T::mapped_type>::value>>
{
	static constexpr bool value = true;
};

template<typename T>
inline constexpr bool is_associative_v = is_associative<T>::value;


/**
 * @class is_dereferencable
 * @brief allows to know of a class can be derefenced through the * or the -> operators
 * TODO: is_dereferencable should also be convertible to bool
 */
template<typename T, typename Enable = void>
struct is_dereferencable
{
	static constexpr bool value = false;
};
template<typename T>
struct is_dereferencable<T, std::enable_if_t<std::is_pointer<T>::value>>
{
	static constexpr bool value = true;
};
template<typename T>
struct is_dereferencable<T, std::enable_if_t<
	std::is_reference_v<decltype(static_cast<T*>(nullptr)->operator*())>
	&& std::is_pointer_v<decltype(static_cast<T*>(nullptr)->operator->())>
	>>
{
	static constexpr bool value = true;
};

template<typename T>
inline constexpr bool is_dereferencable_v = is_dereferencable<T>::value;


/**
 * @class is_visitable
 * @brief allows to know if a class is a visitable (typically variant)
 */
template<typename T, typename VisitorT, typename Enable = void>
struct is_visitable
{
	static constexpr bool value = false;
};

template<typename T, typename VisitorT>
struct is_visitable<T, VisitorT,
	std::enable_if_t<std::is_member_function_pointer<decltype(&T::valueless_by_exception)>::value>>
{
	static constexpr bool value = true;
};

template<typename T, typename V>
inline constexpr bool is_visitable_v = is_visitable<T,V>::value;

}

#endif
