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

#include <SPL/Runtime/Type/SPLType.h>


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


    // structure of IEEE 802.1Q VLAN extension of ethernet header

    struct VLANHeader {
      uint16_t vlanTag;
      static const uint16_t priorityClass   = 0xE000;
      static const uint16_t dropEligibility = 0x1000;
      static const uint16_t vlanIdentifier  = 0x0FFF;
      uint16_t protocol;
    }  __attribute__((packed)) ;



    // structure of Generic Routing Encapsulation (GRE) header
    // as used with Encapsulated Remote Switch Port Analyzer II (ERSPAN2) header

    struct GREHeader {
      uint16_t flags;                     // flags for optional fields, plus header version
      static const uint16_t checksumFlag = 0x8000; // checksum field is present
      static const uint16_t routingFlag  = 0x4000; // routing field is present 
      static const uint16_t keyFlag      = 0x2000; // key field is present
      static const uint16_t sequenceFlag = 0x1000; // sequence field is present
      static const uint16_t strictFlag   = 0x0800; // strict routing is requested
      static const uint16_t recursion    = 0x0700; // number of encapsulation layers permitted
      static const uint16_t reserved     = 0x00F8; // reserved flags (should be zero)
      static const uint16_t version      = 0x0007; // version of this header (must be zero)
      uint16_t protocolType;              // ether type of encapsulated packet, always 0x88BE for ERSPAN
      uint16_t checksum[0];               // optional checksum of encapsulated packet, always omitted for ERSPAN
      uint16_t offset[0];                 // optional offset from routing field to first routing entry, always omitted for ERSPAN
      uint32_t key[0];                    // optional correlator set by encapsulator, always omitted for ERSPAN
      uint32_t sequence[0];               // optional sequence number set by encapsulator, almost always included for ERSPAN
      uint32_t routing[0];                // optional routing field set by encapsulator, always omitted for ERSPAN
    } __attribute__((packed)) ;

    struct ERSPAN2Header {
      uint16_t version;                   // header version and VLAN of encapsulated ethernet frame
      uint16_t flags;                     // flags for encapsulated frame
      uint32_t index;                     // index or port number of encapsulator
    } __attribute__((packed)) ;

    struct ERSPAN2Headers {
      struct iphdr ipHeader;
      struct GREHeader greHeader;
      struct ERSPAN2Header erspanHeader;
      struct ethhdr etherHeader;
    } __attribute__((packed)) ;

    static const uint8_t greProtocol = 47; // value of IP header 'protocol' field for GRE packets
    static const uint16_t erspanProtocolType = 0x88BE; // value of GRE header 'protocolType' field for ERSPAN2 packets


    // The parseNetworkHeaders() function below returns the address and length
    // of the full packet in these variables.

    char* packetBuffer; // address of packet
    int packetLength; // length of packet, possibly truncated

    // The parsePacketHeaders() function below returns the addresses and lengths
    // of the packet's network headers and payload data in these variables, if
    // present, or NULL and zero, if absent.

    struct JMirrorHeaders* jmirrorHeader; int jmirrorHeaderLength;
    struct ERSPAN2Headers* erspanHeader; int erspanHeaderLength;
    struct ethhdr*  etherHeader; int etherHeaderLength;
    struct VLANHeader* vlanHeader; int vlanHeaderLength;
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


    void parseNetworkHeaders(char* buffer, int length, bool jmirrorEnable = false) {

        // store address and length of packet for the output attribute assignment functions
        packetBuffer = buffer;
        packetLength = length;

        // clear network header pointers and lengths to be returned
        jmirrorHeader = NULL; jmirrorHeaderLength = 0;
        erspanHeader = NULL; erspanHeaderLength = 0;
        vlanHeader = NULL;  vlanHeaderLength = 0;
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
        uint16_t etherType = ntohs(etherHeader->h_proto);
        etherHeaderLength = sizeof(struct ethhdr); // ... not including optional VLAN tags
        //printf("ethernet: "); for (int i=0; i<etherHeaderLength; i++) printf("%02x ", (uint8_t)buffer[i]); printf("\n");
        buffer += etherHeaderLength;
        length -= etherHeaderLength;

        // if the ethernet header has one or more IEEE 802.1Q VLAN headers, 
        // remember where they are in the buffer and step over them
        if (ETH_P_8021Q == etherType) {
          vlanHeader = (struct VLANHeader*) buffer;
          vlanHeaderLength = sizeof(VLANHeader);
          struct VLANHeader *nextVLAN = vlanHeader;
          etherType = ntohs(nextVLAN->protocol);
          while(ETH_P_8021Q == etherType) {
            vlanHeaderLength += sizeof(VLANHeader);
            nextVLAN++;
            etherType = ntohs(nextVLAN->protocol);
          } ;
      	  buffer += vlanHeaderLength;
      	  length -= vlanHeaderLength;
        }
        //if (vlanHeader) printf("    VLAN: "); for (int i=0; i<vlanHeaderLength; i++) printf("%02x ", (uint8_t)(((uint8_t*)vlanHeader)[i]) ); printf("\n");

        // if the buffer contains a Juniper Networks mirror packet, step over the 'jmirror' headers
        // (note that field tests are not in natural order so the inner 'if' will fail faster in the usual case)
        if ( length>=sizeof(struct JMirrorHeaders) ) {
            struct JMirrorHeaders* jmirror = (struct JMirrorHeaders*)buffer;
            if ( jmirrorEnable &&
                 ntohs(jmirror->udpHeader.dest)==jmirrorPort &&
                 jmirror->ipHeader.version==4 &&
                 jmirror->ipHeader.ihl==5 &&
                 jmirror->ipHeader.protocol==IPPROTO_UDP ) {
              jmirrorHeader = jmirror;
              jmirrorHeaderLength = sizeof(struct JMirrorHeaders);
              buffer += sizeof(struct JMirrorHeaders);
              length -= sizeof(struct JMirrorHeaders);
            }
        }

        // if the buffer contains a GRE ERSPAN packet, step over the GRE and ERSPAN headers
        // (note that field tests are not in natural order so the inner 'if' will fail faster in the usual case)
        if ( length>=sizeof(struct ERSPAN2Headers) ) {
            struct ERSPAN2Headers* erspan = (struct ERSPAN2Headers*)buffer;
            if ( ntohs(erspan->greHeader.protocolType)==erspanProtocolType && 
                 erspan->ipHeader.protocol==greProtocol && 
                 erspan->ipHeader.version==4 &&
                 erspan->ipHeader.ihl==5 ) {
              uint16_t greHeaderFlags = ntohs(erspan->greHeader.flags);
              int greHeaderLength = 
                ( greHeaderFlags&GREHeader::checksumFlag ? 4 : 0 ) +
                ( greHeaderFlags&GREHeader::routingFlag ? 4 : 0 ) +
                ( greHeaderFlags&GREHeader::keyFlag ? 4 : 0 ) +
                ( greHeaderFlags&GREHeader::sequenceFlag ? 4 : 0 );
              erspanHeader = (struct ERSPAN2Headers*)buffer;
              erspanHeaderLength = sizeof(struct ERSPAN2Headers) + greHeaderLength;
              //???printf("IPv4+GRE+%d+ERSPAN+ethernet: ", greHeaderLength); for (int i=0; i<erspanHeaderLength; i++) printf("%02x ", (uint8_t)buffer[i]); printf("\n");
              etherHeader = (struct ethhdr*)( (uint8_t*)(&erspan->etherHeader) + greHeaderLength );
              etherHeaderLength = sizeof(struct ethhdr); // ... plus length of optional VLAN tag ?
              //???printf("inner ethernet: "); for (int i=0; i<etherHeaderLength; i++) printf("%02x ", ((uint8_t*)etherHeader)[i]); printf("\n");
              buffer += sizeof(struct ERSPAN2Headers) + greHeaderLength;
              length -= sizeof(struct ERSPAN2Headers) + greHeaderLength;
            }
        }


        // if the buffer contains an IPv4 packet, overlay an IPv4 header on it
        if ( etherType == ETH_P_IP && length>=sizeof(struct ip) && ((struct iphdr*)buffer)->version==4 ) {
            ipv4Header = (struct iphdr*)buffer;
            ipv4HeaderLength = ipv4Header->ihl * 4;
            //???printf("IPv4: "); for (int i=0; i<ipv4HeaderLength; i++) printf("%02x ", (uint8_t)buffer[i]); printf("\n"); 
            buffer += ipv4HeaderLength;
            length -= ipv4HeaderLength;
        }

        // if the buffer contains an IPv6 packet, overlay an IPv6 header on it
        if ( etherType ==ETH_P_IPV6 && length>=sizeof(struct ip6_hdr) && (((struct ip6_hdr*)buffer)->ip6_vfc)>>4==6 ) {
            ipv6Header = (struct ip6_hdr*)buffer;
            ipv6HeaderLength = sizeof(struct ip6_hdr); // ... plus length of optional extension headers ?
            //???printf("IPv6: "); for (int i=0; i<ipv6HeaderLength; i++) printf("%02x ", (uint8_t)buffer[i]); printf("\n"); 
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
            //???printf("UDP: "); for (int i=0; i<udpHeaderLength; i++) printf("%02x ", (uint8_t)buffer[i]); printf("\n"); 
            buffer += udpHeaderLength;
            length -= udpHeaderLength;
        }

        // if the buffer contains a TCP packet, and it has a TCP header, overlay a TCP header on it
        if ( ( ipv4Header && ipv4Header->protocol==IPPROTO_TCP && length>=sizeof(struct tcphdr) ) ||
             ( ipv6Header && ipv6Header->ip6_nxt==IPPROTO_TCP && length>=sizeof(struct tcphdr) ) ) {
            tcpHeader = (struct tcphdr*)buffer;
            tcpHeaderLength = tcpHeader->doff * 4;
            //???printf("TCP: "); for (int i=0; i<tcpHeaderLength; i++) printf("%02x ", (uint8_t)buffer[i]); printf("\n"); 
            buffer += tcpHeaderLength;
            length -= tcpHeaderLength;
        }

        // if there is any data left in the buffer, its the packet's payload
        if (length>0) {
            payload = buffer;
            payloadLength = length;
        }
    }

    SPL::list<SPL::uint16> convertVlanTagsToList() {
    	SPL::list<SPL::uint16> vList;
    	int numIds = vlanHeaderLength / sizeof(struct VLANHeader);
    	for (int i = 0;  i < numIds; i++)
    	  vList.push_back( ntohs(vlanHeader[i].vlanTag) & VLANHeader::vlanIdentifier );
    	return vList;
    }

};

#endif /* NETWORK_HEADER_PARSER_H_ */
