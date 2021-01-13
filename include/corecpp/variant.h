#ifndef VARIANT_H
#define VARIANT_H

#include <memory>
#include <type_traits>
#include <utility>

#include <corecpp/algorithm.h>
#include <corecpp/any.h>
#include <corecpp/except.h>
#include <corecpp/meta/reflection.h>
#include <corecpp/meta/extensions.h>

/* compiler-dependent stuff */
#include <cxxabi.h>

namespace corecpp
{

namespace
{
	template<typename VariantT, uint pos, typename VisitorT, typename... ArgsT>
	struct variant_apply
	{
		auto operator()(VariantT& v, VisitorT&& visitor, ArgsT&&... args)
		{
			if (v.index() != pos)
			{
				variant_apply<VariantT, pos - 1, VisitorT, ArgsT...> applier;
				return applier(v, std::forward<VisitorT>(visitor), std::forward<ArgsT>(args)...);
			}
			using match_type = typename VariantT::template type_at<pos>::type;
			return visitor(v.template get<match_type>(), std::forward<ArgsT>(args)...);
		}
	};

	template<typename VariantT, typename VisitorT, typename... ArgsT>
	struct variant_apply<VariantT, 0, VisitorT, ArgsT...>
	{
		auto operator()(VariantT& v, VisitorT&& visitor, ArgsT&&... args)
		{
			using match_type = typename VariantT::template type_at<0>::type;
			return visitor(v.template get<match_type>(), std::forward<ArgsT>(args)...);
		}
	};
}

/*!
 * \brief class intented to implement the variant concept
 * NOTE: While this class tend to use some of the stl vocabulary, it is not stl-compliant
 * Unlike stl-variant, this class should support references types (though some operations like copies don't work woth them)
 */
template<typename... TArgs>
class variant
{
	using this_type = corecpp::variant<TArgs...>;
	int m_type_index;
	unsigned char m_data[sizeof(std::aligned_union_t<1, TArgs...>)];
public:
	template<typename T>
	struct index_of
	{
		static inline constexpr uint value = corecpp::type_index<T, TArgs...>::value;
	};
	template<typename T>
	static inline constexpr uint index_of_v = index_of<T>::value;

	template<size_t index>
	struct type_at
	{
		using type = typename corecpp::type_at<index, TArgs...>::type;
	};
	template<size_t index> using type_at_t = typename type_at<index>::type;

	/* TODO */
	static inline bool has_reference = corecpp::all_type<std::is_reference, TArgs...>::value;
	static inline constexpr bool is_move_constructible = corecpp::all_type<std::is_move_constructible, TArgs...>::value;
	static inline constexpr bool is_nothrow_move_constructible = corecpp::all_type<std::is_nothrow_move_constructible, TArgs...>::value;
	static inline constexpr bool is_copy_constructible = corecpp::all_type<std::is_copy_constructible, TArgs...>::value;
	static inline constexpr bool is_nothrow_copy_constructible = corecpp::all_type<std::is_nothrow_copy_constructible, TArgs...>::value;

	/* Here the behaviour differs from STL.
	 * Default initialisation return a value-less variant,
	 * while in the STL it returns a variant with the first type default-initialised
	 */
	variant()
	: m_type_index(-1)
	{
	}
	//variant(std::enable_if_t<is_move_constructible, variant&&> other)
	variant(variant&& other)
	noexcept(is_nothrow_move_constructible)
	: m_type_index(other.m_type_index)
	{
		if (m_type_index == -1)
			return;
		visit([&other](auto& value) {
			using ValueT = std::remove_reference_t<decltype(value)>;
			new (&value) ValueT(std::move(other.template get<ValueT>()));
		});
	}
	//variant(std::enable_if_t<is_copy_constructible, const variant&> other)
	variant(const variant& other)
	noexcept(is_nothrow_copy_constructible)
	: m_type_index(other.m_type_index)
	{
		if (m_type_index == -1)
			return;
		visit([&other](auto& value) {
			using ValueT = std::remove_reference_t<decltype(value)>;
			new (&value) ValueT(other.template get<ValueT>());
		});
	}

