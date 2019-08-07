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

template<typename... TArgs>
class variant
{
	using deleter_type = std::function<void(unsigned char*)>;
	unsigned char* m_data;
	deleter_type m_deleter;
	uint m_type_index;

	template<typename T>
	static deleter_type make_deleter()
	{
		return [](void *ptr) { std::default_delete<T>()(static_cast<T*>(ptr)); };
	}
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

	variant(const variant& other) noexcept
	: m_data(other.m_data), m_deleter(other.m_deleter), m_type_index(other.m_type_index)
	{
	}
	variant(variant&& other) noexcept
	: m_data(other.m_data), m_deleter(other.m_deleter), m_type_index(other.m_type_index)
	{
		other.m_data = nullptr;
	}

	template<typename T>
	variant(T&& data)
	: m_data(reinterpret_cast<unsigned char*>(new typename std::remove_reference<T>::type { std::forward<T>(data) } )),
	m_deleter(make_deleter<typename std::remove_reference<T>::type>()), m_type_index(index_of<typename std::remove_reference<T>::type>::value)
	{
	}

	variant& operator = (const variant& data) = delete;
	variant& operator = (variant&& other)
	{
		if(this == std::addressof(other))
			return *this;
		m_data = other.m_data;
		other.m_data = nullptr;
		m_deleter = other.m_deleter;
		m_type_index = other.m_type_index;
		return *this;
	}

	template<typename T>
	variant& operator = (T&& data)
	{
		m_data = reinterpret_cast<unsigned char*>(new typename std::remove_reference<T>::type { std::forward<T>(data) } );
		m_type_index = index_of<T>::value;
		return *this;
	}

	template<typename T>
	variant& operator = (const T& data)
	{
		m_data = new T { data };
		m_type_index = index_of<T>::value;
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
		return reinterpret_cast<T&>(m_data);
	}

	template<typename T>
	const T& get() const
	{
		assert(m_type_index == index_of<T>::value);
		return reinterpret_cast<const T&>(m_data);
	}

	template<uint pos>
	typename type_at<pos>::type& at()
	{
		assert(m_type_index == pos);
		return reinterpret_cast<typename type_at<pos>::type&>(m_data);
	}

	template<uint pos>
	const typename type_at<pos>::type& at() const
	{
		assert(m_type_index == pos);
		return reinterpret_cast<const typename type_at<pos>::type&>(m_data);
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

};

}

#endif
