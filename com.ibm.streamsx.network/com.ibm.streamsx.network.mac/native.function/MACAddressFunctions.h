/*
** Copyright (C) 2015, International Business Machines Corporation
** All Rights Reserved
*/

#ifndef MACADDRESSFUNCTIONS_H_
#define MACADDRESSFUNCTIONS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <string.h>
#include <streams_boost/lexical_cast.hpp>
#include "SPL/Runtime/Function/SPLFunctions.h"

namespace com { namespace ibm { namespace streamsx { namespace network { namespace mac {

	  // This function converts an ethernet MAC address from a six-byte binary value to a string representation.
	  static SPL::rstring convertMACAddressNumericToString(SPL::blist<SPL::uint8,6> macAddressNumeric) {
	    return std::string(ether_ntoa((struct ether_addr*)macAddressNumeric.begin()));
	  }

	  // This function converts an ethernet MAC address from a string representation to a six-byte binary value.
	  static SPL::blist<SPL::uint8,6> convertMACAddressStringToNumeric(SPL::rstring macAddressString) {
	    struct ether_addr* etherAddress;
	    etherAddress = ether_aton(macAddressString.c_str());
	    return SPL::blist<SPL::uint8,6>(&etherAddress->ether_addr_octet[0], &etherAddress->ether_addr_octet[6]);
	  }


} } } } } 

#endif /* MACADDRESSFUNCTIONS_H_ */

