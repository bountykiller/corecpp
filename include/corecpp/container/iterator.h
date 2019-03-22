#ifndef CORE_CPP_ITERATOR_H
#define CORE_CPP_ITERATOR_H


namespace corecpp
{

template<typename IteratorT>
using const_iterator = typename IteratorT::template iterator_type<typename IteratorT::container_type, const typename IteratorT::value_type>;

template<typename ContainerT, typename ValueT>
class contiguous_iterator final
{
public:
	using iterator = contiguous_iterator<ContainerT, ValueT>;
	using container_type = ContainerT;
	using value_type = ValueT;
	using difference_type = std::ptrdiff_t;
	using reference = typename std::add_lvalue_reference<value_type>::type;
	using pointer = typename std::add_pointer<value_type>::type;

private:
	pointer m_value;

public:
	explicit contiguous_iterator(pointer value) noexcept
	: m_value(value)
	{}
	reference operator*() noexcept
	{
		return *m_value;
	}
	pointer operator->() noexcept
	{
		return m_value;
	}

	iterator& operator++() noexcept
	{
		++m_value;
		return *this;
	}
	iterator operator++(int) noexcept
	{
		return iterator(m_value++);
	}

	// Bidirectional iterator requirements
	iterator& operator--() noexcept
	{
		--m_value;
		return *this;
	}
	iterator operator--(int) noexcept
	{
		return iterator(m_value--);
	}

	// Random access iterator requirements
	reference operator[](difference_type pos) const noexcept
	{
		return m_value[pos];
	}

	iterator& operator+=(difference_type pos) noexcept
	{
		m_value += pos;
		return *this;
	}

	iterator operator+(difference_type pos) const noexcept
	{
		return iterator(m_value + pos);
	}

	iterator& operator-=(difference_type pos) noexcept
	{
		m_value -= pos;
		return *this;
	}

	iterator operator-(difference_type pos) const noexcept
	{
		return iterator(m_value - pos);
	}

	bool operator == (iterator &other)
	{
		return other.m_value == m_value;
	}
};

}
