#ifndef CORE_CPP_EVENT_H
#define CORE_CPP_EVENT_H

#include <vector>
#include <functional>
#include <type_traits>

#include <corecpp/ref_ptr.h>

namespace corecpp
{
	template<class>
	struct event;

	template<class ResultT, class... ArgsT>
	struct event<ResultT(ArgsT...)>
	{
		using func_type = std::function<ResultT(ArgsT...)>;
		using result_type = typename func_type::result_type;
	private:
		std::vector<func_type> m_observers;
	public:
		func_type& operator += (func_type func)
		{
			m_observers.emplace_back(func);
			return func;
		}
		func_type& operator -= (func_type func)
		{
			m_observers.remove(func);
			return func;
		}
		ref_ptr<func_type> rebind (func_type oldfunc, func_type newfunc)
		{
			for (auto& func : m_observers)
			{
				if (func == oldfunc)
				{
					func = newfunc;
					return &func;
				}
			}
			return nullptr;
		}
		//TODO: Perfect forwarding
		template<typename = typename std::enable_if<std::is_void<result_type>::value>::type>
		void operator()(ArgsT... args) const
		{
			for (auto func : m_observers)
				func(args...);
		}
		template<typename Result = result_type,
				typename = typename std::enable_if<!std::is_void<Result>::value>::type>
		std::vector<Result> operator()(ArgsT... args) const
		{
			std::vector<result_type> res;
			res.reserve(m_observers.size());
			for (auto func : m_observers)
				res.emplace_back(func(args...));
			return res;
		}
	};
}

#endif

