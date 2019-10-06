#ifndef VARIANT_H
#define VARIANT_H

#include <type_traits>
#include <utility>
#include <cassert>
#include <corecpp/meta/reflection.h>
#include <corecpp/any.h>

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

/*
 * !\brief class intented to implement the variant concept
 * NOTE: While this class tend to use some of the stl vocabulary, it is not stl-compliant
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
		static constexpr uint value = corecpp::type_index<T, TArgs...>::value;
	};
	template<size_t index>
	struct type_at
	{
		using type = typename corecpp::type_at<index, TArgs...>::type;
	};

	variant() noexcept(std::is_nothrow_default_constructible<typename type_at<0>::type>::value)
	: m_type_index(0)
	{
		new(&m_data) typename type_at<0>::type;
	}
	variant(variant&& other) noexcept
	: m_type_index(other.m_type_index)
	{
		visit([&other](auto& value) {
			using ValueT = std::remove_reference_t<decltype(value)>;
			new (&value) ValueT(std::move(other.template get<ValueT>()));
		});
	}
	variant(const variant& other) noexcept(std::is_nothrow_copy_constructible<typename type_at<0>::type>::value)
	: m_type_index(other.m_type_index)
	{
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

	uint index() const
	{
		return m_type_index;
	}

	template<typename T>
	T& get()
	{
		assert(m_type_index == index_of<T>::value);
		return *(reinterpret_cast<T*>(m_data));
	}

	template<typename T>
	const T& get() const
	{
		assert(m_type_index == index_of<T>::value);
		return *(reinterpret_cast<const T*>(m_data));
	}

	template<uint pos>
	typename type_at<pos>::type& at()
	{
		assert(m_type_index == pos);
		return get<typename type_at<pos>::type>(m_data);
	}

	template<uint pos>
	const typename type_at<pos>::type& at() const
	{
		assert(m_type_index == pos);
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
		}
	}

	template<class VisitorT, typename... ArgsT>
	auto visit(VisitorT&& visitor, ArgsT&&... args) const
	{
		variant_apply<const variant<TArgs...>, sizeof...(TArgs) - 1, VisitorT, ArgsT...> applier;
		return applier(*this, std::forward<VisitorT>(visitor), std::forward<ArgsT>(args)...);
	}

	template<class VisitorT, typename... ArgsT>
	auto visit(VisitorT&& visitor, ArgsT&&... args)
	{
		variant_apply<variant<TArgs...>, sizeof...(TArgs) - 1, VisitorT, ArgsT...> applier;
		return applier(*this, std::forward<VisitorT>(visitor), std::forward<ArgsT>(args)...);
	}
	/* for compatibility with stl */
	constexpr bool valueless_by_exception() const noexcept
	{
		return false;
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
