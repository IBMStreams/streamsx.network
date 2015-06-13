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

struct dec_network_range {
	uint32_t network_start;
	uint32_t network_end;
};

struct network_cidr {
	uint32_t ip;
	uint32_t prefix;
};

#define GET_DEC_NET_RANGE(cidr, range) { \
	range.network_start = cidr.ip; \
	range.network_end = range.network_start + (uint32_t)(1 << (32-cidr.prefix)); \
}

namespace com {
namespace ibm {
namespace streamsx {
namespace networkanalysis {
namespace ipv4 {
	/*
	 * Internal use only. Splits a networkCIDR string into IP and prefix components
	 */
	inline SPL::boolean _parseNetworkCIDR(SPL::rstring const & networkCIDR, network_cidr &resultCIDR)
	{
		SPL::list<SPL::rstring> tokens = SPL::Functions::String::tokenize(networkCIDR, "/", false);
		if(tokens.size() != 2) return false;

		if (inet_pton(AF_INET, tokens[0].c_str(), &(resultCIDR.ip)) == 0) return false;
		resultCIDR.ip = ntohl(resultCIDR.ip);

		resultCIDR.prefix = SPL::Functions::Utility::strtoll(tokens[1], 10);
		if(resultCIDR.prefix > 32) return false;

		return true;
	}

	inline SPL::boolean isIPv4Address(SPL::rstring const & value)
	{
		in_addr decIP;
		return (inet_pton(AF_INET, value.c_str(), &decIP) == 1) ? true : false ;
	}

