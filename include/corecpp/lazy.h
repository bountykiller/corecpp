#ifndef CORECPP_LAZY_H
#define CORECPP_LAZY_H

#include <functional>
#include <corecpp/variant.h>

namespace corecpp
{

	template <typename T>
	struct lazy final
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
		explicit lazy(const func_type& func)
		: m_function(func), m_data(nullptr)
		{}
		lazy(lazy&&) = default;
		value_type& operator*()
		{
			if(m_data.which() == 0)
				evaluate();
			return m_data.get<value_type>();
		}
		value_type* operator->()
		{
			if(m_data.which() == 0)
				evaluate();
			return &m_data.get<value_type>();
		}
		void reset(void)
		{
			m_data = nullptr;
		}
	};

}

#endif
