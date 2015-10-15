/*
** Copyright (C) 2011, 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef NETWORK_HEADER_PARSER_H_
#define NETWORK_HEADER_PARSER_H_

#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>


/////////////////////////////////////////////////////////////////////////////////////
// this class parses ethernet, IPv4, IPv6, UDP, and TCP headers in network packets
/////////////////////////////////////////////////////////////////////////////////////

class NetworkHeaderParser {


 private: 

	// structure of Juniper Networks 'jmirror' headers

	struct JMirrorHeader {
		uint32_t interceptIdentifier;       
		uint32_t sessionIdentifier;
	} __attribute__((packed)) ;

	struct JMirrorHeaders {
		struct iphdr ipHeader;
		struct udphdr udpHeader;
		struct JMirrorHeader jmirrorHeader;
	} __attribute__((packed)) ;

	static const uint16_t jmirrorPort = 30030;


 public:

	// The parseNetworkHeaders() function below returns the address and length
	// of the full packet in these variables.

	char* packetBuffer; // address of packet
	int packetLength; // length of packet, possibly truncated

	// The parsePacketHeaders() function below returns the addresses and lengths
	// of the packet's network headers and payload data in these variables, if
	// present, or NULL and zero, if absent.

	struct JMirrorHeaders* jmirrorHeader; int jmirrorHeaderLength;

	struct ethhdr*  etherHeader; int etherHeaderLength;
	struct iphdr*   ipv4Header;  int ipv4HeaderLength;
	struct ip6_hdr* ipv6Header;  int ipv6HeaderLength;
	struct udphdr*  udpHeader;   int udpHeaderLength;
	struct tcphdr*  tcpHeader;   int tcpHeaderLength;

	char*           payload;     int payloadLength;


	// The parseNetworkHeaders() function below locates the network headers and
	// payload data in the ethernet packet contained in the specified buffer and
	// returns them in the variables above.

	// Note: this function does not yet handle variable-length ethernet headers,
	// such as those with VLAN tags, or variable-length IPv6 headers, such as
	// those with extension headers

	void parseNetworkHeaders(char* buffer, int length) {

		// store address and length of packet for the output attribute assignment functions 
		packetBuffer = buffer;
		packetLength = length;

		// clear network header pointers and lengths to be returned
		jmirrorHeader = NULL; jmirrorHeaderLength = 0;
		etherHeader = NULL; etherHeaderLength = 0;
		ipv4Header = NULL; ipv4HeaderLength = 0;
		ipv6Header = NULL; ipv6HeaderLength = 0;
		udpHeader = NULL; udpHeaderLength = 0;
		tcpHeader = NULL; tcpHeaderLength = 0;
		payload = NULL; payloadLength = 0;
	
		// if the buffer isn't big enough for an ethernet header, give up
		if (length<sizeof(struct ethhdr)) return;

		// overlay an ethernet header on the buffer and step over it
		etherHeader = (struct ethhdr*)buffer; 
		etherHeaderLength = sizeof(struct ethhdr); // ... plus length of optional VLAN tag ?
		buffer += etherHeaderLength;
		length -= etherHeaderLength; 
	
		// if the buffer contains a Juniper Networks mirror packet, step over the 'jmirror' headers
		// (note that field tests are not in natural order so the 'if' will fail faster in the usual case)
		if ( length>=sizeof(struct JMirrorHeaders) ) {
			struct JMirrorHeaders* jmirror = (struct JMirrorHeaders*)buffer;
			if ( ntohs(jmirror->udpHeader.dest)==jmirrorPort &&
				 jmirror->ipHeader.version==4 && 
				 jmirror->ipHeader.ihl==5 && 
				 jmirror->ipHeader.protocol==IPPROTO_UDP ) {
			  jmirrorHeader = jmirror;
			  jmirrorHeaderLength = sizeof(struct JMirrorHeaders);
			  buffer += sizeof(struct JMirrorHeaders);
			  length -= sizeof(struct JMirrorHeaders);
			}
		}

		// if the buffer contains an IPv4 packet, overlay an IPv4 header on it
		if ( ntohs(etherHeader->h_proto)==ETH_P_IP && length>=sizeof(struct ip) && ((struct iphdr*)buffer)->version==4 ) {
			ipv4Header = (struct iphdr*)buffer; 
			ipv4HeaderLength = ipv4Header->ihl * 4;
			buffer += ipv4HeaderLength;
			length -= ipv4HeaderLength;
		}

		// if the buffer contains an IPv6 packet, overlay an IPv6 header on it
		if ( ntohs(etherHeader->h_proto)==ETH_P_IPV6 && length>=sizeof(struct ip6_hdr) && (((struct ip6_hdr*)buffer)->ip6_vfc)>>4==6 ) {
			ipv6Header = (struct ip6_hdr*)buffer; 
			ipv6HeaderLength = sizeof(struct ip6_hdr); // ... plus length of optional extension headers ?
			buffer += ipv6HeaderLength;
			length -= ipv6HeaderLength;
		}

		// if the buffer does not contain an IPv4 or IPv6 packet, give up
		if ( ! (ipv4Header || ipv6Header ) ) return;

		// if the buffer contains a UDP packet, and it has a UDP header, overlay a UDP header on it
		if ( ( ipv4Header && ipv4Header->protocol==IPPROTO_UDP && length>=sizeof(struct udphdr) ) || 
			 ( ipv6Header && ipv6Header->ip6_nxt==IPPROTO_UDP && length>=sizeof(struct udphdr) ) ) {
			udpHeader = (struct udphdr*)buffer;
			udpHeaderLength = sizeof(struct udphdr);
			buffer += udpHeaderLength;
			length -= udpHeaderLength;
		}
	
		// if the buffer contains a TCP packet, and it has a TCP header, overlay a TCP header on it
		if ( ( ipv4Header && ipv4Header->protocol==IPPROTO_TCP && length>=sizeof(struct tcphdr) ) ||
			 ( ipv6Header && ipv6Header->ip6_nxt==IPPROTO_TCP && length>=sizeof(struct tcphdr) ) ) {
			tcpHeader = (struct tcphdr*)buffer;
			tcpHeaderLength = tcpHeader->doff * 4;
			buffer += tcpHeaderLength;
			length -= tcpHeaderLength;
		}	

		// if there is any data left in the buffer, its the packet's payload
		if (length>0) {
			payload = buffer;
			payloadLength = length;
		}
	}

};

#endif /* NETWORK_HEADER_PARSER_H_ */