	inline SPL::boolean isInNetwork(SPL::rstring const & networkCIDR, SPL::rstring const & ip)
	{
		network_cidr resultCIDR;
		if(_parseNetworkCIDR(networkCIDR, resultCIDR) == false) return false;

		in_addr decIP;
		if (inet_pton(AF_INET, ip.c_str(), &decIP) == 0) return false;
		decIP.s_addr = ntohl(decIP.s_addr);

		dec_network_range range;
		GET_DEC_NET_RANGE(resultCIDR, range);

		return (decIP.s_addr >= range.network_start && decIP.s_addr < range.network_end);
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

	inline SPL::boolean isReserved(SPL::rstring const & ip)
	{
		SPL::rstring networks[12] =
			{"169.254.0.0/16","172.16.0.0/12",
			"192.0.0.0/24","192.0.0.0/29",
			"192.0.2.0/24", "192.88.99.0/24",
			"192.168.0.0/16","198.18.0.0/15",
			"198.51.100.0/24","203.0.113.0/24",
			"240.0.0.0/4","255.255.255.255/32"};

		SPL::list<SPL::rstring> reservedNetworks;
		for(unsigned int i = 0; i < 12; i++)
		{
			reservedNetworks.push_back(networks[i]);
		}

		return isInNetworkList(reservedNetworks, ip);
	}

	inline SPL::uint32 convertToDecimal(SPL::rstring const & value)
	{
		in_addr decIP;
		if (inet_pton(AF_INET, value.c_str(), &decIP) == 0) return 0;

		return ntohl(decIP.s_addr);
	}

	inline SPL::rstring convertToDottedDecimal(SPL::uint32 const & decIP)
	{
		uint32_t networkDecIP = htonl(decIP);
		char dottedIP[INET_ADDRSTRLEN];
		return (inet_ntop(AF_INET, &networkDecIP, dottedIP, INET_ADDRSTRLEN) != NULL) ? (SPL::rstring)dottedIP : "-1";
	}

	inline SPL::list<SPL::rstring> getAllAddressesInNetwork(SPL::rstring const & networkCIDR)
	{
		SPL::list<SPL::rstring> addresses;

		network_cidr resultCIDR;
		if(_parseNetworkCIDR(networkCIDR, resultCIDR) == false) return addresses; //empty list

		dec_network_range range;
		GET_DEC_NET_RANGE(resultCIDR, range);

		for(uint32_t i = range.network_start; i < range.network_end; ++i)
		{
			addresses.push_back(convertToDottedDecimal(i));
		}

		return addresses;
	}

	inline SPL::boolean isGlobal(SPL::rstring const & ip)
	{
		return isInNetwork("192.88.99.0/24", ip) || !isReserved(ip);
	}

	inline SPL::boolean isGreaterThan(SPL::rstring const & ip1, SPL::rstring const & ip2)
	{
		in_addr decIP1, decIP2;
		if (inet_pton(AF_INET, ip1.c_str(), &decIP1) == 0) return false;
		if (inet_pton(AF_INET, ip2.c_str(), &decIP2) == 0) return false;

		return ntohl(decIP1.s_addr) > ntohl(decIP2.s_addr);
	}

	inline SPL::boolean isIPv4CIDRNotation(SPL::rstring const & networkCIDR)
	{
		SPL::list<SPL::rstring> tokens = SPL::Functions::String::tokenize(networkCIDR, "/", false);
		if(tokens.size() != 2) return false;

		in_addr decIP;
		if (inet_pton(AF_INET, tokens[0].c_str(), &decIP) == 0) return false;

		uint32_t prefix = SPL::Functions::Utility::strtoll(tokens[1], 10);
		if(prefix > 32) return false;

		return true;
	}

	inline SPL::boolean isEqualTo(SPL::rstring const & ip1, SPL::rstring const & ip2)
	{
		in_addr decIP1, decIP2;
		if (inet_pton(AF_INET, ip1.c_str(), &decIP1) == 0) return false;
		if (inet_pton(AF_INET, ip2.c_str(), &decIP2) == 0) return false;

		return ntohl(decIP1.s_addr) == ntohl(decIP2.s_addr);
	}

	inline SPL::boolean isInIPRange(SPL::rstring const & startIP, SPL::rstring const & endIP, SPL::rstring const & ip)
	{
		in_addr decStartIP, decEndIP, decIP;
		if (inet_pton(AF_INET, startIP.c_str(), &decStartIP) == 0) return false;
		if (inet_pton(AF_INET, endIP.c_str(), &decEndIP) == 0) return false;
		if (inet_pton(AF_INET, ip.c_str(), &decIP) == 0) return false;

		return (ntohl(decIP.s_addr) >= ntohl(decStartIP.s_addr) && ntohl(decIP.s_addr) < ntohl(decEndIP.s_addr));
	}

	inline SPL::boolean isLessThan(SPL::rstring const & ip1, SPL::rstring const & ip2)
	{
		in_addr decIP1, decIP2;
		if (inet_pton(AF_INET, ip1.c_str(), &decIP1) == 0) return false;
		if (inet_pton(AF_INET, ip2.c_str(), &decIP2) == 0) return false;

		return ntohl(decIP1.s_addr) < ntohl(decIP2.s_addr);
	}

	inline SPL::boolean isLinkLocal(SPL::rstring const & ip)
	{
		return isInNetwork("169.254.0.0/16", ip);
	}

	inline SPL::boolean isLoopback(SPL::rstring const & ip)
	{
		return isInNetwork("127.0.0.0/8", ip);
	}

	inline SPL::boolean isMulticast(SPL::rstring const & ip)
	{
		return isInNetwork("224.0.0.0/4", ip);
	}

	inline SPL::boolean isNetworkOverlap(SPL::rstring const & networkCIDR1, SPL::rstring const & networkCIDR2)
	{
		network_cidr resultCIDR1, resultCIDR2;
		if(_parseNetworkCIDR(networkCIDR1, resultCIDR1) == false) return false;
		if(_parseNetworkCIDR(networkCIDR2, resultCIDR2) == false) return false;

		dec_network_range range1;
		GET_DEC_NET_RANGE(resultCIDR1, range1);

		dec_network_range range2;
		GET_DEC_NET_RANGE(resultCIDR2, range2);

		return (range2.network_start >= range1.network_start && range2.network_start <= range1.network_end)
			|| (range1.network_start >= range2.network_start && range1.network_start <= range2.network_end);
	}

	inline SPL::boolean isPrivate(SPL::rstring const & ip)
	{
		SPL::rstring networks[3] = {"10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16"};

		SPL::list<SPL::rstring> privateNetworks;
		for(unsigned int i = 0; i < 3; ++i)
		{
			privateNetworks.push_back(networks[i]);
		}

		return isInNetworkList(privateNetworks, ip);
	}

	inline SPL::uint32 numAddressesInIPRange(SPL::rstring const & startIP, SPL::rstring const & endIP)
	{
		in_addr decStartIP, decEndIP;
		if (inet_pton(AF_INET, startIP.c_str(), &decStartIP) == 0) return 0;
		if (inet_pton(AF_INET, endIP.c_str(), &decEndIP) == 0) return 0;

		in_addr_t diff = ntohl(decEndIP.s_addr) - ntohl(decStartIP.s_addr);

		return (diff < 0) ? 0 : diff;
	}

	inline SPL::uint32 numAddressesInNetwork(SPL::rstring const & networkCIDR)
	{
		network_cidr resultCIDR;
		if(_parseNetworkCIDR(networkCIDR, resultCIDR) == false) return 0;

		dec_network_range range;
		GET_DEC_NET_RANGE(resultCIDR, range);

		return range.network_end - range.network_start;
	}

	inline SPL::int32 compare(SPL::rstring const & ip1, SPL::rstring const & ip2)
	{
		in_addr decIP1, decIP2;
		if (inet_pton(AF_INET, ip1.c_str(), &decIP1) == 0) return -2;
		if (inet_pton(AF_INET, ip2.c_str(), &decIP2) == 0) return -2;
		decIP1.s_addr = ntohl(decIP1.s_addr);
		decIP2.s_addr = ntohl(decIP2.s_addr);

		return (decIP1.s_addr == decIP2.s_addr) ? 0 : ((decIP1.s_addr < decIP2.s_addr) ? -1 : 1) ;
	}
}
}
}
}
}

#endif  /* _NETWORK_IPV4_FUNCTIONS_H_ */
