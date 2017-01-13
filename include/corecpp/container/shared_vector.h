#ifndef CORE_CPP_CONTAINER_H
#define CORE_CPP_CONTAINER_H

#include <atomic>
#include <memory>
#include <utility>
#include <corecpp/meta/reflection.h>
#include <corecpp/ref_ptr.h>

namespace corecpp
{

namespace
{

template<typename T>
class shared_vector_iterator
{
	corecpp::ref_ptr<T> m_pos;
public:
	explicit shared_vector_iterator(corecpp::ref_ptr<T> position)
	: m_pos(position)
	{}
};

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
	using iterator = shared_vector_iterator<T>;
	using const_iterator = const shared_vector_iterator<T>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
private:
	struct block {
		std::size_t m_size;
		std::size_t m_reserved;
		T m_data[];
		block(std::size_t size = 0, size_t reserved = 0)
		: m_size(size), m_reserved(reserved)
		{}
		block(const block&) = delete;
		block(const block&&) = delete;
	} ;
	std::shared_ptr<block> m_data;
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
public:
	/* CTORS */
	shared_vector() : shared_vector(Allocator())
	{}
	explicit shared_vector(const Allocator& alloc)
	: m_data(std::allocate_shared<block>(alloc))
	{
	}
	explicit shared_vector(size_type count, const Allocator& alloc = Allocator())
	{
		std::allocator allocator;
		allocator.allocate<T>(m_reserved);
		for (auto i = count; count; --count)
		{
			m_data[i].T();
		}
	}
	shared_vector(size_type count, const T& value, const Allocator& alloc = Allocator())
	: shared_vector(alloc), m_data(), m_refcount(1), m_size(count), m_reserved(next_power_of_2(count))
	{
		m_data = m_allocator.allocate(m_reserved);
		for (auto i = count; count; --count)
		{
			//::new (m_data + m_size) T(value);
			m_data[i].T(value);
		}
	}
	template<typename RandomAccessIt, typename = RandomAccessIt::difference_type>
	shared_vector(RandomAccessIt first, RandomAccessIt last, const Allocator& alloc = Allocator())
	: shared_vector(alloc), m_data(), m_refcount(1), m_size(0), m_reserved(next_power_of_2(last - first))
	{
		m_data = m_allocator.allocate(m_reserved);
		for (;first < last; ++first)
		{
			//::new (m_data + (++m_size)) T(*first);
			m_data[++m_size].T(*first);
		}
	}
	template<typename InputIt>
	shared_vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
	: shared_vector(alloc), m_data(), m_refcount(1), m_size(0), m_reserved(8)
	{
		m_data = m_allocator.allocate(m_reserved);
		for (;first < last; ++first)
		{
			emplace_back(*first);
		}
	}
	shared_vector(const shared_vector& other)
	: m_allocator(other.alloc), m_data(other.data), m_refcount(1), m_size(0), m_reserved(8)
	{}
	shared_vector(const shared_vector& other, const Allocator& alloc);
	shared_vector(shared_vector&& other);
	shared_vector(shared_vector&& other, const Allocator& alloc);
	shared_vector(std::initializer_list<T> init, const Allocator& alloc = Allocator());
	/* DTOR */
	~shared_vector()
	{
		/* m_data is used here in cases this dtor is called twice (could happen with some compilers in complex situations) */
		if (m_data)
		{
			if (--m_refcount == 0)
			{
				m_allocator.deallocate(m_data, m_reserved);
			}
			m_data = nullptr_t;
		}
	}
	template<typename... TArgs>
	void emplace_back(TArgs&&... args)
	{
		reserve(m_size + 1);
		m_data[m_size].T(std::forward<TArgs>(args)...);
	}
	void reserve(std::size_t size)
	{
		if (size < m_reserved)
			return;
		throw std::runtime_error("UNIMPLEMENTED");
	}
};

}

#endif
