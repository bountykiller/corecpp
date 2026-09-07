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
};

/**
* @struct inet
* @brief Represents the IPv4 domain for socket communication.
*/
struct inet
{
	static constexpr int domain = PF_INET;
	using address_type = sockaddr_in;
};

/**
* @struct inet6
* @brief Represents the IPv6 domain for socket communication.
*/
struct inet6
{
	static constexpr int domain = PF_INET6;
	using address_type = sockaddr_in6;
};

/**
* @struct netlink
* @brief Represents the NETLINK domain for socket communication.
*/
struct netlink
{
	static constexpr int domain = PF_NETLINK;
	using address_type = sockaddr_nl;
};


}

#endif
