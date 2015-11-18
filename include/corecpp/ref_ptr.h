#ifndef CORE_CPP_REF_PTR_H
#define CORE_CPP_REF_PTR_H

#include <stdexcept>
#include <type_traits>

namespace corecpp
{

template<typename _T>
class ref_ptr final
{
public:
	using element_type = typename std::remove_reference<_T>::type;
	using reference_type = typename std::add_lvalue_reference<_T>::type;
	using pointer = typename std::add_pointer<_T>::type;

	ref_ptr() = default;
	ref_ptr(const ref_ptr&) = default;

	template<typename _Tp>
	ref_ptr(const _Tp& ptr)
	: m_ptr(ptr.get())
	{}

	reference_type operator* () const
	{
		if(m_ptr)
			return *m_ptr;
		else
			throw std::logic_error("trying to dereference a null pointer");
	}
	pointer operator ->() const
	{
		return m_ptr;
	}
	explicit operator bool() const
	{
		return m_ptr;
	}
	ref_ptr& operator =(const ref_ptr& ptr) = default;
	template<typename TPointer>
	ref_ptr& operator =(const TPointer& ptr)
	{
		m_ptr = ptr.get();
		return *this;
	}
	pointer get() const
	{
		return m_ptr;
	}
private:
	pointer m_ptr;
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
