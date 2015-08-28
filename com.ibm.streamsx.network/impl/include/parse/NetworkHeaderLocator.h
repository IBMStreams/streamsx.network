/*
** Copyright (C) 2011, 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef NETWORKHEADERLOCATOR_H_
#define NETWORKHEADERLOCATOR_H_

#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

// structure of Juniper Networks 'jmirror' headers
  struct JMirrorHeader {
	uint32_t interceptionIdentifier;       
	uint32_t sessionIdentifier;
  } __attribute__((packed)) ;
struct JMirrorHeaders {
  struct ip ipHeader;
  struct udphdr udpHeader;
  struct JMirrorHeader jmirrorHeader;
} __attribute__((packed)) ;
const uint16_t jmirrorPort = 30030;

class NetworkHeaderLocator {

 public:

  // packet buffer and length, as passed to locateNetworkHeaders() function

  char* packetBuffer; // address of captured packet
  int packetCapturedLength; // captured length of packet, perhaps truncated
  int packetFullLength; // full length of packet, perhaps not all captured

  // network header pointers and lengths, as returned by locateNetworkHeaders() function

  struct ethhdr* etherHeader; int etherHeaderLength;
  struct ip* ipv4Header; int ipv4HeaderLength;
  struct ip6_hdr* ipv6Header; int ipv6HeaderLength;
  struct udphdr* udpHeader; int udpHeaderLength;
  struct tcphdr* tcpHeader; int tcpHeaderLength;
  char* payload; int payloadLength;

  // This function locates network headers within a packet and sets the member variables above.
  // Note: this function does not yet handle variable-length ethernet headers, such as those with
  // VLAN tags, or variable-length IPv6 headers, such as those with extension headers

  void locateNetworkHeaders(char* buffer, int length, int fullLength) {

	// set address and length of packet for the output attribute assignment functions 
	packetBuffer = buffer;
	packetCapturedLength = length;
	packetFullLength = fullLength;	

	// clear network header pointers and lengths to be returned
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
	etherHeaderLength = sizeof(struct ethhdr); // ... plus length of optional VLAN tag ???????????????????
	buffer += etherHeaderLength;
	length -= etherHeaderLength; 
	
	// if the buffer contains a Juniper Networks mirror packet, step over the 'jmirror' headers
	if ( length>=sizeof(struct JMirrorHeaders) ) {
	  struct JMirrorHeaders& jmirror = *(struct JMirrorHeaders*)buffer;
	  if ( jmirror.ipHeader.ip_v==4 && 
		   jmirror.ipHeader.ip_hl==5 && 
		   jmirror.ipHeader.ip_p==IPPROTO_UDP && 
		   ntohs(jmirror.udpHeader.source)==jmirrorPort && 
		   ntohs(jmirror.udpHeader.dest)==jmirrorPort ) {
		buffer += sizeof(struct JMirrorHeaders);
		length -= sizeof(struct JMirrorHeaders);
	  }
	}

	// if the buffer contains an IPv4 packet, overlay an IPv4 header on it
	if ( ntohs(etherHeader->h_proto)==ETH_P_IP && length>=sizeof(struct ip) && ((struct ip*)buffer)->ip_v==4 ) {
	  ipv4Header = (struct ip*)buffer; 
	  ipv4HeaderLength = ipv4Header->ip_hl * 4;
	  buffer += ipv4HeaderLength;
	  length -= ipv4HeaderLength;
	}

	// if the buffer contains an IPv6 packet, overlay an IPv6 header on it
	if ( ntohs(etherHeader->h_proto)==ETH_P_IPV6 && length>=sizeof(struct ip6_hdr) && (((struct ip6_hdr*)buffer)->ip6_vfc)>>4==6 ) {
	  ipv6Header = (struct ip6_hdr*)buffer; 
	  ipv6HeaderLength = sizeof(struct ip6_hdr); // ... plus length of optional extension headers ??????????????????
	  buffer += ipv6HeaderLength;
	  length -= ipv6HeaderLength;
	}

	// if the buffer does not contain an IPv4 or IPv6 packet, give up
	if ( ! (ipv4Header || ipv6Header ) ) return;

	// if the buffer contains a UDP packet, and it has a UDP header, overlay a UDP header on it
	if ( ( ipv4Header && ipv4Header->ip_p==IPPROTO_UDP && length>=sizeof(struct udphdr) ) || 
		 ( ipv6Header && ipv6Header->ip6_nxt==IPPROTO_UDP && length>=sizeof(struct udphdr) ) ) {
	  udpHeader = (struct udphdr*)buffer;
	  udpHeaderLength = sizeof(struct udphdr);
	  buffer += udpHeaderLength;
	  length -= udpHeaderLength;
	}
	
	// if the buffer contains a TCP packet, and it has a TCP header, overlay a TCP header on it
	if ( ( ipv4Header && ipv4Header->ip_p==IPPROTO_TCP && length>=sizeof(struct tcphdr) ) ||
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

#endif /* NETWORKHEADERLOCATOR_H_ */

