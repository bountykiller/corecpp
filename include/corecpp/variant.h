#ifndef VARIANT_H
#define VARIANT_H

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
			if (v.which() != pos)
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
	template<size_t index>
	struct type_at
	{
		using type = typename corecpp::type_at<index, TArgs...>::type;
	};

	variant(const variant&) = delete;
	variant(variant&& other)
	: m_data(std::move(other.m_data)), m_type_index(other.m_type_index)
	{
	}

	template<typename T, typename RealT = typename std::remove_reference<T>::type>
	variant(T&& data)
	: m_data(std::forward<T>(data)), m_type_index(index_of<RealT>::value)
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

	bool operator < (const variant& other) const
	{
		if (m_type_index != other.m_type_index)
			return m_type_index < other.m_type_index;
		return m_data < other.m_data;
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

	template<uint pos>
	typename type_at<pos>::type& at()
	{
		assert(m_type_index == pos);
		return m_data.get<typename type_at<pos>::type>();
	}

	template<uint pos>
	const typename type_at<pos>::type& at() const
	{
		assert(m_type_index == pos);
		return m_data.get<typename type_at<pos>::type>();
	}

	template<class VisitorT, typename... ArgsT>
	auto apply(VisitorT&& visitor, ArgsT&&... args) const
	{
		variant_apply<const variant<TArgs...>, sizeof...(TArgs) - 1, VisitorT, ArgsT...> applier;
		return applier(*this, std::forward<VisitorT>(visitor), std::forward<ArgsT>(args)...);
	}

	template<class VisitorT, typename... ArgsT>
	auto apply(VisitorT&& visitor, ArgsT&&... args)
	{
		variant_apply<variant<TArgs...>, sizeof...(TArgs) - 1, VisitorT, ArgsT...> applier;
		return applier(*this, std::forward<VisitorT>(visitor), std::forward<ArgsT>(args)...);
	}

};

}

#endif
