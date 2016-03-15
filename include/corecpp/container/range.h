#ifndef CORE_CPP_RANGE_H
#define CORE_CPP_RANGE_H

#include <utility>

namespace corecpp
{

template <typename IteratorT>
class range_from_iterator
{
	IteratorT m_begin;
	IteratorT m_end;
public:
	using value_type = typename IteratorT::value_type;
	template<typename ArgsT> 
	range_from_iterator(ArgsT&& begin, ArgsT&& end)
	: m_begin(std::forward<ArgsT>(begin)), m_end(std::forward<ArgsT>(end))
	{}
	auto begin() const
	{
		return m_begin;
	}
	auto end() const
	{
		return m_end;
	}
	auto begin()
	{
		return m_begin;
	}
	auto end()
	{
		return m_end;
	}
	auto size() const
	{
		return m_end - m_begin;
	}
	bool empty() const
	{
		return m_begin == m_end;
	}
};

}

#endif