#ifndef CORECPP_NET_MAILADDRESS_H
#define CORECPP_NET_MAILADDRESS_H

#include <string>
#include <tuple>

#include <corecpp/meta/reflection.h>

namespace corecpp
{

class mailaddress final
{
	std::string m_value;
	std::string_view m_local_part;
	std::string_view m_domain;


	void parse();

public:
	static const auto& properties()
	{
		static auto result = std::make_tuple(
			corecpp::make_property("value", &mailaddress::m_value)
		);
		return result;
	}

	mailaddress()
	{
		/* TODO */
	}
	mailaddress(const mailaddress& other)
	: mailaddress { other.m_value }
	{
		/* TODO : could be optimized to avoid reparsing the mailaddress by copying the views and change their start pointer */
	}
	mailaddress(mailaddress&& other)
	: mailaddress { std::move(other.m_value) }
	{
		/* TODO : could be optimized to avoid reparsing the mailaddress by copying the views and change their start pointer */
	}
	explicit mailaddress(std::string s)
	: m_value { std::move(s) }
	{
		parse();
	}

	mailaddress& operator = (const mailaddress& other)
	{
		m_value = other.m_value;
		/* TODO : copying the views and change their start pointer */
		return *this;
	}
	mailaddress& operator = (mailaddress&& other)
	{
		m_value = std::move(other.m_value);
		/* TODO : copying the views and change their start pointer */
		return *this;
	}
	mailaddress& operator = (std::string value)
	{
		m_value = std::move(value);
		/* TODO : parse the given string */
		return *this;
	}

	const std::string& str() const
	{
		return m_value;
	}
};

static inline const std::string& to_string(const mailaddress& m)
{
	return m.str();
}

}

#endif
