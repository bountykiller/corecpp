#ifndef CORECPP_WAITINGQUEUE_H
#define CORECPP_WAITINGQUEUE_H

#include <deque>
#include <mutex>
#include <utility>
#include <type_traits>

namespace corecpp
{
	template <typename ValueT, typename ContainerT = std::deque<ValueT> >
	class waiting_queue
	{
		ContainerT m_container;
		mutable std::mutex m_mutex;
		mutable std::condition_variable m_condition;
		public:
			using value_type = std::remove_reference<ValueT>;
			template<typename ArgsT...>
			waiting_queue(ArgsT&& args)
			: m_container(std::forward<ArgsT>(args)...)
			{
			}
			auto empty() const
			{
				std::lock_guard lock(m_mutex);
				return m_container.empty();
			}
			auto size() const
			{
				std::lock_guard lock(m_mutex);
				return m_container.size();
			}
			void push(const value_type& value)
			{
				std::lock_guard lock(m_mutex);
				m_container.push_back(value);
				m_condition.notify_all();
			}
			template<typename ArgsT...>
			void emplace(const ArgsT&& args)
			{
				std::lock_guard lock(m_mutex);
				m_container.emplace_back(std::forward<ArgsT>(args)...);
				m_condition.notify_all();
			}
			value_type pop()
			{
				std::unique_lock lock(m_mutex);
				m_condition.wait(lock, [&m_container] { return !m_container.empty(); });
				value_type res = m_container.front();
				m_container.pop_front();
				return res;
			}
			template<typename T, typename = std::enable_if<std::is_assignable<T, std::add_rvalue_reference_t<value_type>>::value>::type>
			bool try_pop(T& out)
			{
				std::unique_lock lock(m_mutex);
				if (m_container.empty())
					return false;
				out = std::move(m_container.front());
				m_container.pop_front();
				return true;
			}
			template<typename T, typename = std::enable_if<std::is_assignable<T, value_type>::value>::type>
			bool try_pop(T& out)
			{
				std::unique_lock lock(m_mutex);
				if (m_container.empty())
					return false;
				out = m_container.front();
				m_container.pop_front();
				return true;
			}
			template<typename T, typename = std::enable_if<std::is_assignable<T, std::add_rvalue_reference_t<value_type>>::value>::type,
					class Rep, class Period>
			bool try_pop_for(T& out, const std::chrono::duration<Rep, Period>& rel_time)
			{
				std::unique_lock lock(m_mutex);
				if (!m_condition.wait_for(lock, rel_time, [&m_container] { return !m_container.empty(); }))
					return false;
				out = std::move(m_container.front());
				m_container.pop_front();
				return true;
			}
			template<typename T, typename = std::enable_if<std::is_assignable<T, value_type>::value>::type,
					class Rep, class Period>
			bool try_pop_for(T& out, const std::chrono::duration<Rep, Period>& rel_time)
			{
				std::unique_lock lock(m_mutex);
				if (!m_condition.wait_for(lock, rel_time, [&m_container] { return !m_container.empty(); }))
					return false;
				out = m_container.front();
				m_container.pop_front();
				return true;
			}
	};
}

#endif
