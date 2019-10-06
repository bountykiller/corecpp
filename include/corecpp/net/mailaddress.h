#ifndef CORECPP_NET_MAILADDRESS_H
#define CORECPP_NET_MAILADDRESS_H

#include <string>
#include <tuple>

#include <corecpp/meta/reflection.h>

namespace corecpp
{

namespace net
{

class mailaddress final
{
	std::string m_value;
public:
	static const auto& properties()
	{
		static auto result = std::make_tuple(
			corecpp::make_property("value", &mailaddress::m_value)
		);
		return result;
	}

	explicit mailaddress(const std::string& s)
	: m_value(s)
	{
		/* TODO : parse the given string */
	}
	explicit mailaddress(const mailaddress& other)
	: mailaddress(other.m_value)
	{
		/* TODO : could be optimized to avoid reparsing the mailaddress by copying the views and change their start pointer */
	}
	const std::string& str() const
	{
		return m_value;
	}
};

}

}

#endif
