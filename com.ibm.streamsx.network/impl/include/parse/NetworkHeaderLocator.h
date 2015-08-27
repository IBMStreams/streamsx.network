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
#include <netinet/udp.h>
#include <netinet/tcp.h>

class NetworkHeaderLocator {

 public:

  // packet buffer and length
  char* packetBuffer;
  int bufferLength;
  int pktLength;

  // network header pointers and lengths
  struct ethhdr* etherHeader; int etherHeaderLength;
  struct ip* ipv4Header; int ipv4HeaderLength;
  struct udphdr* udpHeader; int udpHeaderLength;
  struct tcphdr* tcpHeader; int tcpHeaderLength;

  // This function locates network headers within a packet
  /*****note: this function does not handle variable-length network headers yet**** */

  void locateNetworkHeaders(char* buffer, int captureLength, int fullLength) {

	// save address and length of packet for the output attribute assignment functions 
	packetBuffer = buffer;
	bufferLength = captureLength;
	pktLength = fullLength;	

	// clear network header pointers and lengths
	etherHeader = NULL; etherHeaderLength = 0;
	ipv4Header = NULL; ipv4HeaderLength = 0;
	udpHeader = NULL; udpHeaderLength = 0;
	tcpHeader = NULL; tcpHeaderLength = 0;
	
	// if there is an ethernet header, overlay an ethernet header structure on it
	if (captureLength>sizeof(struct ethhdr)) {
	  etherHeader = (struct ethhdr*)buffer; }
		
	// if this is an IPv4 packet, and it has an IPv4 header, overlay an IPv4 header structure on it
	if ( etherHeader && ntohs(etherHeader->h_proto)==ETH_P_IP && captureLength>=sizeof(struct ethhdr)+sizeof(struct ip) ) {
	  ipv4Header = (struct ip*)(buffer + sizeof(struct ethhdr)); }
	
	// if this is a UDP packet, and it has a UDP header, overlay a UDP header structure on it
	if ( (ipv4Header && ipv4Header->ip_p==IPPROTO_UDP && captureLength>=sizeof(struct ethhdr)+sizeof(struct ip)+sizeof(struct udphdr)) ) {
	  udpHeader = (struct udphdr*)((char*)ipv4Header + ipv4Header->ip_hl*4); }
	
	// if this is a TCP packet, and it has a TCP header, overlay a TCP header structure on it
	if ( (ipv4Header && ipv4Header->ip_p==IPPROTO_TCP && captureLength>=sizeof(struct ethhdr)+sizeof(struct ip)+sizeof(struct tcphdr)) ) {
	  tcpHeader = (struct tcphdr*)((char*)ipv4Header + ipv4Header->ip_hl*4); }	
  }

};

#endif /* NETWORKHEADERLOCATOR_H_ */

