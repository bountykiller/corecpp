#ifndef CORECPP_NET_URI_H
#define CORECPP_NET_URI_H

#include <cassert>

#include <string>
#include <string_view>
#include <tuple>

#include <corecpp/except.h>
#include <corecpp/meta/reflection.h>

namespace corecpp
{

namespace net
{
/* RFC 3986
 * URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
 */
class uri final
{
	std::string m_value;
	std::string_view m_scheme;
	std::string_view m_authority;
	std::string_view m_userinfo;
	std::string_view m_host;
	std::string_view m_port;
	std::string_view m_path;
	std::string_view m_query;
	std::string_view m_fragment;


	void parse();

public:
	static const auto& properties()
	{
		static auto result = std::make_tuple(
			corecpp::make_property("value", &uri::m_value)
		);
		return result;
	}

	uri()
	{
	}
	uri(const uri& other)
	: uri(other.m_value)
	{
		/* TODO : could be optimized to avoid reparsing the uri by copying the views and change their start pointer */
	}
	uri(uri&& other)
	: uri(std::move(other.m_value))
	{
		/* TODO : could be optimized to avoid reparsing the uri by copying the views and change their start pointer */
	}
	explicit uri(std::string s)
	: m_value { std::move(s) }
	{
		parse();
	}

	uri& operator = (const uri& other)
	{
		m_value = other.m_value;
		/* TODO : copying the views and change their start pointer */
		return *this;
	}
	uri& operator = (uri&& other)
	{
		m_value = std::move(other.m_value);
		/* TODO : copying the views and change their start pointer */
		return *this;
	}
	uri& operator = (std::string value)
	{
		m_value = std::move(value);
		parse();
		return *this;
	}

	const std::string& str() const
	{
		return m_value;
	}
	std::string_view scheme() const
	{
		return m_scheme;
	}
	std::string_view authority() const
	{
		return m_authority;
	}
	std::string_view path() const
	{
		return m_path;
	}
	std::string_view query() const
	{
		return m_query;
	}
	std::string_view fragment() const
	{
		return m_fragment;
	}
};

static inline const std::string& to_string(const uri& u)
{
	return u.str();
}

}

}

#endif
