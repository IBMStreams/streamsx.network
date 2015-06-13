/*
#######################################################################
# Copyright (C)2015, International Business Machines Corporation and
# others. All Rights Reserved.
#######################################################################
*/

#ifndef _NETWORK_IPV4_FUNCTIONS_H_
#define _NETWORK_IPV4_FUNCTIONS_H_

// Define SPL types and functions
#include "SPL/Runtime/Function/SPLFunctions.h"
#include <arpa/inet.h>
#include <netinet/in.h>

#define htonll(x)
#define IS_EQUAL_TO(a,b) \
			((a[0] == b[0]) \
			&& (a[1] == b[1]) \
			&& (a[2] == b[2]) \
			&& (a[3] == b[3]))

#define	IS_LESS_THAN(a,b) \
			((ntohl(a[0]) < ntohl(b[0])) \
			|| (ntohl(a[0]) == ntohl(b[0]) && ntohl(a[1]) < ntohl(b[1])) \
			|| (ntohl(a[0]) == ntohl(b[0]) && ntohl(a[1]) == ntohl(b[1]) && ntohl(a[2]) < ntohl(b[2])) \
			|| (ntohl(a[0]) == ntohl(b[0]) && ntohl(a[1]) == ntohl(b[1]) && ntohl(a[2]) == ntohl(b[2]) && ntohl(a[3]) < ntohl(b[3])))

#define	IS_GREATER_THAN(a,b) \
			((ntohl(a[0]) > ntohl(b[0])) \
			|| (ntohl(a[0]) == ntohl(b[0]) && ntohl(a[1]) > ntohl(b[1])) \
			|| (ntohl(a[0]) == ntohl(b[0]) && ntohl(a[1]) == ntohl(b[1]) && ntohl(a[2]) > ntohl(b[2])) \
			|| (ntohl(a[0]) == ntohl(b[0]) && ntohl(a[1]) == ntohl(b[1]) && ntohl(a[2]) == ntohl(b[2]) && ntohl(a[3]) > ntohl(b[3])))

#define IS_IN_RANGE(startIP,endIP,ip) \
			!IS_GREATER_THAN(startIP, endIP) && (IS_EQUAL_TO(ip, startIP) || (IS_GREATER_THAN(ip, startIP) && IS_LESS_THAN(ip, endIP)));

namespace com {
namespace ibm {
namespace streamsx {
namespace networkanalysis {
namespace ipv6 {
	inline SPL::boolean isIPv6Address(SPL::rstring const & value)
	{
		sockaddr_in6 sa;
		return (inet_pton(AF_INET6, value.c_str(), &(sa.sin6_addr)) == 1) ? true : false ;
	}

	inline SPL::rstring compactIPv6(SPL::rstring const & value)
	{
		uint32_t ip[4];
		char compactStr[INET6_ADDRSTRLEN];

		if(inet_pton(AF_INET6, value.c_str(), ip) == 0) return value;

		inet_ntop(AF_INET6, ip, compactStr, INET6_ADDRSTRLEN);

		return SPL::rstring(compactStr);
	}

	inline SPL::rstring expandIPv6(SPL::rstring const & value)
	{
		uint8_t ip[16];

		if(inet_pton(AF_INET6, value.c_str(), ip) == 0) return value;

		char expandedStr[INET6_ADDRSTRLEN];
		sprintf(expandedStr, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                (int)ip[0], (int)ip[1],
                (int)ip[2], (int)ip[3],
                (int)ip[4], (int)ip[5],
                (int)ip[6], (int)ip[7],
                (int)ip[8], (int)ip[9],
                (int)ip[10], (int)ip[11],
                (int)ip[12], (int)ip[13],
                (int)ip[14], (int)ip[15]);

		SPL::rstring expandedIP(expandedStr);
		return expandedIP;
	}

	inline SPL::boolean isIPv6CIDRNotation(SPL::rstring const & networkCIDR)
	{
		SPL::list<SPL::rstring> tokens = SPL::Functions::String::tokenize(networkCIDR, "/", false);

		if(tokens.size() != 2) return false;

		// check if valid IPv6 Address
		uint32_t ip1[4];
		if(inet_pton(AF_INET6, tokens[0].c_str(), ip1) == 0) return false;

		// check if prefix is between 0 and 128
		SPL::int32 prefix = SPL::Functions::Utility::strtoll(tokens[1], 10);
		if(prefix > 128 || prefix < 0) return false;

		return true;
	}

	inline SPL::boolean isEqualTo(SPL::rstring const & value1, SPL::rstring const & value2)
	{
		uint32_t ip1[4], ip2[4];

		if(inet_pton(AF_INET6, value1.c_str(), ip1) == 0) return false;
		if(inet_pton(AF_INET6, value2.c_str(), ip2) == 0) return false;

		return IS_EQUAL_TO(ip1, ip2);
	}

	inline SPL::boolean isLinkLocal(SPL::rstring const & value)
	{
		uint32_t ip[4];
		if(inet_pton(AF_INET6, value.c_str(), ip) == 1)
		{
			return (ip[0] & htonl(0xffc00000)) == htonl(0xfe800000);
		}

		return false;
	}

	inline SPL::boolean isSiteLocal(SPL::rstring const & value)
	{
		uint32_t ip[4];
		if(inet_pton(AF_INET6, value.c_str(), ip) == 1)
		{
			return (ip[0] & htonl(0xffc00000)) == htonl(0xfec00000);
		}

		return false;
	}

