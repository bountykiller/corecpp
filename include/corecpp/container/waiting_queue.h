#ifndef CORECPP_WAITINGQUEUE_H
#define CORECPP_WAITINGQUEUE_H

#include <deque>
#include <mutex>
#include <utility>
#include <type_traits>
#include <condition_variable>

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
			template<typename ...ArgsT>
			waiting_queue(ArgsT&&... args)
			: m_container(std::forward<ArgsT>(args)...)
			{
			}
			bool empty() const
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				return m_container.empty();
			}
			auto size() const -> decltype(m_container.size())
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				return m_container.size();
			}
			void push(const value_type& value)
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_container.push_back(value);
				m_condition.notify_all();
			}
			template<typename ...ArgsT>
			void emplace(const ArgsT&&... args)
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_container.emplace_back(std::forward<ArgsT>(args)...);
				m_condition.notify_all();
			}
			void pop()
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_condition.wait(lock, [&] { return !m_container.empty(); });
				m_container.pop_front();
			}
			template<typename T, typename =
					typename std::enable_if<std::is_assignable<T, typename std::add_rvalue_reference<value_type>::type>::value>::type>
			bool pop(T& out)
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (m_container.empty())
					return false;
				out = std::move(m_container.front());
				m_container.pop_front();
				return true;
			}
			template<typename T, typename = typename std::enable_if<std::is_assignable<T, value_type>::value>::type>
			bool pop_try(T& out)
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (m_container.empty())
					return false;
				out = m_container.front();
				m_container.pop_front();
				return true;
			}
			template<typename T, class Rep, class Period,
					typename CopyCtrArgT = typename std::add_rvalue_reference<value_type>::type,
					typename  = typename std::enable_if<std::is_assignable<T, CopyCtrArgT>::value>::type>
			bool pop_for(T& out, const std::chrono::duration<Rep, Period>& rel_time)
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (!m_condition.wait_for(lock, rel_time, [&] { return !m_container.empty(); }))
					return false;
				out = std::move(m_container.front());
				m_container.pop_front();
				return true;
			}
			template<typename T, class Rep, class Period,
					typename = typename std::enable_if<std::is_assignable<T, value_type>::value>::type>
			bool pop_for(T& out, const std::chrono::duration<Rep, Period>& rel_time)
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (!m_condition.wait_for(lock, rel_time, [&] { return !m_container.empty(); }))
					return false;
				out = m_container.front();
				m_container.pop_front();
				return true;
			}
			template<typename T, class Rep, class Period,
					typename CopyCtrArgT = typename std::add_rvalue_reference<value_type>::type,
					typename  = typename std::enable_if<std::is_assignable<T, CopyCtrArgT>::value>::type>
			bool pop_until(T& out, const std::chrono::duration<Rep, Period>& rel_time)
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (!m_condition.wait_until(lock, rel_time, [&] { return !m_container.empty(); }))
					return false;
				out = std::move(m_container.front());
				m_container.pop_front();
				return true;
			}
			template<typename T, class Rep, class Period,
					typename = typename std::enable_if<std::is_assignable<T, value_type>::value>::type>
			bool pop_until(T& out, const std::chrono::duration<Rep, Period>& rel_time)
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				if (!m_condition.wait_until(lock, rel_time, [&] { return !m_container.empty(); }))
					return false;
				out = m_container.front();
				m_container.pop_front();
				return true;
			}
	};
}

#endif
