#ifndef EXPERIMENTALPP_REF_PTR_H
#define EXPERIMENTALPP_REF_PTR_H

#include <stdexcept>
#include <type_traits>

namespace experimental 
{

template<typename T>
class ref_ptr final
{
public:
	typedef typename std::remove_reference<T>::type element_type;
	typedef typename std::add_lvalue_reference<T>::type reference_type;
	typedef typename std::add_pointer<T>::type pointer;

	ref_ptr() = default;
	ref_ptr(const ref_ptr&) = default;

	template<typename Tp>
	ref_ptr(const Tp& ptr)
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
	template<typename Tp>
	ref_ptr& operator =(const Tp& ptr)
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


template<typename Tp>
ref_ptr<typename Tp::element_type> take_ref(const Tp& pointer)
{
	return ref_ptr<typename Tp::element_type>(pointer);
}

template<typename Rp, typename Tp>
ref_ptr<Rp> take_ref(const Tp& pointer)
{
	return ref_ptr<Rp>(pointer);
}

}


#endif
