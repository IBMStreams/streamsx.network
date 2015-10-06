/*
** Copyright (C) 2011, 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef IPV4_ADDRESS_FUNCTIONS_H_
#define IPV4_ADDRESS_FUNCTIONS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include <streams_boost/lexical_cast.hpp>

#include "SPL/Runtime/Function/SPLFunctions.h"

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

namespace com { namespace ibm { namespace streamsx { namespace network { namespace ipv4 {

	/*
	 * Internal use only. Splits a networkCIDR string into IP and prefix components
	 */
	inline SPL::boolean parseNetworkCIDR_(SPL::rstring const & networkCIDR, network_cidr &resultCIDR)
	{
		SPL::list<SPL::rstring> tokens = SPL::Functions::String::tokenize(networkCIDR, "/", false);
		if(tokens.size() != 2) return false;

		if (inet_pton(AF_INET, tokens[0].c_str(), &(resultCIDR.ip)) == 0) return false;
		resultCIDR.ip = ntohl(resultCIDR.ip);

		resultCIDR.prefix = SPL::Functions::Utility::strtoll(tokens[1], 10);
		if(resultCIDR.prefix > 32) return false;

		return true;
	}

	  // This internal function handles domain name lookups for the 'convert*Hostname()' functions below.

	  static SPL::rstring convertTo(SPL::rstring addressOrHostname, int flags) {
	    
	    // convert the string (whether its an IPv4 address or hostname) 
	    // into a binary representation of an IPv4 address (if it has one), 
	    // stored in a sockaddr structure
	    struct addrinfo *result;
	    SPLLOG(L_DEBUG, "domain name lookup for '" << addressOrHostname << "' ...", "convertIPV4AddressToHostname");
	    int rc1 = getaddrinfo(addressOrHostname.c_str(), NULL, NULL, &result);
	    if (rc1) {
	      SPLLOG(L_ERROR, "getaddrinfo(" << addressOrHostname << ") failed, " << gai_strerror(rc1), "convertIPV4AddressToHostname");
	      return addressOrHostname;
	    }

	    // convert the binary representation of the IPv4 address in the sockaddr
	    // structure into a hostname (if it has one) or an IPv4 address, depending 
	    // upon the flags
	    char hbuf[NI_MAXHOST];
	    char sbuf[NI_MAXSERV];
	    int rc2 = getnameinfo(result->ai_addr, sizeof(struct sockaddr), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), flags);
	    if (rc2) {
	      SPLLOG(L_ERROR, "getnameinfo(" << addressOrHostname << ") failed, " << gai_strerror(rc2), "convertIPV4AddressToHostname");
	      return addressOrHostname;
	    }
	    
	    // return the converted string
	    SPLLOG(L_DEBUG, "domain name lookup for '" << addressOrHostname << "' found '" << hbuf << "'", "convertIPV4AddressToHostname");
	    return SPL::rstring(hbuf);
	  }



	  // This function converts a four-byte binary representation of an IPv4
	  // address into a string representation.

	  static SPL::rstring convertIPV4AddressNumericToString(SPL::uint32 ipv4AddressNumeric) {
	    uint32_t networkOrderAddress;
	    char ipv4Address[INET_ADDRSTRLEN];
	    networkOrderAddress = htonl(ipv4AddressNumeric);
	    return std::string(inet_ntop(AF_INET, &networkOrderAddress, ipv4Address, sizeof(ipv4Address)));
	  }



	  // This function converts a string representation of an IPv4 address
	  // to a four-byte binary representation. If the string does not represent a valid
	  // IPv4 address, zero is returned.

	  static SPL::uint32 convertIPV4AddressStringToNumeric(SPL::rstring ipv4AddressString) {
	    struct in_addr ipv4Address;
	    inet_pton(AF_INET, ipv4AddressString.c_str(), &ipv4Address);
	    return ntohl(ipv4Address.s_addr);
	  }



	  // This function converts a string representing an IPv4 address into a
	  // string representation of the subnet address, using the specified number of mask
	  // bits. If the string does not represent a valid IPv4 address, or the count is
	  // negative, zero is returned.

	  static SPL::rstring convertIPV4AddressStringToSubnet(SPL::rstring ipv4Address, SPL::int32 maskbits) { 

	    // empty addresses are easy
	    if (ipv4Address.empty() || maskbits<=0) return SPL::rstring("0.0.0.0");
	    
	    // save all conversions in this cache so the network resolution is only done once
	    static SPL::map<SPL::rstring, SPL::rstring > mapping;
	    const SPL::rstring cidrAddress = ipv4Address + "/" + streams_boost::lexical_cast<std::string>(maskbits);
	    SPLLOG(L_DEBUG, "converting " << cidrAddress << " ...", "convertIPV4AddressToSubnet");
	    
	    // return the IPv4 address saved in the cache, if this address has been converted before
	    SPL::map<SPL::rstring, SPL::rstring >::iterator i = mapping.find(cidrAddress);
	    if (i != mapping.end()) return i->second;
	    
	    // create the bitmask
	    if (maskbits>32) maskbits = 32;
	    const uint32_t bitmask = 0xFFFFFFFF << (32-maskbits);
	    
	    // convert the string representation of the IPv4 address to binary
	    struct in_addr address;
	    const int rc = inet_aton(ipv4Address.c_str(), &address);
	    if (rc==0) {
	      SPLLOG(L_ERROR, "inet_aton(" << ipv4Address << ") failed, rc=" << rc, "convertIPV4AddressToSubnet");
	      address.s_addr = 0; // set 'address' to zeroes
	    }
	    
	    // apply the bitmask to the binary representation of the IPv4 address
	    // to produce its subnet address
	    address.s_addr = htonl( ntohl(address.s_addr) & bitmask );
	    const SPL::rstring subnetAddress(inet_ntoa(address));
	    
	    // save the conversion in the cache
	    mapping.add(cidrAddress, subnetAddress);
	    SPLLOG(L_DEBUG, "converted " << cidrAddress << " to subnet " << subnetAddress, "convertIPV4AddressToSubnet");
	    
	    // return the string representation of the subnet address
	    return subnetAddress;
	  }



	  // This function converts a four-byte binary representation of an IPv4
	  // address into a four-byte binary subnet address, using the specified number of
	  // mask bits.

	  static SPL::uint32 convertIPV4AddressNumericToSubnet(SPL::uint32 ipv4Address, SPL::int32 maskbits) { 
	    
	    // create the bitmask
	    if (maskbits<0) maskbits = 0;
	    if (maskbits>32) maskbits = 32;
	    const uint32_t bitmask = 0xFFFFFFFF << (32-maskbits);
	    
	    // apply the bitmask to the binary representation of the IPv4 address
	    // to produce its subnet address
	    SPL::uint32 subnetAddress = ipv4Address & bitmask;
	    
	    // return the string representation of the subnet address
	    return subnetAddress;
	  }




	  // convert a string containing an IPv4 address (in 'dotted decimal' format) into
	  // a string containing a fully-qualified domain name, if it has one, or else
	  // return the 'dotted decimal' representation

	  static SPL::rstring convertIPV4AddressStringToHostname(SPL::rstring ipv4Address) { 
	    
	    // save all conversions in this cache so the network resolution is only done once
	    static SPL::map<SPL::rstring, SPL::rstring > mapping;
	    //SPLLOG(L_DEBUG, "domain name lookup for '" << ipv4address << "' ...", "convertIPV4AddressToHostname");
	    
	    // return the hostname saved in the cache, if this IPv4 address has been converted before
	    SPL::map<SPL::rstring, SPL::rstring >::iterator i = mapping.find(ipv4Address);
	    if (i != mapping.end()) return i->second;
	    
	    // otherwise, resolve the IPv4 address into a hostname with a domain name lookup
	    const SPL::rstring hostname = convertTo(ipv4Address, 0); 
	    //SPLLOG(L_DEBUG, "domain name found '" << ipv4address << "' --> '" << hostname  << "'...", "convertIPV4AddressToHostname");
	    mapping.add(ipv4Address, hostname);
	    
	    return hostname;
	  }



	  // This function converts a four-byte binary representation of an IPv4
	  // address into a hostname. If no hostname can be found for the address, an empty
	  // string is returned.

	  static SPL::rstring convertIPV4AddressNumericToHostname(SPL::uint32 ipv4AddressNumeric) {
	    
	    // save all conversions in this cache so the network resolution is only done once
	    static SPL::map<SPL::uint32, SPL::rstring > mapping;
	    
	    // return the hostname saved in the cache, if this address has been converted before
	    SPL::map<SPL::uint32, SPL::rstring >::iterator i = mapping.find(ipv4AddressNumeric);
	    if (i != mapping.end()) return i->second;
	    
	    // store the IPv4 address in a socket structure
	    const struct sockaddr_in sock = { AF_INET, 0, { htonl((uint32_t)ipv4AddressNumeric) } };
	    
	    // convert the binary representation of the IPv4 address in the sockaddr
	    // structure into a hostname (if it has one) or an IPv4 address, depending 
	    // upon the flags
	    SPLLOG(L_DEBUG, "domain name lookup for '" << (uint32_t)ipv4AddressNumeric << "' ...", "convertIPV4AddressNumericToHostname");
	    char hbuf[NI_MAXHOST];
	    char sbuf[NI_MAXSERV];
	    int rc = getnameinfo((const struct sockaddr*)(&sock), sizeof(sock), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), 0);
	    if (rc) SPLLOG(L_ERROR, "getnameinfo(" << (uint32_t)ipv4AddressNumeric << ") failed, " << gai_strerror(rc), "convertIPV4AddressNumericToHostname");
	    const SPL::rstring hostname = SPL::rstring(rc ? "" : hbuf);
	    SPLLOG(L_DEBUG, "domain name found '" << (uint32_t)ipv4AddressNumeric << "' --> '" << hostname  << "', cachesize=" << mapping.getSize(), "convertIPV4AddressNumericToHostname");
	    
	    // add the mapping to the cache and return the hostname
	    mapping.add(ipv4AddressNumeric, hostname);
	    return hostname;
	  }

  

	  // This function converts a hostname into a string representation of
	  // an IPv4 address. If no address can be found for the hostname, an empty string is
	  // returned.

	  static SPL::rstring convertHostnameToIPV4AddressString(SPL::rstring hostname) { 
	    
	    // save all conversions in this cache so the network resolution is only done once
	    static SPL::map<SPL::rstring, SPL::rstring > mapping;
	    //SPLLOG(L_DEBUG, "domain name lookup for '" << hostname << "' ...", "convertHostnameToIPV4Address");
	    
	    // return the IPv4 address saved in the cache, if this hostname has been converted before
	    SPL::map<SPL::rstring, SPL::rstring >::iterator i = mapping.find(hostname);
	    if (i != mapping.end()) return i->second;
	    
	    // otherwise, resolve the hostname into an IPv4 address with a domain name lookup
	    const SPL::rstring ipv4address = convertTo(hostname, NI_NUMERICHOST); 
	    //SPLLOG(L_DEBUG, "domain name found '" << hostname << "' --> '" << ipv4address  << "'...", "convertHostnameToIPV4Address");
	    mapping.add(hostname, ipv4address);
	    
	    return ipv4address;
	  }


	  
	  // This function converts a hostname into a binary IPv4 address. If no
	  // address can be found for the hostname, zero is returned.

	  static SPL::uint32 convertHostnameToIPV4AddressNumeric(SPL::rstring hostname) {

	    // save all conversions in this cache so the network resolution is only done once
	    static SPL::map<SPL::rstring, SPL::uint32 > mapping;
	    SPLLOG(L_DEBUG, "domain name lookup for '" << hostname << "' ...", "convertHostnameToIPV4AddressNumeric");
	    
	    // return the address saved in the cache, if this hostname has been converted before
	    SPL::map<SPL::rstring, SPL::uint32 >::iterator i = mapping.find(hostname);
	    if (i != mapping.end()) return i->second;
	    
	    // otherwise, resolve the hostname into a binary representation of 
	    // its IPv4 address with a domain name lookup
	    struct addrinfo *result;
	    SPL::uint32 ipv4AddressNumeric = 0;
	    int rc = getaddrinfo(hostname.c_str(), NULL, NULL, &result);
	    if (!rc) { 
	      // ... was ... const SPL::uint32 ipv4AddressNumeric = rc ? 0 : *(SPL::uint32*)(result->ai_addr->sa_data);
	      char* sa_data_address = result->ai_addr->sa_data;
	      ipv4AddressNumeric = *(SPL::uint32*)(sa_data_address);
	    } else {
	      SPLLOG(L_ERROR, "getaddrinfo(" << hostname << ") failed, " << gai_strerror(rc), "convertHostnameToIPV4AddressNumeric"); 
	    }
	    SPLLOG(L_DEBUG, "domain name found '" << hostname << "' --> '" << ipv4AddressNumeric  << "', cachesize=" << mapping.getSize(), "convertHostnameToIPV4AddressNumeric");
	    
	    // add the mapping to the cache and return the hostname
	    mapping.add(hostname, ipv4AddressNumeric);
	    return ipv4AddressNumeric;
	  }

	  static SPL::boolean isIPV4Address(SPL::rstring const & value)
	  {
			in_addr decIP;
			return (inet_pton(AF_INET, value.c_str(), &decIP) == 1) ? true : false ;
	  }

	  static SPL::boolean isInNetwork(SPL::rstring const & networkCIDR, SPL::uint32 const & ip)
	  {
		  	network_cidr resultCIDR;
			if(parseNetworkCIDR_(networkCIDR, resultCIDR) == false) return false;

			dec_network_range range;
			GET_DEC_NET_RANGE(resultCIDR, range);

			return (ip >= range.network_start && ip < range.network_end);
	  }

	  static SPL::boolean isInNetwork(SPL::rstring const & networkCIDR, SPL::rstring const & ip)
	  {
			in_addr decIP;
			if (inet_pton(AF_INET, ip.c_str(), &decIP) == 0) return false;
			decIP.s_addr = ntohl(decIP.s_addr);

			return isInNetwork(networkCIDR, (SPL::uint32)decIP.s_addr);
	  }

	  static SPL::boolean isInNetworkList(SPL::list<SPL::rstring> const & networkList, SPL::rstring const & ip)
	  {
			SPL::list<SPL::rstring>::const_iterator it;
			for(it = networkList.begin(); it != networkList.end(); ++it)
			{
				if(isInNetwork(*it, ip)) return true;
			}

			return false;
	  }

	  static SPL::boolean isReserved(SPL::rstring const & ip)
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

	  static SPL::list<SPL::rstring> getAllAddressesInNetwork(SPL::rstring const & networkCIDR)
	  {
			SPL::list<SPL::rstring> addresses;

			network_cidr resultCIDR;
			if(parseNetworkCIDR_(networkCIDR, resultCIDR) == false) return addresses; //empty list

			dec_network_range range;
			GET_DEC_NET_RANGE(resultCIDR, range);

			for(uint32_t i = range.network_start; i < range.network_end; ++i)
			{
				addresses.push_back(convertIPV4AddressNumericToString((SPL::uint32)i));
			}

			return addresses;
	  }

	  static SPL::boolean isGlobal(SPL::rstring const & ip)
	  {
			return isInNetwork("192.88.99.0/24", ip) || !isReserved(ip);
	  }

	  static SPL::boolean isGreaterThan(SPL::rstring const & ip1, SPL::rstring const & ip2)
	  {
			in_addr decIP1, decIP2;
			if (inet_pton(AF_INET, ip1.c_str(), &decIP1) == 0) return false;
			if (inet_pton(AF_INET, ip2.c_str(), &decIP2) == 0) return false;

			return ntohl(decIP1.s_addr) > ntohl(decIP2.s_addr);
	  }

	  static SPL::boolean isIPV4CIDRNotation(SPL::rstring const & networkCIDR)
	  {
			SPL::list<SPL::rstring> tokens = SPL::Functions::String::tokenize(networkCIDR, "/", false);
			if(tokens.size() != 2) return false;

			in_addr decIP;
			if (inet_pton(AF_INET, tokens[0].c_str(), &decIP) == 0) return false;

			uint32_t prefix = SPL::Functions::Utility::strtoll(tokens[1], 10);
			if(prefix > 32) return false;

			return true;
	  }

	  static SPL::boolean isEqualTo(SPL::rstring const & ip1, SPL::rstring const & ip2)
	  {
			in_addr decIP1, decIP2;
			if (inet_pton(AF_INET, ip1.c_str(), &decIP1) == 0) return false;
			if (inet_pton(AF_INET, ip2.c_str(), &decIP2) == 0) return false;

			return ntohl(decIP1.s_addr) == ntohl(decIP2.s_addr);
	  }

	  static SPL::boolean isInIPRange(SPL::rstring const & startIP, SPL::rstring const & endIP, SPL::rstring const & ip)
	  {
			in_addr decStartIP, decEndIP, decIP;
			if (inet_pton(AF_INET, startIP.c_str(), &decStartIP) == 0) return false;
			if (inet_pton(AF_INET, endIP.c_str(), &decEndIP) == 0) return false;
			if (inet_pton(AF_INET, ip.c_str(), &decIP) == 0) return false;

			return (ntohl(decIP.s_addr) >= ntohl(decStartIP.s_addr) && ntohl(decIP.s_addr) < ntohl(decEndIP.s_addr));
	  }

	  static SPL::boolean isLessThan(SPL::rstring const & ip1, SPL::rstring const & ip2)
	  {
			in_addr decIP1, decIP2;
			if (inet_pton(AF_INET, ip1.c_str(), &decIP1) == 0) return false;
			if (inet_pton(AF_INET, ip2.c_str(), &decIP2) == 0) return false;

			return ntohl(decIP1.s_addr) < ntohl(decIP2.s_addr);
	  }

	  static SPL::boolean isLinkLocal(SPL::rstring const & ip)
	  {
			return isInNetwork("169.254.0.0/16", ip);
	  }

	  static SPL::boolean isLoopback(SPL::rstring const & ip)
	  {
			return isInNetwork("127.0.0.0/8", ip);
	  }

	  static SPL::boolean isMulticast(SPL::rstring const & ip)
	  {
			return isInNetwork("224.0.0.0/4", ip);
	  }

	  static SPL::boolean isNetworkOverlap(SPL::rstring const & networkCIDR1, SPL::rstring const & networkCIDR2)
	  {
			network_cidr resultCIDR1, resultCIDR2;
			if(parseNetworkCIDR_(networkCIDR1, resultCIDR1) == false) return false;
			if(parseNetworkCIDR_(networkCIDR2, resultCIDR2) == false) return false;

			dec_network_range range1;
			GET_DEC_NET_RANGE(resultCIDR1, range1);

			dec_network_range range2;
			GET_DEC_NET_RANGE(resultCIDR2, range2);

			return (range2.network_start >= range1.network_start && range2.network_start <= range1.network_end)
				|| (range1.network_start >= range2.network_start && range1.network_start <= range2.network_end);
	  }

	  static SPL::boolean isPrivate(SPL::rstring const & ip)
	  {
			SPL::rstring networks[3] = {"10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16"};

			SPL::list<SPL::rstring> privateNetworks;
			for(unsigned int i = 0; i < 3; ++i)
			{
				privateNetworks.push_back(networks[i]);
			}

			return isInNetworkList(privateNetworks, ip);
	  }

	  static SPL::uint32 numAddressesInIPRange(SPL::rstring const & startIP, SPL::rstring const & endIP)
	  {
			in_addr decStartIP, decEndIP;
			if (inet_pton(AF_INET, startIP.c_str(), &decStartIP) == 0) return 0;
			if (inet_pton(AF_INET, endIP.c_str(), &decEndIP) == 0) return 0;

			in_addr_t diff = ntohl(decEndIP.s_addr) - ntohl(decStartIP.s_addr);

			return (diff < 0) ? 0 : diff;
	  }

	  static SPL::uint32 numAddressesInNetwork(SPL::rstring const & networkCIDR)
	  {
			network_cidr resultCIDR;
			if(parseNetworkCIDR_(networkCIDR, resultCIDR) == false) return 0;

			dec_network_range range;
			GET_DEC_NET_RANGE(resultCIDR, range);

			return range.network_end - range.network_start;
	  }

	  static SPL::int32 compare(SPL::rstring const & ip1, SPL::rstring const & ip2)
	  {
			in_addr decIP1, decIP2;
			if (inet_pton(AF_INET, ip1.c_str(), &decIP1) == 0) return -2;
			if (inet_pton(AF_INET, ip2.c_str(), &decIP2) == 0) return -2;
			decIP1.s_addr = ntohl(decIP1.s_addr);
			decIP2.s_addr = ntohl(decIP2.s_addr);

			return (decIP1.s_addr == decIP2.s_addr) ? 0 : ((decIP1.s_addr < decIP2.s_addr) ? -1 : 1) ;
	  }

} } } } } 

#endif /* IPV4_ADDRESS_FUNCTIONS_H_ */


























