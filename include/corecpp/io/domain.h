#ifndef CORE_CPP_DOMAIN_H
#define CORE_CPP_DOMAIN_H

extern "C" {
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/un.h>
	#include <linux/netlink.h>
}

/**
* @namespace corecpp::family
* @brief Namespace containing various domain structures for socket communication.
*/
namespace corecpp::family
{

/**
* @struct local
* @brief Represents the local domain for socket communication.
*
* This struct provides constants and types specific to the local (Unix domain) socket family.
*/
struct local
{
	static constexpr int domain = PF_LOCAL;
	using address_type = sockaddr_un;

	static socklen_t address_len(const address_type& addr) noexcept
	{
		return SUN_LEN(&addr);
	}
};

/**
* @struct inet
* @brief Represents the IPv4 domain for socket communication.
*/
struct inet
{
	static constexpr int domain = PF_INET;
	using address_type = sockaddr_in;

	static constexpr socklen_t address_len(const address_type& addr) noexcept
	{
		return sizeof(addr);
	}
};

/**
* @struct inet6
* @brief Represents the IPv6 domain for socket communication.
*/
struct inet6
{
	static constexpr int domain = PF_INET6;
	using address_type = sockaddr_in6;

	static constexpr socklen_t address_len(const address_type& addr) noexcept
	{
		return sizeof(addr);
	}
};

/**
* @struct netlink
* @brief Represents the NETLINK domain for socket communication.
*/
struct netlink
{
	static constexpr int domain = PF_NETLINK;
	using address_type = sockaddr_nl;

	static constexpr socklen_t address_len(const address_type& addr) noexcept
	{
		return sizeof(addr);
	}
};


}

#endif
