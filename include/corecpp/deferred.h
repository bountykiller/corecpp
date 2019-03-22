#ifndef CORECPP_DEFERRED_H
#define CORECPP_DEFERRED_H

#include <functional>
#include <type_traits>
#include <corecpp/variant.h>

namespace corecpp
{
	/* TODO: Add a template parameter (a locking_policy)
	 * to allow to embded an internal mutex if a lock must be held when evaluating the function
	 */
	template <typename T>
	struct deferred final
	{
		using value_type = T;
		using func_type = std::function<value_type(void)>;
	private:
		func_type m_function;
		corecpp::variant<std::nullptr_t, value_type> m_data;
		void evaluate(void)
		{
			m_data = m_function();
		}
	public:
		explicit deferred(const func_type& func)
		: m_function(func), m_data(nullptr)
		{}
		deferred(deferred&&) = default;
		value_type& operator*()
		{
			if(m_data.which() == 0)
				evaluate();
			return m_data.template get<value_type>();
		}
		value_type* operator->()
		{
			if(m_data.which() == 0)
				evaluate();
			return &m_data.template get<value_type>();
		}
		void reset(void)
		{
			m_data = nullptr;
		}
		operator bool() const
		{
			return (m_data.which() == 0);
		}
	};

	template <typename FuncT>
	auto defer(const FuncT& func)
	{
#if __cplusplus >= 201703L
		using result_type = typename std::invoke_result<FuncT>::type;
#else
		using result_type = typename std::result_of<FuncT>::type;
#endif
		return deferred<result_type>(func);
	}
}

#endif
