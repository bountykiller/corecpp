#ifndef CORE_CPP_REF_PTR_H
#define CORE_CPP_REF_PTR_H

#include <stdexcept>
#include <type_traits>

namespace corecpp
{

/**
 * @brief this class is intented to facilitate the use of an object which is already
 * owned somewhere else.
 * \warning since this class doesn't owns his data, a ref_ptr becomes invalid
 * as soon as the data it points to is deleted
 */
template<typename _T>
class ref_ptr final
{
public:
	using element_type = typename std::remove_reference<_T>::type;
	using reference_type = typename std::add_lvalue_reference<_T>::type;
	using pointer = typename std::add_pointer<_T>::type;
	using const_pointer = typename std::add_const<pointer>::type;

private:
	pointer m_ptr;

public:
	ref_ptr() = default;
	ref_ptr(const ref_ptr&) = default;
	explicit ref_ptr(const_pointer ptr)
	: m_ptr(ptr)
	{}
	template<typename _Tp>
	ref_ptr(const _Tp& ptr)
	: m_ptr(ptr.get())
	{}

	reference_type operator* () const
	{
#if DEBUG
		if(m_ptr)
			return *m_ptr;
		else
			throw std::logic_error("trying to dereference a null pointer");
#else
		return *m_ptr;
#endif
	}
	pointer operator ->() const
	{
		return m_ptr;
	}
	explicit operator bool() const
	{
		return m_ptr;
	}
	bool operator !()
	{
		return !m_ptr;
	}
	ref_ptr& operator =(const ref_ptr& ptr) = default;
	template<typename TPointer>
	ref_ptr& operator =(const TPointer& ptr)
	{
		m_ptr = ptr.get();
		return *this;
	}
	bool operator ==(const ref_ptr& ptr)
	{
		return m_ptr == ptr.m_ptr;
	}
	bool operator !=(const ref_ptr& ptr)
	{
		return m_ptr != ptr.m_ptr;
	}
	bool operator <(const ref_ptr& ptr)
	{
		return m_ptr < ptr.m_ptr;
	}
	pointer get() const
	{
		return m_ptr;
	}
};


template<typename _Tp>
ref_ptr<typename _Tp::element_type> take_ref(const _Tp& pointer)
{
	return ref_ptr<typename _Tp::element_type>(pointer);
}

template<typename _Rp, typename _Tp>
ref_ptr<_Rp> take_ref(const _Tp& pointer)
{
	return ref_ptr<_Rp>(pointer);
}

}


#endif
