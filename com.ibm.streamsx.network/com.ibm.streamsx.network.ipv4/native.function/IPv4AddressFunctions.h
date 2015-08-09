/*
** Copyright (C) 2015, International Business Machines Corporation
** All Rights Reserved
*/

#ifndef IPV4ADDRESSFUNCTIONS_H_
#define IPV4ADDRESSFUNCTIONS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <string.h>
#include <streams_boost/lexical_cast.hpp>
#include "SPL/Runtime/Function/SPLFunctions.h"

namespace com { namespace ibm { namespace streamsx { namespace network { namespace ipv4 {

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
	    const int rc = inet_aton(ipv4address.c_str(), &address);
	    if (rc==0) {
	      SPLLOG(L_ERROR, "inet_aton(" << ipv4address << ") failed, rc=" << rc, "convertIPV4AddressToSubnet");
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

	  static SPL::uint32 convertIPV4AddressNumericToSubnet(SPL::uint32 ipv4address, SPL::int32 maskbits) { 
	    
	    // create the bitmask
	    if (maskbits<0) maskbits = 0;
	    if (maskbits>32) maskbits = 32;
	    const uint32_t bitmask = 0xFFFFFFFF << (32-maskbits);
	    
	    // apply the bitmask to the binary representation of the IPv4 address
	    // to produce its subnet address
	    SPL::uint32 subnetAddress = ipv4address & bitmask;
	    
	    // return the string representation of the subnet address
	    return subnetAddress;
	  }




	  // convert a string containing an IPv4 address (in 'dotted decimal' format) into
	  // a string containing a fully-qualified domain name, if it has one, or else
	  // return the 'dotted decimal' representation

	  static SPL::rstring convertIPV4AddressStringToHostname(SPL::rstring ipv4address) { 
	    
	    // save all conversions in this cache so the network resolution is only done once
	    static SPL::map<SPL::rstring, SPL::rstring > mapping;
	    //SPLLOG(L_DEBUG, "domain name lookup for '" << ipv4address << "' ...", "convertIPV4AddressToHostname");
	    
	    // return the hostname saved in the cache, if this IPv4 address has been converted before
	    SPL::map<SPL::rstring, SPL::rstring >::iterator i = mapping.find(ipv4address);
	    if (i != mapping.end()) return i->second;
	    
	    // otherwise, resolve the IPv4 address into a hostname with a domain name lookup
	    const SPL::rstring hostname = convertTo(ipv4address, 0); 
	    //SPLLOG(L_DEBUG, "domain name found '" << ipv4address << "' --> '" << hostname  << "'...", "convertIPV4AddressToHostname");
	    mapping.add(ipv4address, hostname);
	    
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

  
	  // This internal function handles domain name lookups for the 'convert*Hostname()' functions above.

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


} } } } } 

#endif /* IPV4ADDRESSFUNCTIONS_H_ */

