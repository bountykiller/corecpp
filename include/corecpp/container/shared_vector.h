#ifndef CORE_CPP_CONTAINER_H
#define CORE_CPP_CONTAINER_H

#include <atomic>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <corecpp/meta/reflection.h>
#include <corecpp/ref_ptr.h>
#include <corecpp/container/iterator.h>

namespace corecpp
{

	/* TODO:
	 * void assign( size_type count, const T& value );
	 * template< class InputIt >
	 * void assign( InputIt first, InputIt last );
	 * void assign( std::initializer_list<T> ilist );
	 * rbegin + rend
	 * size_type max_size() const noexcept;
	 * overloads of insert
	 * overloads of erase
	 * template< class T, class Alloc, class U >
	 * void erase(std::vector<T,Alloc>& c, const U& value);
	 * template< class T, class Alloc, class Pred >
	 * void erase_if(std::vector<T,Alloc>& c, Pred pred);
	 */
template<typename T, class Allocator = std::allocator<T>>
class shared_vector
{
public:
	using value_type = T;
	using allocator_type = Allocator;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = typename std::allocator_traits<Allocator>::pointer;
	using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
	template<typename ContainerT, typename ValueT>
	using iterator_type = contiguous_iterator<ContainerT, ValueT>;
	using iterator = iterator_type<shared_vector<value_type, allocator_type>, value_type>;
	using const_iterator =  corecpp::const_iterator<iterator>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
private:
	struct block
	{
		Allocator m_allocator;
		size_type m_refcount;
		size_type m_reserved;
		size_type m_size;
		T *m_data;
		std::vector<int> a;
	};
	block* m_block;

	size_type next_power_of_2(size_type count)
	{
		count--;
		count |= count >> 1;
		count |= count >> 2;
		count |= count >> 4;
		count |= count >> 8;
		count |= count >> 16;
		if (sizeof(size_type) > 32)
			count |= count >> 32;
		count++;
	}
	void destroy()
	{
		if (m_block->m_refcount == 0)
		{
			clear();
			delete m_block;
		}
	}

public:
	/* CTORS */
	shared_vector()
	: shared_vector(Allocator())
	{}
	explicit shared_vector(const Allocator& allocator)
	: m_block(new block { allocator, 1, 0, 0 })
	{
	}
	explicit shared_vector(size_type count, const Allocator& allocator = Allocator())
	: m_block(new block { allocator, 1, next_power_of_2(count), count })
	{
		m_block->m_data = m_block->m_allocator.allocate(m_block->m_reserved);
		for (auto i = count; count; --count)
		{
			Allocator::construct(m_block->m_allocator, &m_block->m_data[i]);
		}
	}
	shared_vector(size_type count, const T& value, const Allocator& allocator = Allocator())
	: m_block(new block { allocator, 1, next_power_of_2(count), count })
	{
		size_t i = 0;
		try
		{
			m_block->m_data = m_block->m_allocator.allocate(m_block->m_reserved);
			for (; i < count; ++i)
				Allocator::construct(m_block->m_allocator, &m_block->m_data[i], value);
		}
		catch (...)
		{
			while (i > 0)
				Allocator::destroy(m_block->m_allocator, &m_block->m_data[--i]);

			m_block->m_allocator.deallocate(m_block->m_data, m_block->m_reserved);
			delete m_block;
			throw;
		}
	}
	template<typename InputIt>
	shared_vector(InputIt first, InputIt last, const Allocator& allocator = Allocator())
	: m_block(new block { allocator, 1, next_power_of_2(last - first), last - first })
	{
		size_t i = 0;
		try
		{
			m_block->m_data = m_block->m_allocator.allocate(m_block->m_reserved);
			for (; first < last; ++first, ++i)
				Allocator::construct(m_block->m_allocator, &m_block->m_data[i], *first);
		}
		catch (...)
		{
			while (i > 0)
				Allocator::destroy(m_block->m_allocator, &m_block->m_data[--i]);

			m_block->m_allocator.deallocate(m_block->m_data, m_block->m_reserved);
			delete m_block;
			throw;
		}
	}
	shared_vector(const shared_vector& other)
	: m_block(other.m_block)
	{
		m_block->m_refcount++;
	}
	shared_vector(shared_vector&& other)
	: m_block(other.m_block)
	{
		/* here if an exception is thrown,
		 * - the current shared_vector won't be destroyed
		 * - other will still be pointing to his ownn block*
		 * ==> all good!
		 */
		other.m_block = new block { m_block->allocator, 1, 0, 0 };
	}

	shared_vector(std::initializer_list<T> init, const Allocator& allocator = Allocator())
	: m_block(new block { allocator, 1, next_power_of_2(init.size()), init.size() })
	{
		size_t i = 0;
		try
		{
			m_block->m_data = m_block->m_allocator.allocate(m_block->m_reserved);
			for (; i < init.size(); ++i)
				Allocator::construct(m_block->m_allocator, &m_block->m_data[i], init[i]);
		}
		catch (...)
		{
			while (i > 0)
				Allocator::destroy(m_block->m_allocator, &m_block->m_data[--i]);

			m_block->m_allocator.deallocate(m_block->m_data, m_block->m_reserved);
			delete m_block;
			throw;
		}
	}
	/* DTOR */
	~shared_vector()
	{
		/* test m_block to avoid crash if the dtor is called twice */
		if (m_block)
		{
			destroy();
			m_block = nullptr;
		}
	}
	/* Operators
	 */
	shared_vector& operator = (const shared_vector& lvalue)
	{
		if (m_block != lvalue.m_block)
		{
			destroy();
			m_block = lvalue.m_block;
			m_block->m_refcount++;
		}
		return *this;
	}
	shared_vector& operator = (shared_vector&& rvalue) noexcept
	{
		swap(rvalue);
		return *this;
	}

