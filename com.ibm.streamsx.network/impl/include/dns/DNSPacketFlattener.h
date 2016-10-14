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
#include <pcap.h>
#include <pcap-bpf.h>

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
  // location. The function writes a terminating null byte after the string
  // (that is, it writes an ASCIIZ string).  The function returns the address of the 
  // beginning of the string.

  const char* convertIPV4AddressToString(const uint32_t ipv4Address, const char* ipv4String) {

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
    
    char* string = (char*)ipv4String;
    size_t stringLength = 0;
    
    for(int byteOffset=0; byteOffset<4; byteOffset++) {
      const uint8_t byteValue = (ipv4Address >> ((byteOffset)*8)) & 0xff;
      const int digits = byteValue>=100 ? 3 : byteValue>=10 ? 2 : 1;
      for (int i=0; i<digits; i++, stringLength++) { string[stringLength] = textValue[byteValue].text[i]; }
      string[stringLength++] = byteOffset == 3 ? 0 : '.';
    }
    
    return ipv4String;
  }
  


 public:



  SPL::rstring dnsAllFields(struct pcap_pkthdr* pcapHeader, NetworkHeaderParser& headers, DNSMessageParser& parser, const char* recordDelimiter, const char* fieldDelimiter, const char* subfieldDelimiter) {

    // allocate a buffer large enough to hold the largest possible string representation of a DNS message
    char buffer[1024*1024];
    size_t bufferLength = 0;

    // buffers for converting the packet source and destination addresses into strings
    char ipv4SourceBuffer[100];
    char ipv4DestinationBuffer[100];
    char questionNameBuffer[4096];
    char answerNameBuffer[4096];
    char nameserverNameBuffer[4096];
    char additionalNameBuffer[4096];
    
    // format fields 1 through 14 once, to be copied below into the result buffer before each resource record
    char fields1to14[10*1024];
    const size_t fields1to14Length = snprintf( fields1to14, 
                                               sizeof(fields1to14), 
                                               "1-to-14: %ld%s%s%s%s%s%hhu%s%hu%s%hu%s%hu%s%c%s%hhu%s%hhu%s%hu%s%s%s%hu%s%hu%s",
                                               pcapHeader->ts.tv_sec, // field 1
                                               fieldDelimiter,
                                               convertIPV4AddressToString(headers.ipv4Header->saddr, ipv4SourceBuffer), // field 2
                                               fieldDelimiter,
                                               convertIPV4AddressToString(headers.ipv4Header->daddr, ipv4DestinationBuffer), // field 3
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
                                               parser.questionRecordCount>0 ? parser.convertDNSEncodedNameToString(parser.questionRecords[0].name, questionNameBuffer) : ".", // field 12
                                               fieldDelimiter,
                                               parser.questionRecordCount>0 ? parser.questionRecords[0].type : 0, // field 13
                                               fieldDelimiter,
                                               parser.questionRecordCount>0 ? parser.questionRecords[0].classs : 0, // field 14
                                               fieldDelimiter );


    // format fields 21 and 22 in a temporary buffer, to be copied below after resource records
    char fields21and22[1024];
    const size_t fields21and22Length = snprintf( fields21and22,
                                                 sizeof(fields21and22),
                                                 " 21-and-22: %u%s%hu%s",
                                                 pcapHeader->len, // field 21
                                                 fieldDelimiter,
                                                 ( (headers.etherHeader->h_dest[4]<<8) + headers.etherHeader->h_dest[5] ), // field 22
                                                 recordDelimiter );
                      

    // if there are no resource records in this DNS message, append empty fields 15 through 20 to the buffer
    if ( parser.answerRecordCount==0 && parser.nameserverRecordCount==0 && parser.additionalRecordCount==0 ) {
      memcpy(buffer+bufferLength, fields1to14, fields1to14Length);
      bufferLength += fields1to14Length;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                " 15-to-20: %s%s%s%s%s%s",
                                fieldDelimiter,
                                fieldDelimiter,
                                fieldDelimiter,
                                fieldDelimiter,
                                fieldDelimiter,
                                fieldDelimiter );
      memcpy(buffer+bufferLength, fields21and22, fields21and22Length);
      bufferLength += fields21and22Length;
    }
    
    // format 'answer' resource records
    for (int32_t i = 0; i<parser.answerRecordCount; i++) {
      memcpy(buffer+bufferLength, fields1to14, fields1to14Length);
      bufferLength += fields1to14Length;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                " 15-to-20: N%s%s%s%hu%s%hu%s%u%s%.*s%s",
                                fieldDelimiter,
                                parser.convertDNSEncodedNameToString(parser.answerRecords[i].name, answerNameBuffer), // field 16
                                fieldDelimiter,
                                parser.answerRecords[i].type, // field 17
                                fieldDelimiter,
                                parser.answerRecords[i].classs, // field 18
                                fieldDelimiter,
                                parser.answerRecords[i].ttl, // field 19
                                fieldDelimiter,
                                /***parser.answerRecords[i].rdlength***/0, parser.answerRecords[i].rdata, // field 20
                                fieldDelimiter );

      memcpy(buffer+bufferLength, fields21and22, fields21and22Length);
      bufferLength += fields21and22Length;
    }

    // format 'nameserver' resource records
    for (int32_t i = 0; i<parser.nameserverRecordCount; i++) {
      memcpy(buffer+bufferLength, fields1to14, fields1to14Length);
      bufferLength += fields1to14Length;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                " 15-to-20: T%s%s%s%hu%s%hu%s%u%s%.*s%s",
                                fieldDelimiter,
                                parser.convertDNSEncodedNameToString(parser.nameserverRecords[i].name, nameserverNameBuffer), // field 16
                                fieldDelimiter,
                                parser.nameserverRecords[i].type, // field 17
                                fieldDelimiter,
                                parser.nameserverRecords[i].classs, // field 18
                                fieldDelimiter,
                                parser.nameserverRecords[i].ttl, // field 19
                                fieldDelimiter,
                                /***parser.nameserverRecords[i].rdlength***/0, parser.nameserverRecords[i].rdata, // field 20
                                fieldDelimiter );

      memcpy(buffer+bufferLength, fields21and22, fields21and22Length);
      bufferLength += fields21and22Length;
    }

    // format 'additional' resource records
    for (int32_t i = 0; i<parser.additionalRecordCount; i++) {
      memcpy(buffer+bufferLength, fields1to14, fields1to14Length);
      bufferLength += fields1to14Length;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                " 15-to-20: D%s%s%s%hu%s%hu%s%u%s%.*s%s",
                                fieldDelimiter,
                                parser.convertDNSEncodedNameToString(parser.additionalRecords[i].name, additionalNameBuffer), // field 16
                                fieldDelimiter,
                                parser.additionalRecords[i].type, // field 17
                                fieldDelimiter,
                                parser.additionalRecords[i].classs, // field 18
                                fieldDelimiter,
                                parser.additionalRecords[i].ttl, // field 19
                                fieldDelimiter,
                                /***parser.additionalRecords[i].rdlength***/0, parser.additionalRecords[i].rdata, // field 20
                                fieldDelimiter );

      memcpy(buffer+bufferLength, fields21and22, fields21and22Length);
      bufferLength += fields21and22Length;
    }



  // format fields 15 to 20 for each resource record