	inline SPL::boolean isLoopback(SPL::rstring const & value)
	{
		uint32_t ip[4];
		if(inet_pton(AF_INET6, value.c_str(), ip) == 1)
		{
			return ip[0] == 0
					&& ip[1] == 0
					&& ip[2] == 0
					&& ip[3] == htonl(1);
		}

		return false;
	}

	inline SPL::boolean isMulticast(SPL::rstring const & value)
	{
		uint32_t ip[4];
		if(inet_pton(AF_INET6, value.c_str(), ip) == 1)
		{
			return (ip[0] & htonl(0xff000000)) == htonl(0xff000000);
		}

		return false;
	}

	inline SPL::boolean isUnspecified(SPL::rstring const & value)
	{
		uint32_t ip[4];
		if(inet_pton(AF_INET6, value.c_str(), ip) == 1)
		{
			return ip[0] == 0
					&& ip[1] == 0
					&& ip[2] == 0
					&& ip[3] == 0;
		}

		return false;
	}

	inline SPL::boolean isV4Mapped(SPL::rstring const & value)
	{
		uint32_t ip[4];
		if(inet_pton(AF_INET6, value.c_str(), ip) == 1)
		{
			return ip[0] == 0
					&& ip[1] == 0
					&& ip[2] == htonl(0xffff);
		}

		return false;
	}

	inline SPL::boolean isV4Compatible(SPL::rstring const & value)
	{
		uint32_t ip[4];
		if(inet_pton(AF_INET6, value.c_str(), ip) == 1)
		{
			return ip[0] == 0
					&& ip[1] == 0
					&& ip[2] == 0
					&& ntohl(ip[3]) > 1;
		}

		return false;
	}

	inline SPL::boolean isGreaterThan(SPL::rstring const & value1, SPL::rstring const & value2)
	{
		uint32_t ip1[4], ip2[4];

		if(inet_pton(AF_INET6, value1.c_str(), ip1) == 0) return false;
		if(inet_pton(AF_INET6, value2.c_str(), ip2) == 0) return false;

		// check if startIP is larger than end IP
		return IS_GREATER_THAN(ip1, ip2);
	}

	inline SPL::boolean isLessThan(SPL::rstring const & value1, SPL::rstring const & value2)
	{
		uint32_t ip1[4], ip2[4];

		if(inet_pton(AF_INET6, value1.c_str(), ip1) == 0) return false;
		if(inet_pton(AF_INET6, value2.c_str(), ip2) == 0) return false;

		// check if startIP is larger than end IP
		return IS_LESS_THAN(ip1, ip2);
	}

	inline SPL::boolean isInIPRange(SPL::rstring const &  ipStart, SPL::rstring const &  ipEnd, SPL::rstring const &  ip)
	{
		uint32_t decStartIP[4], decEndIP[4], decIP[4];

		if(inet_pton(AF_INET6, ipStart.c_str(), decStartIP) == 0) return false;
		if(inet_pton(AF_INET6, ipEnd.c_str(), decEndIP) == 0) return false;
		if(inet_pton(AF_INET6, ip.c_str(), decIP) == 0) return false;

		// check if ip is larger than startIP
		return IS_IN_RANGE(decStartIP, decEndIP, decIP);
	}

	inline SPL::boolean isInNetwork(SPL::rstring const & networkCIDR, SPL::rstring const & ip)
	{
		SPL::list<SPL::rstring> tokens = SPL::Functions::String::tokenize(networkCIDR, "/", false);
		SPL::rstring networkIP = tokens[0];
		SPL::int32 prefix = SPL::Functions::Utility::strtoll(tokens[1], 10);

		uint32_t decNetworkIP[4], decIP[4];
		if(inet_pton(AF_INET6, networkIP.c_str(), decNetworkIP) == 0) return false;
		if(inet_pton(AF_INET6, ip.c_str(), decIP) == 0) return false;

		uint32_t netStart[4];
		uint32_t netEnd[4];

		memcpy(&netStart, decNetworkIP, 4 * sizeof(uint32_t));
		memcpy(&netEnd, decNetworkIP, 4 * sizeof(uint32_t));

		SPL::int32 diff = 128 - prefix;
		for(int i = 3; i >= 0; i--)
		{
			if(diff >= 32)
			{
				netStart[i] = 0;
				netEnd[i] = 0xFFFFFFFF;
				diff -= 32;
			}
			else if(diff > 0)
			{
				uint32_t sMask = ntohl(0xFFFFFFFF << diff);
				uint32_t eMask = ntohl(0xFFFFFFFF >> (32-diff));

				netStart[i] &= sMask;
				netEnd[i] |= eMask;

				diff = 0;
			}

			if(diff == 0) break;
		}

		return IS_IN_RANGE(netStart, netEnd, decIP);
	}

	inline SPL::boolean isInNetworkList(SPL::list<SPL::rstring> const & networkList, SPL::rstring const & ip)
	{
		SPL::list<SPL::rstring>::const_iterator it;
		for(it = networkList.begin(); it != networkList.end(); ++it)
		{
			if(isInNetwork(*it, ip)) return true;
		}

		return false;
	}
}
}
}
}
}

#endif  /* _NETWORK_IPV4_FUNCTIONS_H_ */