	bool operator == (const shared_vector& value) const noexcept
	{
		return (m_block == value.m_block);
	}
	bool operator != (const shared_vector& value) const noexcept
	{
		return (m_block != value.m_block);
	}

	/* warning : unlike std::vector, the comparison here is not lexicographical
	 */
	bool operator < (const shared_vector& value) const noexcept
	{
		return (m_block < value.m_block);
	}
	bool operator <= (const shared_vector& value) const noexcept
	{
		return (m_block <= value.m_block);
	}
	bool operator > (const shared_vector& value) const noexcept
	{
		return (m_block > value.m_block);
	}
	bool operator >= (const shared_vector& value) const noexcept
	{
		return (m_block >= value.m_block);
	}

	reference operator [] (size_type index) noexcept
	{
		return m_block->m_data[index];
	}
	const_reference operator [] (size_type index) const noexcept
	{
		return m_block->m_data[index];
	}


	/* Properties
	 */
	size_type size() const noexcept
	{
		return m_block->m_size;
	}
	size_type capacity() const noexcept
	{
		return m_block->m_reserved;
	}
	bool empty() const noexcept
	{
		return (m_block->m_size == 0);
	}

	/* Modifiers
	 */
	void reserve(size_type size)
	{
		if (size <= capacity())
			return;
		size = next_power_of_2(size);
		T* new_data = m_block->m_allocator.allocate(size);
		for (size_type i = 0; i < m_block->m_size; ++i)
		{
			/* throwing here is UB */
			Allocator::construct(m_block->m_allocator, &new_data[i], std::move(m_block[i]));
			Allocator::destroy(m_block->m_allocator, m_block[i]);
		}
		m_block->m_allocator.deallocate(m_block->m_data, m_block->m_reserved);
		m_block->m_data = new_data;
		m_block->m_reserved = size;
	}

	template <typename... TArgs>
	void emplace_back(TArgs&&... args)
	{
		reserve(size() + 1);
		Allocator::construct(m_block->m_allocator, &m_block->m_data[size()], std::forward<TArgs>(args)...);
		m_block->m_size++;
	}

	void push_back(const T& value)
	{
		reserve(size() + 1);
		Allocator::construct(m_block->m_allocator, &m_block->m_data[size()], value);
		m_block->m_size++;
	}
	void push_back(T&& value)
	{
		reserve(size() + 1);
		Allocator::construct(m_block->m_allocator, &m_block->m_data[size()], value);
		m_block->m_size++;
	}
	void pop_back()
	{
		Allocator::destroy(m_block->m_allocator, &m_block->m_data[size()]);
		m_block->m_size--;
	}
	void clear()
	{
		while(m_block->m_size > 0)
			Allocator::destroy(m_block->m_allocator, &m_block->m_data[--m_block->m_size]);
		m_block->m_allocator.deallocate(m_block->m_data, m_block->m_reserved);
		m_block->m_data = nullptr;
		m_block->m_size = m_block->m_reserved = 0;
	}
	void swap(shared_vector& other) noexcept
	{
		if (m_block != other.m_block)
		{
			block *tmp = m_block;
			m_block = other.m_block;
			other.m_block = tmp;
		}
	}
	void resize(size_type new_size)
	{
		while(m_block->m_size > new_size)
			Allocator::destroy(m_block->m_allocator, &m_block->m_data[--m_block->m_size]);
		reserve(new_size);
		while(m_block->m_size < new_size)
			Allocator::construct(m_block->m_allocator, &m_block->m_data[m_block->m_size++]);
	}
	void shrink()
	{
		if (capacity() > size())
		{
			T* new_data = m_block->m_allocator.allocate(size());
			for (size_type i = 0; i < m_block->m_size; ++i)
			{
				/* throwing here is UB */
				Allocator::construct(m_block->m_allocator, &new_data[i], std::move(m_block[i]));
				Allocator::destroy(m_block->m_allocator, m_block[i]);
			}
			m_block->m_allocator.deallocate(m_block->m_data, m_block->m_reserved);
			m_block->m_data = new_data;
			m_block->m_reserved = size();
		}
	}

	/* accessors
	 */
	allocator_type& get_allocator() noexcept
	{
		return m_block->m_allocator;
	}
	const allocator_type& get_allocator() const noexcept
	{
		return m_block->m_allocator;
	}
	reference at(size_type index) const
	{
		if (index >= size())
			throw std::overflow_error(std::to_string(index));
		return m_block->m_reserved;
	}
	iterator begin() noexcept
	{
		return m_block->m_data;
	}
	const_iterator begin() const noexcept
	{
		return m_block->m_data;
	}
	const_iterator cbegin() const noexcept
	{
		return m_block->m_data;
	}

	iterator end() noexcept
	{
		return &m_block->m_data[size()];
	}
	const_iterator end() const noexcept
	{
		return &m_block->m_data[size()];
	}
	const_iterator cend() const noexcept
	{
		return &m_block->m_data[size()];
	}

	/* RMQ: Calling front on an empty container is undefined.  */
	reference front() noexcept
	{
		return m_block->m_data[0];
	}
	const_reference front() const noexcept
	{
		return m_block->m_data[0];
	}
	reference back() noexcept
	{
		return m_block->m_data[m_block->m_size-1];
	}
	const_reference back() const noexcept
	{
		return m_block->m_data[m_block->m_size-1];
	}
};

}

#endif
