/*
** Copyright (C) 2011, 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef DNS_PACKET_FLATTENER_H_
#define DNS_PACKET_FLATTENER_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string>

#include <SPL/Runtime/Type/SPLType.h>
//??? #include <SPL/Runtime/Utility/Mutex.h>

#include "parse/NetworkHeaderParser.h"
#include "parse/DNSMessageParser.h"



////////////////////////////////////////////////////////////////////////////////
// This class parses DNS fields within a DNS packet
////////////////////////////////////////////////////////////////////////////////

class DNSPacketFlattener {



 private:

  // This function converts a four-byte binary representation of an IPv4 address
  // in network byte order (that is, not in host byte order) into the
  // conventional string representation.  The string is written to the specified
  // buffer. The function writes a terminating null byte after the string (that
  // is, it writes an ASCIIZ string).  The function returns the address of the
  // buffer.

  const char* convertIPV4AddressToString(uint32_t ipv4Address, char* buffer) {

    struct textEntry { char text[4]; };
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
    
    char* string = (char*)buffer;
    size_t stringLength = 0;
    
    for(int byteOffset=0; byteOffset<4; byteOffset++) {
      const uint8_t byteValue = (ipv4Address >> ((byteOffset)*8)) & 0xff;
      const int digits = byteValue>=100 ? 3 : byteValue>=10 ? 2 : 1;
      for (int i=0; i<digits; i++, stringLength++) { string[stringLength] = textValue[byteValue].text[i]; }
      string[stringLength++] = byteOffset == 3 ? '\0' : '.';
    }    
    return buffer;
  }



  // This function flattens the specified DNS-encoded domain name into the
  // specified buffer, appends a trailing null character, and folds
  // uppercase charaters to lowercase. If there is no domain name, it stores "."
  // in the buffer.

  const char* flattenDNSEncodedName(DNSMessageParser& parser, uint8_t* dnsEncodedName, char* buffer) {

    // decode DNS-encoded name into buffer
    uint8_t* p = dnsEncodedName;
    int bufferLength = 0;
    parser.decodeDNSEncodedName(&p, buffer, &bufferLength);
    *(buffer+bufferLength) = '\0';

    // fold name to lowercase
    for (char* q = buffer; *q!='\0'; q++) *q = tolower(*q);

    // substitute "." for empty names
    if (*buffer=='\0') strcpy(buffer, ".");
    return buffer;
  }



  // This function flattens the five subfields of an SOA resource record
  // 'rdata' field into the specified buffer, converting numbers to strings as
  // appropriate, folding uppercase characters to lowercase, and appending a
  // trailing null character. The function returns the address of the buffer.

  const char* flattenSOAResourceRecord(DNSMessageParser& parser, uint8_t* rdata, int32_t rdataLength, const char* delimiter, char* buffer) {

    // decode the DNS-encoded name in the first subfield
    uint8_t* p = rdata;
    int bufferLength = 0;
    parser.decodeDNSEncodedName(&p, buffer, &bufferLength);

    // append a delimiter to the first subfield
    strcpy(buffer+bufferLength, delimiter);
    bufferLength += strlen(delimiter);

    // decode another DNS-encoded name in the second subfield
    parser.decodeDNSEncodedName(&p, buffer, &bufferLength); 
    
    // format the five unsigned integers in the remainder of the resource record
    const uint32_t* q = (uint32_t*)p;
    bufferLength += sprintf( buffer+bufferLength, 
                             "%s%u%s%u%s%u%s%u%s%u", 
                             delimiter,
                             ntohl(q[0]),
                             delimiter,
                             ntohl(q[1]),
                             delimiter,
                             ntohl(q[2]),
                             delimiter,
                             ntohl(q[3]),
                             delimiter,
                             ntohl(q[4]) );
    return buffer;
  }



   // This function flattens the two subfields of an MX resource record
  // 'rdata' field into the specified buffer, converting numbers to strings as
  // appropriate, folding uppercase characters to lowercase, and appending a
  // trailing null character. The function returns the address of the buffer.

  const char* flattenMXResourceRecord(DNSMessageParser& parser, uint8_t* rdata, int32_t rdataLength, const char* delimiter, char* buffer) {

    // format the unsigned integer in the first subfield of the resource record
    const uint16_t* p = (uint16_t*)rdata;
    int bufferLength = sprintf( buffer, 
                                "%u%s",
                                ntohs(*p),
                                delimiter );

    // decode the DNS-encoded name in the second subfield of the resource record
    flattenDNSEncodedName(parser, rdata+2, buffer+bufferLength);

    return buffer;
  }



  // This function flattens the four subfields of an SRV resource record
  // 'rdata' field into the specified buffer, converting numbers to strings as
  // appropriate, folding uppercase characters to lowercase, and appending a
  // trailing null character. The function returns the address of the buffer.

  const char* flattenSRVResourceRecord(DNSMessageParser& parser, uint8_t* rdata, int32_t rdataLength, const char* delimiter, char* buffer) {

    // format the unsigned integers in the first three subfields of the resource record
    const uint16_t* p = (uint16_t*)rdata;
    int bufferLength = sprintf( buffer, 
                                "%u%s%u%s%u%s",
                                ntohs(p[0]),
                                delimiter,
                                ntohs(p[1]),
                                delimiter,
                                ntohs(p[2]),
                                delimiter );

    // decode the DNS-encoded name in the fourth subfield of the resource record
    flattenDNSEncodedName(parser, rdata+6, buffer+bufferLength);

    return buffer;
  }


 // This function flattens the specified resource record 'rdata' field into the
  // specified buffer, appends a trailing null character, and folds uppercase
  // characters to lowercase. For SOA resource records, all of the subfields are
  // converted to strings as appropriate, and included in the buffer, separated
  // by the specified delimiter.

  const char* flattenRdataField(DNSMessageParser& parser, uint16_t recordType, uint8_t* rdata, int32_t rdataLength, const char* delimiter, char* buffer) {

    switch(recordType) {
        /* A */          case   1: convertIPV4AddressToString(*((uint32_t*)rdata), buffer); break;
        /* NS */         case   2: flattenDNSEncodedName(parser, rdata, buffer); break;
        /* CNAME */      case   5: flattenDNSEncodedName(parser, rdata, buffer); break;
        /* SOA */        case   6: flattenSOAResourceRecord(parser, rdata, rdataLength, delimiter, buffer);  break;
        /* PTR */        case  12: flattenDNSEncodedName(parser, rdata, buffer);  break;
        /* MX */         case  15: flattenMXResourceRecord(parser, rdata, rdataLength, delimiter, buffer);  break;
        /* TXT */        case  16: memcpy(buffer, rdata, rdataLength); *(buffer+rdataLength) = '\0'; break;
        /* AFSDB */      case  18: flattenDNSEncodedName(parser, rdata+2, buffer);  break;
        /* AAAA */       case  28: inet_ntop(AF_INET6, rdata, buffer, 100);  break;
        /* SRV */        case  33: flattenSRVResourceRecord(parser, rdata, rdataLength, delimiter, buffer);  break;
                         default: *buffer = '\0'; break;
    }
    return buffer;
  }


 public:

  SPL::rstring dnsAllFields(uint32_t captureTime, uint32_t packetLength, NetworkHeaderParser& headers, DNSMessageParser& parser, const char* recordDelimiter, const char* fieldDelimiter, const char* subfieldDelimiter) {

    // allocate a buffer large enough to hold the largest possible string representation of a DNS message
    char buffer[1024*1024];
    size_t bufferLength = 0;

    // allocate buffers for converting names and addresses into strings
    char sourceAddressBuffer[100];
    char destinationAddressBuffer[100];
    char nameBuffer[4096];
    char rdataBuffer[4096];

    // format fields 1 through 14 once, to be copied below before each resource record
    char fields1to14[10*1024];
    const size_t fields1to14Length = snprintf( fields1to14, 
                                               sizeof(fields1to14), 
                                               "%u%s%s%s%s%s%hhu%s%hu%s%hu%s%hu%s%c%s%hhu%s%hhu%s%hu%s%s%s%hu%s%hu%s",
                                               captureTime, // field 1
                                               fieldDelimiter,
                                               convertIPV4AddressToString(headers.ipv4Header->saddr, sourceAddressBuffer), // field 2
                                               fieldDelimiter,
                                               convertIPV4AddressToString(headers.ipv4Header->daddr, destinationAddressBuffer), // field 3
                                               fieldDelimiter,
                                               headers.ipv4Header->protocol, // field 4
                                               fieldDelimiter,
                                               ntohs(headers.udpHeader->source), // field 5
                                               fieldDelimiter,
                                               ntohs(headers.udpHeader->dest), // field 6
                                               fieldDelimiter,
                                               ntohs(parser.dnsHeader->identifier), // field 7
                                               fieldDelimiter,
                                               parser.dnsHeader->flags.indFlags.responseFlag ? 'R' : 'Q', // field 8
                                               fieldDelimiter,
                                               parser.dnsHeader->flags.indFlags.opcodeField, // field 9
                                               fieldDelimiter,
                                               parser.dnsHeader->flags.indFlags.responseCode, // field 10
                                               fieldDelimiter,
                                               ( ( ntohs(parser.dnsHeader->flags.allFlags) & 0x0780 ) >> 7 ), // field 11
                                               fieldDelimiter,
                                               parser.questionRecordCount>0 ? flattenDNSEncodedName(parser, parser.questionRecords[0].name, nameBuffer) : ".", // field 12
                                               fieldDelimiter,
                                               parser.questionRecordCount>0 ? parser.questionRecords[0].type : 0, // field 13
                                               fieldDelimiter,
                                               parser.questionRecordCount>0 ? parser.questionRecords[0].classs : 0, // field 14
                                               fieldDelimiter );

    // format fields 21 through 23 once, to be copied below after each resource record
    char fields21through23[1024];
    const size_t fields21through23Length = snprintf( fields21through23,
                                                     sizeof(fields21through23),
                                                     "%u%s%hu%s%d%s",
                                                     packetLength, // field 21
                                                     fieldDelimiter,
                                                     ( (headers.etherHeader->h_dest[4]<<8) + headers.etherHeader->h_dest[5] ), // field 22
                                                     fieldDelimiter,
                                                     parser.error ? parser.error : parser.incompatibleFlags(), // field 23
                                                     recordDelimiter );
                      
    // if there are no resource records in this DNS message, format an empty resource record
    if ( parser.answerRecordCount==0 && parser.nameserverRecordCount==0 && parser.additionalRecordCount==0 ) {
      memcpy(buffer+bufferLength, fields1to14, fields1to14Length);
      bufferLength += fields1to14Length;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "%s%s%s%s%s%s",
                                fieldDelimiter,
                                fieldDelimiter,
                                fieldDelimiter,
                                fieldDelimiter,
                                fieldDelimiter,
                                fieldDelimiter );
      memcpy(buffer+bufferLength, fields21through23, fields21through23Length);
      bufferLength += fields21through23Length;
    }
    
    // format 'answer' resource records
    for (int32_t i = 0; i<parser.answerRecordCount; i++) {
      memcpy(buffer+bufferLength, fields1to14, fields1to14Length);
      bufferLength += fields1to14Length;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "N%s%s%s%hu%s%hu%s%u%s%s%s",
                                fieldDelimiter,
                                flattenDNSEncodedName(parser, parser.answerRecords[i].name, nameBuffer), // field 16
                                fieldDelimiter,
                                parser.answerRecords[i].type, // field 17
                                fieldDelimiter,
                                parser.answerRecords[i].classs, // field 18
                                fieldDelimiter,
                                parser.answerRecords[i].ttl, // field 19
                                fieldDelimiter,
                                flattenRdataField(parser, parser.answerRecords[i].type, parser.answerRecords[i].rdata, parser.answerRecords[i].rdlength, subfieldDelimiter, rdataBuffer), // field 20
                                fieldDelimiter );

      memcpy(buffer+bufferLength, fields21through23, fields21through23Length);
      bufferLength += fields21through23Length;
    }

    // format 'nameserver' resource records
    for (int32_t i = 0; i<parser.nameserverRecordCount; i++) {
      memcpy(buffer+bufferLength, fields1to14, fields1to14Length);
      bufferLength += fields1to14Length;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "T%s%s%s%hu%s%hu%s%u%s%s%s",
                                fieldDelimiter,
                                flattenDNSEncodedName(parser, parser.nameserverRecords[i].name, nameBuffer), // field 16
                                fieldDelimiter,
                                parser.nameserverRecords[i].type, // field 17
                                fieldDelimiter,
                                parser.nameserverRecords[i].classs, // field 18
                                fieldDelimiter,
                                parser.nameserverRecords[i].ttl, // field 19
                                fieldDelimiter,
                                flattenRdataField(parser, parser.nameserverRecords[i].type, parser.nameserverRecords[i].rdata, parser.nameserverRecords[i].rdlength, subfieldDelimiter, rdataBuffer), // field 20
                                fieldDelimiter );

      memcpy(buffer+bufferLength, fields21through23, fields21through23Length);
      bufferLength += fields21through23Length;
    }

    // format 'additional' resource records
    for (int32_t i = 0; i<parser.additionalRecordCount; i++) {
      memcpy(buffer+bufferLength, fields1to14, fields1to14Length);
      bufferLength += fields1to14Length;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "D%s%s%s%hu%s%hu%s%u%s%s%s",
                                fieldDelimiter,
                                flattenDNSEncodedName(parser, parser.additionalRecords[i].name, nameBuffer), // field 16
                                fieldDelimiter,
                                parser.additionalRecords[i].type, // field 17
                                fieldDelimiter,
                                parser.additionalRecords[i].classs, // field 18
                                fieldDelimiter,
                                parser.additionalRecords[i].ttl, // field 19
                                fieldDelimiter,
                                flattenRdataField(parser, parser.additionalRecords[i].type, parser.additionalRecords[i].rdata, parser.additionalRecords[i].rdlength, subfieldDelimiter, rdataBuffer), // field 20
                                fieldDelimiter );

      memcpy(buffer+bufferLength, fields21through23, fields21through23Length);
      bufferLength += fields21through23Length;
    }

    // return the completed results as an SPL 'rstring' attribute
    return SPL::rstring(buffer, bufferLength-strlen(recordDelimiter));
  }

};

#endif /* DNS_PACKET_FLATTENER_H_ */