	template<typename T>
	variant(T&& data)
	: m_type_index(index_of<typename std::remove_reference<T>::type>::value)
	{
		new(&m_data) std::remove_reference_t<T>(std::forward<T>(data));
	}
	~variant()
	{
		reset();
		m_type_index = -1;
	}

	variant& operator = (const variant& other)
	{
		if(this == std::addressof(other))
			return *this;
		reset();
		m_type_index = other.m_type_index;
		visit([&other](auto& value) {
			using ValueT = std::remove_reference_t<decltype(value)>;
			new (&value) ValueT(other.template get<ValueT>());
		});
		return *this;
	}
	variant& operator = (variant&& other)
	{
		if(this == std::addressof(other))
			return *this;
		reset();
		m_type_index = other.m_type_index;
		visit([&other](auto& value) {
			using ValueT = std::remove_reference_t<decltype(value)>;
			new (&value) ValueT(std::move(other.template get<ValueT>()));
		});
		return *this;
	}

	template<typename T>
	variant& operator = (T&& data)
	{
		using ValueT = std::remove_reference_t<T>;
		if (static_cast<void*>(std::addressof(this->m_data))
			== static_cast<void*>(std::addressof(data)))
			return *this;
		reset();
		m_type_index = index_of<T>::value;
		new (m_data) ValueT(std::forward<T>(data));
		return *this;
	}

	template<typename T>
	variant& operator = (const T& data)
	{
		using ValueT = std::remove_reference_t<T>;
		if (static_cast<void*>(std::addressof(this->m_data))
			== static_cast<void*>(std::addressof(data)))
			return *this;
		reset();
		m_type_index = index_of<T>::value;
		new (m_data) ValueT(std::forward<T>(data));
		return *this;
	}

	bool operator < (const variant& other) const
	{
		if (m_type_index != other.m_type_index)
			return m_type_index < other.m_type_index;
		return m_data < other.m_data;
	}

	int index() const
	{
		return m_type_index;
	}

	/* compiler-dependent stuff */
	std::string which() const
	{
		return visit([](auto& value) {
			int status;
			std::unique_ptr<char> name { abi::__cxa_demangle(typeid(std::decay_t<decltype(value)>).name(), 0, 0, &status) };
			std::string res { name.get() };
			return res;
		});
	}

	template<typename T>
	T& get()
	{
		if (m_type_index != index_of<T>::value)  [[unlikely]]
		{
			if (m_type_index < 0)
				corecpp::throws<corecpp::bad_access>("valueless");
			int status;
			std::unique_ptr<char> name { abi::__cxa_demangle(typeid(std::decay_t<T>).name(), 0, 0, &status) };
			std::string res { name.get() };
			corecpp::throws<corecpp::bad_access>(corecpp::concat<std::string>({ res, " expected, got ", which() }));
		}
		return reinterpret_cast<T&>(*(reinterpret_cast<char*>(m_data)));
	}

	template<typename T>
	const T& get() const
	{
		if (m_type_index != index_of<T>::value)  [[unlikely]]
		{
			if (m_type_index < 0)
				corecpp::throws<corecpp::bad_access>("valueless");
			int status;
			std::unique_ptr<char> name { abi::__cxa_demangle(typeid(std::decay_t<T>).name(), 0, 0, &status) };
			std::string res { name.get() };
			corecpp::throws<corecpp::bad_access>(corecpp::concat<std::string>({ res, " expected, got ", which() }));
		}
		return *(reinterpret_cast<const T*>(m_data));
	}

	template<typename T, typename ArgT>
	variant& set (ArgT&& arg)
	{
		using ValueT = std::remove_reference_t<T>;
		if (static_cast<const void*>(std::addressof(this->m_data))
			== static_cast<const void*>(std::addressof(arg)))
			return *this;
		reset();
		m_type_index = index_of<T>::value;
		new (m_data) ValueT(std::forward<ArgT>(arg));
		return *this;
	}

