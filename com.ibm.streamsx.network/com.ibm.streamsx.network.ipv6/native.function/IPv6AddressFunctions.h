/*
** Copyright (C) 2015, International Business Machines Corporation
** All Rights Reserved
*/

#ifndef IPV6ADDRESSFUNCTIONS_H_
#define IPV6ADDRESSFUNCTIONS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <string.h>
#include <streams_boost/lexical_cast.hpp>
#include "SPL/Runtime/Function/SPLFunctions.h"

namespace com { namespace ibm { namespace streamsx { namespace network { namespace ipv6 {


	  // This function converts a sixteen-byte binary representation of an
	  // IPv6 address into a string representation.

	  static SPL::rstring convertIPV6AddressNumericToString(SPL::blist<SPL::uint8,16> ipv6AddressNumeric) {
	    char ipv6Address[INET6_ADDRSTRLEN];
	    return std::string(inet_ntop(AF_INET6, ipv6AddressNumeric.begin(), ipv6Address, sizeof(ipv6Address)));
	  } 


	  // This function converts a string representation of an IPv6 address
	  // into a sixteen-byte binary representation. If the string does not represent a
	  // valid IPv6 address, zero is returned.

	  static SPL::blist<SPL::uint8,16> convertIPV6AddressStringToNumeric(SPL::rstring ipv6AddressString) {
	    struct in6_addr ipv6Address;
	    inet_pton(AF_INET6, ipv6AddressString.c_str(), &ipv6Address);
	    return SPL::blist<SPL::uint8,16>(&ipv6Address.s6_addr[0], &ipv6Address.s6_addr[16]);
	  }


} } } } } 

#endif /* IPV6ADDRESSFUNCTIONS_H_ */

