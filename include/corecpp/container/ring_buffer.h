#ifndef CORECPP_RINGBUFFER_H
#define CORECPP_RINGBUFFER_H

#include <memory>
#include <stdexcept>

namespace corecpp
{
	/*TODO:
	 * add overload for scalar types
	 * make this thread-safe using a locking_policy (no great interest oherwise)
	 */
	template <typename ValueT, typename AllocatorT = std::allocator<ValueT>>
	class ring_buffer
	{
		ValueT* m_buffer;
		std::size_t m_capacity;
		std::size_t m_first;
		std::size_t m_count; /* points to invalid data */
		AllocatorT m_allocator;

		// return the next to be written position
		std::size_t next()
		{
			auto pos = m_first + m_count;
			if (pos >= m_capacity)
				pos -= m_capacity;
			return pos;
		}

		std::size_t last()
		{
			auto pos = m_first + m_count - 1;
			if (pos >= m_capacity)
				pos -= m_capacity;
			return pos;
		}
	public:
		using value_type = ValueT;
		using allocator_type = AllocatorT;
		using reference_type = typename std::add_lvalue_reference<ValueT>::type;
		using const_reference_type = typename std::add_const<reference_type>::type;

		ring_buffer(std::size_t capacity)
		: ring_buffer(capacity, allocator_type())
		{
		}
		ring_buffer(std::size_t capacity, const allocator_type& allocator)
		: ring_buffer(capacity, allocator_type(allocator))
		{}
		ring_buffer(ring_buffer&& other)
		: m_buffer(other.m_buffer), m_capacity(other.m_capacity),
		m_first(other.m_first), m_count(other.m_count), m_allocator(std::move(other.m_allocator))
		{
			other.m_buffer = nullptr;
		}
		ring_buffer(std::size_t capacity, allocator_type&& allocator)
		: m_buffer(0), m_capacity(0), m_first(0), m_count(0), m_allocator(allocator)
		{
			m_buffer = allocator.allocate(m_capacity, &this[1]);
		}
		ring_buffer(const ring_buffer& other)
		: m_buffer(nullptr), m_capacity(other.m_capacity), m_first(other.m_first), m_count(other.m_count), m_allocator(other.allocator)
		{
			m_buffer = m_allocator.allocate(m_capacity, &this[1]);
			//TODO: copy elements
		}
		~ring_buffer()
		{
			if (m_buffer)
			{
				m_allocator.destroy(m_buffer, m_capacity);
				m_buffer = nullptr;
			}
		}
		std::size_t size() const
		{
			return m_count;
		}
		std::size_t capacity() const
		{
			return m_capacity;
		}
		bool empty() const
		{
			return (m_count == 0);
		}
		const_reference_type front() const
		{
			return m_buffer[m_first];
		}
		const_reference_type back() const
		{
			return m_buffer[last()];
		}
		template<typename ...ArgsT>
		bool try_emplace_back(ArgsT&&... args)
		{
			if (m_count >= m_capacity)
				return false;
			m_allocator.construct(&m_buffer[next()], std::forward<ArgsT>(args)...);
			++m_count;
			return true;
		}
		/* data could be overwritten
		 */
		template<typename ...ArgsT>
		void emplace_back(ArgsT&&... args)
		{
			if (m_count >= m_capacity)
				m_allocator.destroy(&m_buffer[next()]);
			m_allocator.construct(&m_buffer[next()], std::forward<ArgsT>(args)...);
			if (m_count < m_capacity)
				++m_count;
			else if(++m_first == m_capacity)
				m_first = 0;
		}
		bool try_push_back(const_reference_type other)
		{
			if (m_count >= m_capacity)
				return false;
			m_allocator.construct(&m_buffer[next()], other);
			++m_count;
			return true;
		}
		void push_back(const_reference_type other)
		{
			if (m_count >= m_capacity)
				m_allocator.destroy(&m_buffer[next()]);
			m_allocator.construct(&m_buffer[next()], other);
			if (m_count < m_capacity)
				++m_count;
			else if(++m_first == m_capacity)
				m_first = 0;
		}
		void pop_front()
		{
			if (m_count == 0)
				throw std::underflow_error("pop_front");
			m_allocator.destroy(&m_buffer[m_first]);
			if(++m_first == m_capacity)
				m_first = 0;
			--m_count;
		}
		void pop_back()
		{
			if (m_count == 0)
				throw std::underflow_error("pop_back");
			m_allocator.destroy(&m_buffer[last()]);
			--m_count;
		}
		//TODO : front_inserters
	};
}

#endif