	template<typename T, typename... ArgsT>
	variant& set (ArgsT&&... args)
	{
		using ValueT = std::remove_reference_t<T>;
		reset();
		m_type_index = index_of<T>::value;
		new (m_data) ValueT(std::forward<ArgsT>(args)...);
		return *this;
	}

	template<uint pos>
	typename type_at<pos>::type& at()
	{
		if (m_type_index != pos)  [[unlikely]]
		{
			if (m_type_index < 0)
				corecpp::throws<corecpp::bad_access>("valueless");
			int status;
			std::unique_ptr<char> name { abi::__cxa_demangle(typeid(std::decay_t<type_at_t<pos>>).name(), 0, 0, &status) };
			std::string res { name.get() };
			corecpp::throws<corecpp::bad_access>(corecpp::concat<std::string>({ res, " expected, got ", which() }));
		}
		return get<typename type_at<pos>::type>(m_data);
	}

	template<uint pos>
	const typename type_at<pos>::type& at() const
	{
		if (m_type_index != pos)  [[unlikely]]
		{
			if (m_type_index < 0)
				corecpp::throws<corecpp::bad_access>("valueless");
			int status;
			std::unique_ptr<char> name { abi::__cxa_demangle(typeid(std::decay_t<type_at_t<pos>>).name(), 0, 0, &status) };
			std::string res { name.get() };
			corecpp::throws<corecpp::bad_access>(corecpp::concat<std::string>({ res, " expected, got ", which() }));
		}
		return get<const typename type_at<pos>::type>(m_data);
	}

	void reset()
	{
		if ( m_type_index >= 0 )
		{
			visit([](auto& value) {
				using ValueT = std::remove_reference_t<decltype(value)>;
				value.~ValueT();
			});
			m_type_index = -1;
		}
	}

	template<class VisitorT, typename... ArgsT>
	auto visit(VisitorT&& visitor, ArgsT&&... args) const
	{
		if (m_type_index < 0)  [[unlikely]]
			corecpp::throws<corecpp::bad_access>("valueless");
		variant_apply<const variant<TArgs...>, sizeof...(TArgs) - 1, VisitorT, ArgsT...> applier;
		return applier(*this, std::forward<VisitorT>(visitor), std::forward<ArgsT>(args)...);
	}

	template<class VisitorT, typename... ArgsT>
	auto visit(VisitorT&& visitor, ArgsT&&... args)
	{
		if (m_type_index < 0)  [[unlikely]]
			corecpp::throws<corecpp::bad_access>("valueless");
		variant_apply<variant<TArgs...>, sizeof...(TArgs) - 1, VisitorT, ArgsT...> applier;
		return applier(*this, std::forward<VisitorT>(visitor), std::forward<ArgsT>(args)...);
	}
	constexpr bool valueless() const noexcept
	{
		return (m_type_index < 0);
	}
	/* for compatibility with STL
	 * not exclusively due to exception in my implementation, but the name of this method is just stupid
	 */
	constexpr bool valueless_by_exception() const noexcept
	{
		return valueless();
	}

	/* required for serialisation */
	template <typename SerializerT>
	void serialize(SerializerT& s) const
	{
		s.template begin_array<this_type>();
		s.write_element(m_type_index);
		visit([&s](auto&& value){ s.write_element(value); });
		s.end_array();
	}
	template <typename DeserializerT>
	void deserialize(DeserializerT& d, const std::string& property)
	{
		d.template begin_array<this_type>();
		d.read_element(m_type_index);
		visit([&d](auto&& value){ d.read_element(value); });
		d.end_array();
	}
};

template <class Visitor, class Variants>
constexpr auto visit(Visitor&& vis, Variants&& vars)
{
	return vars.visit(vis);
}

}

#endif
