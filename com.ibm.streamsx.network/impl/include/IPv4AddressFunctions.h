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
#include <SPL/Runtime/Utility/Mutex.h>


// ignore erroneous GCC warning "missing braces around initializer" for
// "textValue" array in "convertIPV4AddressNumericToString()" function
// (RHEL/CentOS 6 only, see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119)

#pragma GCC diagnostic ignored "-Wmissing-braces"


struct dec_network_range {
    uint32_t network_start;
    uint32_t network_end;
};

struct network_cidr {
    uint32_t ip;
    uint32_t prefix;
};

#define GET_DEC_NET_RANGE(cidr, range) { \
    range.network_start = ((cidr.ip >> (32-cidr.prefix)) << (32-cidr.prefix)); \
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
      // address in host byte order into a string representation.
      static SPL::rstring convertIPV4AddressNumericToString(SPL::uint32 ipv4AddressNumeric) {
        struct textEntry {
          char text[4];
        };

        static struct textEntry textValue[256] = {
           "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
          "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
          "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
          "30", "31", "32", "33", "34", "35", "36", "37", "38", "39",
          "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
          "50", "51", "52", "53", "54", "55", "56", "57", "58", "59",
          "60", "61", "62", "63", "64", "65", "66", "67", "68", "69",
          "70", "71", "72", "73", "74", "75", "76", "77", "78", "79",
          "80", "81", "82", "83", "84", "85", "86", "87", "88", "89",
          "90", "91", "92", "93", "94", "95", "96", "97", "98", "99",

          "100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
          "110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
          "120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
          "130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
          "140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
          "150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
          "160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
          "170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
          "180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
          "190", "191", "192", "193", "194", "195", "196", "197", "198", "199",

          "200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
          "210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
          "220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
          "230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
          "240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
          "250", "251", "252", "253", "254", "255"};
        
        char retString[16];
        uint8_t strOffset = 0;
        const uint32_t networkOrderAddress = htonl(ipv4AddressNumeric);
        for(uint8_t byteOffset=0; byteOffset<4; byteOffset++) {
          uint8_t byteVal = (networkOrderAddress >> ((byteOffset)*8)) & 0xff;

          uint8_t endCnt = 1; 
          if(byteVal >= 10) endCnt++; 
          if(byteVal >= 100) endCnt++; 

          for(int i=0; i<endCnt; i++) {
            retString[strOffset] = textValue[byteVal].text[i];
            strOffset++;
          }
          if(byteOffset == 3) {
            retString[strOffset] = 0;
          } else {
            retString[strOffset] = '.';
          }
          strOffset++;
        }

        return(retString);
      }

      // This function converts a string representation of an IPv4 address
      // to a four-byte binary representation. If the string does not represent a valid
      // IPv4 address, zero is returned.
      static SPL::uint32 convertIPV4AddressStringToNumeric(SPL::rstring ipv4AddressString) {

        // save results in this cache so the conversion is only done once
        static SPL::Mutex cacheMutex;
        static SPL::map<SPL::rstring, SPL::uint32> cache;

        // return the result saved in the cache, if this string has been converted before
        {
          SPL::AutoMutex m(cacheMutex); 
          const SPL::map<SPL::rstring, SPL::uint32>::iterator i = cache.find(ipv4AddressString);
          if (i != cache.end()) return i->second;
        }
        
        // convert the string to a numeric IPv4 address in host byte order
        struct in_addr networkOrderAddress;
        inet_pton(AF_INET, ipv4AddressString.c_str(), &networkOrderAddress);
        const SPL::uint32 ipv4AddressNumeric = ntohl(networkOrderAddress.s_addr);

        // save the conversion in the cache
        {
          SPL::AutoMutex m(cacheMutex); 
          cache.add(ipv4AddressString, ipv4AddressNumeric);
        }

        return ipv4AddressNumeric;
      }

      // This function converts a string representing an IPv4 address into a
      // string representation of the subnet address, using the specified number of mask
      // bits. If the string does not represent a valid IPv4 address, or the count is
      // negative, zero is returned.

      static SPL::rstring convertIPV4AddressStringToSubnet(SPL::rstring ipv4Address, SPL::int32 maskbits) {

        // empty addresses are easy
        if (ipv4Address.empty() || maskbits<=0) return SPL::rstring("0.0.0.0");

        // save conversions in this cache so the network resolution is only done once
        static SPL::Mutex cacheMutex;
        static SPL::map<SPL::rstring, SPL::rstring> cache;

        // construct a CIDR-style subnet string
        const SPL::rstring cidrAddress = ipv4Address + "/" + streams_boost::lexical_cast<std::string>(maskbits);
        SPLLOG(L_DEBUG, "converting " << cidrAddress << " ...", "convertIPV4AddressToSubnet");

        // return the IPv4 subnet address saved in the cache, if this address has been converted before
        {
          SPL::AutoMutex m(cacheMutex); 
          SPL::map<SPL::rstring, SPL::rstring>::iterator i = cache.find(cidrAddress);
          if (i != cache.end()) return i->second;
        }

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
        {
          SPL::AutoMutex m(cacheMutex); 
          cache.add(cidrAddress, subnetAddress);
        }
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
        static SPL::Mutex cacheMutex;
        static SPL::map<SPL::rstring, SPL::rstring> cache;
        //SPLLOG(L_DEBUG, "domain name lookup for '" << ipv4address << "' ...", "convertIPV4AddressToHostname");

        // return the hostname saved in the cache, if this IPv4 address has been converted before
        {
          SPL::AutoMutex m(cacheMutex); 
          SPL::map<SPL::rstring, SPL::rstring>::iterator i = cache.find(ipv4Address);
          if (i != cache.end()) return i->second;
        }

        // otherwise, resolve the IPv4 address into a hostname with a domain name lookup
        const SPL::rstring hostname = convertTo(ipv4Address, 0);
        {
          SPL::AutoMutex m(cacheMutex); 
          cache.add(ipv4Address, hostname);
        }
        //SPLLOG(L_DEBUG, "domain name found '" << ipv4address << "' --> '" << hostname  << "'...", "convertIPV4AddressToHostname");

        return hostname;
      }



      // This function converts a four-byte binary representation of an IPv4
      // address into a hostname. If no hostname can be found for the address, an empty
      // string is returned.

      static SPL::rstring convertIPV4AddressNumericToHostname(SPL::uint32 ipv4AddressNumeric) {

        // save all conversions in this cache so the network resolution is only done once
        static SPL::Mutex cacheMutex;
        static SPL::map<SPL::uint32, SPL::rstring> cache;

        // return the hostname saved in the cache, if this address has been converted before
        {
          SPL::AutoMutex m(cacheMutex); 
          SPL::map<SPL::uint32, SPL::rstring>::iterator i = cache.find(ipv4AddressNumeric);
          if (i != cache.end()) return i->second;
        }

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
        SPLLOG(L_DEBUG, "domain name found '" << (uint32_t)ipv4AddressNumeric << "' --> '" << hostname  << "', cachesize=" << cache.getSize(), "convertIPV4AddressNumericToHostname");

        // add the mapping to the cache and return the hostname
        {
          SPL::AutoMutex m(cacheMutex); 
          cache.add(ipv4AddressNumeric, hostname);
        }

        return hostname;
      }



      // This function converts a hostname into a string representation of
      // an IPv4 address. If no address can be found for the hostname, an empty string is
      // returned.

      static SPL::rstring convertHostnameToIPV4AddressString(SPL::rstring hostname) {

        // save all conversions in this cache so the network resolution is only done once
        static SPL::Mutex cacheMutex;
        static SPL::map<SPL::rstring, SPL::rstring> cache;
        //SPLLOG(L_DEBUG, "domain name lookup for '" << hostname << "' ...", "convertHostnameToIPV4Address");

        // return the IPv4 address saved in the cache, if this hostname has been converted before
        {
          SPL::AutoMutex m(cacheMutex); 
          SPL::map<SPL::rstring, SPL::rstring>::iterator i = cache.find(hostname);
          if (i != cache.end()) return i->second;
        }

        // otherwise, resolve the hostname into an IPv4 address with a domain name lookup
        const SPL::rstring ipv4address = convertTo(hostname, NI_NUMERICHOST);
        {
          SPL::AutoMutex m(cacheMutex); 
          cache.add(hostname, ipv4address);
        }
        //SPLLOG(L_DEBUG, "domain name found '" << hostname << "' --> '" << ipv4address  << "'...", "convertHostnameToIPV4Address");

        return ipv4address;
      }



      // This function converts a hostname into a binary IPv4 address. If no
      // address can be found for the hostname, zero is returned.

      static SPL::uint32 convertHostnameToIPV4AddressNumeric(SPL::rstring hostname) {

        // save all conversions in this cache so the network resolution is only done once
        static SPL::Mutex cacheMutex;
        static SPL::map<SPL::rstring, SPL::uint32> cache;
        SPLLOG(L_DEBUG, "domain name lookup for '" << hostname << "' ...", "convertHostnameToIPV4AddressNumeric");

        // return the address saved in the cache, if this hostname has been converted before
        {
          SPL::AutoMutex m(cacheMutex); 
          SPL::map<SPL::rstring, SPL::uint32>::iterator i = cache.find(hostname);
          if (i != cache.end()) return i->second;
        }

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
        SPLLOG(L_DEBUG, "domain name found '" << hostname << "' --> '" << ipv4AddressNumeric  << "', cachesize=" << cache.getSize(), "convertHostnameToIPV4AddressNumeric");

        // add the mapping to the cache and return the hostname
        {
          SPL::AutoMutex m(cacheMutex); 
          cache.add(hostname, ipv4AddressNumeric);
        }

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

      static SPL::list<SPL::uint32> getAllAddressesInNetworkInt(SPL::rstring const & networkCIDR)
      {
            SPL::list<SPL::uint32> addresses;

            network_cidr resultCIDR;
            if(parseNetworkCIDR_(networkCIDR, resultCIDR) == false) return addresses; //empty list

            dec_network_range range;
            GET_DEC_NET_RANGE(resultCIDR, range);

            for(uint32_t i = range.network_start; i < range.network_end; ++i)
            {
                addresses.push_back((SPL::uint32)i);
            }

            return addresses;
      }

      static SPL::uint32 getAddressRangeInNetworkInt(SPL::rstring const & networkCIDR, SPL::uint32 &addressStart, SPL::uint32 &addressEnd)
      {
            network_cidr resultCIDR;
            addressStart = 0; addressEnd = 0;
            if(parseNetworkCIDR_(networkCIDR, resultCIDR) == false) return 0;

            dec_network_range range;
            GET_DEC_NET_RANGE(resultCIDR, range);

            addressStart = range.network_start;
            addressEnd = range.network_end;

            return (range.network_end - range.network_start);
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