#if 0
      // produce one tuple for each 'answer' resource record
      for (int32 i in range(size(answerNames))) {
        rstring fields15to20 =
          ANSWER + $outerDelimiter +
          ( answerNames[i]!="" ? lower(answerNames[i]) : "." ) + $outerDelimiter +
          (rstring)answerTypes[i] + $outerDelimiter +
          (rstring)answerClasses[i] + $outerDelimiter +
          (rstring)answerTTLs[i] + $outerDelimiter +
          lower(answerData[i]);
        outputTuple.dnsRecord = fields1to14 + fields15to20 + fields21to22;
        submit( outputTuple , FlattenedDNSMessageStream ); }

      // produce one tuple for each 'nameserver' resource record
      for (int32 i in range(size(nameserverNames))) {
        rstring fields15to20 =
          NAMESERVER + $outerDelimiter +
          ( nameserverNames[i]!="" ?  lower(nameserverNames[i]) : "." ) + $outerDelimiter +
          (rstring)nameserverTypes[i] + $outerDelimiter +
          (rstring)nameserverClasses[i] + $outerDelimiter +
          (rstring)nameserverTTLs[i] + $outerDelimiter +
          lower(nameserverData[i]);
        outputTuple.dnsRecord = fields1to14 + fields15to20 + fields21to22;
        submit( outputTuple , FlattenedDNSMessageStream ); }

      // produce one tuple for each 'additional' resource record
      for (int32 i in range(size(additionalNames))) {
        rstring fields15to20 =
          ADDITIONAL + $outerDelimiter +
          ( additionalNames[i]!="" ? lower(additionalNames[i]) : "." ) + $outerDelimiter +
          (rstring)additionalTypes[i] + $outerDelimiter +
          (rstring)additionalClasses[i] + $outerDelimiter +
          (rstring)additionalTTLs[i] + $outerDelimiter +
          lower(additionalData[i]);
        outputTuple.dnsRecord = fields1to14 + fields15to20 + fields21to22;
        submit( outputTuple , FlattenedDNSMessageStream ); }
#endif



      // temporary
      printf("%.*s\n", (int)bufferLength, buffer);

      
      // return the completed results as an SPL 'rstring' attribute
      //???return SPL::rstring(buffer, bufferLength);
      return SPL::rstring();
  }

                            




};

#endif /* DNS_PACKET_FLATTENER_H_ */
