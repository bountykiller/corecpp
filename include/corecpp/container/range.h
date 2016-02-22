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
};

}

#endif