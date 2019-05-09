/*
** Copyright (C) 2017  International Business Machines Corporation
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
#include <time.h>
#include <string>
#include <cmath>
#include <algorithm> 

#include <SPL/Runtime/Type/SPLType.h>

#include "parse/NetworkHeaderParser.h"
#include "parse/DNSMessageParser.h"

#pragma GCC diagnostic ignored "-Wmissing-braces"

////////////////////////////////////////////////////////////////////////////////
// This class parses DNS fields within a DNS packet
////////////////////////////////////////////////////////////////////////////////

class DNSPacketFlattener {


 private:


  // This function formats a specified 'Unix epoch' timestamp from an
  // SPL::float64 variable as specified Linux time format in a specified buffer,
  // appends the microsecond portion of the timestamp to the buffer, and returns
  // the address of the buffer.

  char* formatTimestamp(const double timestamp, const char* format, char* buffer) {

    struct tm *tmp;
    time_t seconds = (time_t)timestamp;
    tmp = localtime(&seconds);
    size_t length = strftime(buffer, 100, format, tmp);
    sprintf(buffer+length, ".%06d", (int)((timestamp-floor(timestamp))*1000000.0));
    return buffer; }


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



// this class converts DNS type codes into mnemonic names
// (from https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-4)

class DNSTypeNames {
private:
  std::vector<const char*> typeNames;
public:
  DNSTypeNames()  {
    typeNames.resize(65536, "");
    typeNames[0] = "UNUSED";
    typeNames[1] = "A";
    typeNames[2] = "NS";
    typeNames[3] = "MD";
    typeNames[4] = "MF";
    typeNames[5] = "CNAME";
    typeNames[6] = "SOA";
    typeNames[7] = "MB";
    typeNames[8] = "MG";
    typeNames[9] = "MR";
    typeNames[10] = "NULL";
    typeNames[11] = "WKS";
    typeNames[12] = "PTR";
    typeNames[13] = "HINFO";
    typeNames[14] = "MINFO";
    typeNames[15] = "MX";
    typeNames[16] = "TXT";
    typeNames[17] = "RP";	
    typeNames[18] = "AFSDB";	
    typeNames[19] = "X25";	
    typeNames[20] = "ISDN";	
    typeNames[21] = "RT";	
    typeNames[22] = "NSAP";	
    typeNames[23] = "NSAP-PTR";	
    typeNames[24] = "SIG";	
    typeNames[25] = "KEY";	
    typeNames[26] = "PX";	
    typeNames[27] = "GPOS";	
    typeNames[28] = "AAAA";	
    typeNames[29] = "LOC";	
    typeNames[30] = "NXT";	
    typeNames[31] = "EID";	
    typeNames[32] = "NIMLOC";	
    typeNames[33] = "SRV";	
    typeNames[34] = "ATMA";	
    typeNames[35] = "NAPTR";	
    typeNames[36] = "KX";	
    typeNames[37] = "CERT";	
    typeNames[38] = "A6";	
    typeNames[39] = "DNAME";	
    typeNames[40] = "SINK";	
    typeNames[41] = "OPT";	
    typeNames[42] = "APL";	
    typeNames[43] = "DS";	
    typeNames[44] = "SSHFP";	
    typeNames[45] = "IPSECKEY";	
    typeNames[46] = "RRSIG";	
    typeNames[47] = "NSEC";	
    typeNames[48] = "DNSKEY";	
    typeNames[49] = "DHCID";	
    typeNames[50] = "NSEC3";	
    typeNames[51] = "NSEC3PARAM";	
    typeNames[52] = "TLSA";	
    typeNames[53] = "SMIMEA";	
    typeNames[55] = "HIP";	
    typeNames[56] = "NINFO";	
    typeNames[57] = "RKEY";	
    typeNames[58] = "TALINK";	
    typeNames[59] = "CDS";	
    typeNames[60] = "CDNSKEY";	
    typeNames[61] = "OPENPGPKEY";	
    typeNames[62] = "CSYNC";	
    typeNames[99] = "SPF";		
    typeNames[100] = "UINFO";		
    typeNames[101] = "UID";		
    typeNames[102] = "GID";		
    typeNames[103] = "UNSPEC";		
    typeNames[104] = "NID";		
    typeNames[105] = "L32";		
    typeNames[106] = "L64";		
    typeNames[107] = "LP";		
    typeNames[108] = "EUI48";	
    typeNames[109] = "EUI64";	
    typeNames[249] = "TKEY";	
    typeNames[250] = "TSIG";	
    typeNames[251] = "IXFR";	
    typeNames[252] = "AXFR";	
    typeNames[253] = "MAILB";	
    typeNames[254] = "MAILA";	
    typeNames[255] = "ALL";	
    typeNames[256] = "URI";	
    typeNames[257] = "CAA";	
    typeNames[258] = "AVC";	
    typeNames[32768] = "TA";	
    typeNames[32769] = "DLV"; }
  const char* code2name(uint16_t code) { return typeNames[code]; }
};
DNSTypeNames dnsTypeNames;



// this class converts DNS response codes into mnemonic names
// (from https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-6)

class DNSResponseNames {
private:
  std::vector<const char*> responseNames;
public:
  DNSResponseNames()  {
    responseNames.resize(65535, "");
    responseNames[0] = "NoError";
    responseNames[1] = "FormErr";
    responseNames[2] = "ServFail";
    responseNames[3] = "NXDomain";
    responseNames[4] = "NotImp";
    responseNames[5] = "Refused";
    responseNames[6] = "YXDomain";
    responseNames[7] = "YXRRSet";
    responseNames[8] = "NXRRSet";
    responseNames[9] = "NotAuth";
    responseNames[10] = "NotZone";
    responseNames[16] = "BADVERS";
    responseNames[17] = "BADKEY";
    responseNames[18] = "BADTIME";
    responseNames[19] = "BADMODE";
    responseNames[20] = "BADNAME";
    responseNames[21] = "BADALG";
    responseNames[22] = "BADTRUNC";
    responseNames[23] = "BADCOOKIE"; }
  const char* code2name(uint16_t code) { return responseNames[code]; }
};
DNSResponseNames dnsResponseNames;



  // This function flattens the specified DNS-encoded domain name into the
  // specified buffer, appends a trailing null character.

  const char* flattenDNSEncodedName(DNSMessageParser& parser, uint8_t* dnsEncodedName, uint8_t** dnsNextField, char* buffer) {

    // decode DNS-encoded name into buffer
    uint8_t* p = dnsEncodedName;
    int bufferLength = 0;
    parser.decodeDNSEncodedName(&p, buffer, &bufferLength);
    *(buffer+bufferLength) = '\0';

    // return the address of the next field after the DNS-encoded name, if requested
    if (dnsNextField) *dnsNextField = p;

    return buffer;
  }



  // This function flattens the five subfields of an SOA resource record
  // 'rdata' field into the specified buffer, converting numbers to strings as
  // appropriate, folding uppercase characters to lowercase, and appending a
  // trailing null character. The function returns the address of the buffer.

  const char* flattenSOAResourceRecord(DNSMessageParser& parser, uint8_t* rdata, int32_t rdataLength, const char* delimiter, char* buffer) {

    // decode the DNS-encoded domain names in the first two subfields of the resource record
    uint8_t* p = rdata;
    strcpy(buffer, "(mname=");
    flattenDNSEncodedName(parser, p, &p, buffer+strlen(buffer));
    strcat(buffer, delimiter);
    strcat(buffer, "rname=");
    flattenDNSEncodedName(parser, p, &p, buffer+strlen(buffer));

    // format the five unsigned integers in the remainder of the resource record
    const uint32_t* q = (uint32_t*)p;
    sprintf( buffer+strlen(buffer), 
             "%sserial=%u%srefresh=%u%sretry=%u%sexpire=%u%sminimum=%u)", 
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
                                "(preference=%u%sexchange=",
                                ntohs(*p),
                                delimiter );

    // decode the DNS-encoded name in the second subfield of the resource record
    flattenDNSEncodedName(parser, rdata+2, NULL, buffer+bufferLength);
    strcat(buffer, ")");

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
                                "(priority=%u%sweight=%u%sport=%u%starget=",
                                ntohs(p[0]),
                                delimiter,
                                ntohs(p[1]),
                                delimiter,
                                ntohs(p[2]),
                                delimiter );

    // decode the DNS-encoded name in the fourth subfield of the resource record
    flattenDNSEncodedName(parser, rdata+6, NULL, buffer+bufferLength);
    strcat(buffer, ")");

    return buffer;
  }


 // This function flattens the specified resource record 'rdata' field into the
  // specified buffer, appends a trailing null character, and folds uppercase
  // characters to lowercase. For SOA resource records, all of the subfields are
  // converted to strings as appropriate, and included in the buffer, separated
  // by the specified delimiter.

  const char* flattenRdataField(DNSMessageParser& parser, uint16_t recordType, uint8_t* rdata, int32_t rdataLength, const char* delimiter, char* buffer, size_t *length = NULL) {

    if( rdataLength == 0 ) { *buffer = '\0'; if(length) *length = 0; return buffer; }

    switch(recordType) {
        /* A */          case   1: convertIPV4AddressToString(*((uint32_t*)rdata), buffer); break;
        /* NS */         case   2: flattenDNSEncodedName(parser, rdata, NULL, buffer); break;
        /* CNAME */      case   5: flattenDNSEncodedName(parser, rdata, NULL, buffer); break;
        /* SOA */        case   6: flattenSOAResourceRecord(parser, rdata, rdataLength, delimiter, buffer);  break;
        /* PTR */        case  12: flattenDNSEncodedName(parser, rdata, NULL, buffer);  break;
        /* MX */         case  15: flattenMXResourceRecord(parser, rdata, rdataLength, delimiter, buffer);  break;
        /* TXT */        case  16: memcpy(buffer, rdata, rdataLength); *(buffer+rdataLength) = '\0'; break;
        /* AFSDB */      case  18: flattenDNSEncodedName(parser, rdata+2, NULL, buffer);  break;
        /* AAAA */       case  28: inet_ntop(AF_INET6, rdata, buffer, 100);  if(length) *length = strlen(buffer); break;
        /* SRV */        case  33: flattenSRVResourceRecord(parser, rdata, rdataLength, delimiter, buffer);  break;
        /* OPT */        case  41: memcpy(buffer, rdata, rdataLength); *(buffer+rdataLength) = '\0'; break;
        /* SPF */        case  99: memcpy(buffer, rdata, rdataLength); *(buffer+rdataLength) = '\0'; break;
                         default: *buffer = '\0'; break;
    }
    return buffer;
  }


 public:

  SPL::rstring dnsAllFields(double captureTime, uint32_t packetLength, NetworkHeaderParser& headers, DNSMessageParser& parser, const char* recordDelimiter, const char* fieldDelimiter, const char* subfieldDelimiter, SPL::list<SPL::uint16>& rrTypes) {
    return dnsAllFields(captureTime, packetLength, headers.ipv4Header->saddr, headers.ipv4Header->daddr, headers.udpHeader->source, headers.udpHeader->dest, parser, recordDelimiter, fieldDelimiter, subfieldDelimiter, rrTypes);
  }

  SPL::rstring dnsAllFields(double captureTime, uint32_t packetLength, uint32_t srcAddr, uint32_t dstAddr, uint16_t udpSrcPort, uint16_t udpDstPort, DNSMessageParser& parser, const char* recordDelimiter, const char* fieldDelimiter, const char* subfieldDelimiter, SPL::list<SPL::uint16>& rrTypes) {
    // allocate a buffer large enough to hold the largest possible string representation of a DNS message
    char buffer[1024*1024];
    size_t bufferLength = 0;

    // allocate buffers for converting names and addresses into strings
    char sourceAddressBuffer[100];
    char destinationAddressBuffer[100];
    char timestampBuffer[100];
    char nameBuffer[4096];
    char rdataBuffer[4096];

    // format network header fields, plus parser error 
    bufferLength += snprintf( buffer+bufferLength,
                              sizeof(buffer)-bufferLength, 
                              "%s%ssource=%s:%hu%sdestination=%s:%hu parseError=%d,'%s'%s",
                              formatTimestamp(captureTime, "%Y-%m-%d %H:%M:%S", timestampBuffer), 
                              fieldDelimiter,
                              convertIPV4AddressToString(srcAddr, sourceAddressBuffer),
                              ntohs(udpSrcPort), 
                              fieldDelimiter,
                              convertIPV4AddressToString(dstAddr, destinationAddressBuffer), 
                              ntohs(udpDstPort), 
                              parser.error,
                              parser.errorDescriptions.description[parser.error],
                              recordDelimiter );

    // format DNS header fields
    bufferLength += snprintf( buffer+bufferLength,
                              sizeof(buffer)-bufferLength, 
                              "    DNS%s%s%sidentifier=%hu%sresponseCode=%hhu,%s%sflags=0x%04x%squestionCount=%hu%sanswerCount=%hu%snameserverCount=%hu%sadditionalCount=%hu%s",
                              fieldDelimiter,
                              parser.dnsHeader->flags.indFlags.responseFlag ? "Response" : "Query",
                              fieldDelimiter,
                              ntohs(parser.dnsHeader->identifier),
                              fieldDelimiter,
                              parser.dnsHeader->flags.indFlags.responseCode,
                              dnsResponseNames.code2name(parser.dnsHeader->flags.indFlags.responseCode), 
                              fieldDelimiter,
                              ntohs(parser.dnsHeader->flags.allFlags),
                              fieldDelimiter,
                              parser.questionRecordCount,
                              fieldDelimiter,
                              parser.answerRecordCount,
                              fieldDelimiter,
                              parser.nameserverRecordCount,
                              fieldDelimiter,
                              parser.additionalRecordCount,
                              recordDelimiter );

    // format 'question' resource records
    for (int32_t i = 0; i<parser.questionRecordCount; i++) {
      if ( !rrTypes.empty() && std::find(rrTypes.begin(), rrTypes.end(), parser.questionRecords[i].type) == rrTypes.end() ) continue;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "    question.%d%stype=%hu,%s%sname=%s%s",
                                i,
                                fieldDelimiter,
                                parser.questionRecords[i].type, 
                                dnsTypeNames.code2name(parser.questionRecords[i].type), 
                                fieldDelimiter,
                                flattenDNSEncodedName(parser, parser.questionRecords[i].name, NULL, nameBuffer),
                                recordDelimiter ); }

    // format 'answer' resource records
    for (int32_t i = 0; i<parser.answerRecordCount; i++) {
      if ( !rrTypes.empty() && std::find(rrTypes.begin(), rrTypes.end(), parser.answerRecords[i].type) == rrTypes.end() ) continue;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "    answer.%d%stype=%hu,%s%sname=%s%sttl=%u%srdata=%s%s",
                                i,
                                fieldDelimiter,
                                parser.answerRecords[i].type, 
                                dnsTypeNames.code2name(parser.answerRecords[i].type), 
                                fieldDelimiter,
                                flattenDNSEncodedName(parser, parser.answerRecords[i].name, NULL, nameBuffer), 
                                fieldDelimiter,
                                parser.answerRecords[i].ttl, 
                                fieldDelimiter,
                                flattenRdataField(parser, parser.answerRecords[i].type, parser.answerRecords[i].rdata, parser.answerRecords[i].rdlength, subfieldDelimiter, rdataBuffer),
                                recordDelimiter ); }

    // format 'nameserver' resource records
    for (int32_t i = 0; i<parser.nameserverRecordCount; i++) {
      if ( !rrTypes.empty() && std::find(rrTypes.begin(), rrTypes.end(), parser.nameserverRecords[i].type) == rrTypes.end() ) continue;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "    nameserver.%d%stype=%hu,%s%sname=%s%sttl=%u%srdata=%s%s",
                                i,
                                fieldDelimiter,
                                parser.nameserverRecords[i].type, 
                                dnsTypeNames.code2name(parser.nameserverRecords[i].type), 
                                fieldDelimiter,
                                flattenDNSEncodedName(parser, parser.nameserverRecords[i].name, NULL, nameBuffer), 
                                fieldDelimiter,
                                parser.nameserverRecords[i].ttl, 
                                fieldDelimiter,
                                flattenRdataField(parser, parser.nameserverRecords[i].type, parser.nameserverRecords[i].rdata, parser.nameserverRecords[i].rdlength, subfieldDelimiter, rdataBuffer),
                                recordDelimiter ); }

    // format 'additional' resource records
    for (int32_t i = 0; i<parser.additionalRecordCount; i++) {
      if ( !rrTypes.empty() && std::find(rrTypes.begin(), rrTypes.end(), parser.additionalRecords[i].type) == rrTypes.end() ) continue;
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "    additional.%d%stype=%hu,%s%sname=%s%sttl=%u%srdata=%s%s",
                                i,
                                fieldDelimiter,
                                parser.additionalRecords[i].type, 
                                dnsTypeNames.code2name(parser.additionalRecords[i].type), 
                                fieldDelimiter,
                                flattenDNSEncodedName(parser, parser.additionalRecords[i].name, NULL, nameBuffer), 
                                fieldDelimiter,
                                parser.additionalRecords[i].ttl, 
                                fieldDelimiter,
                                flattenRdataField(parser, parser.additionalRecords[i].type, parser.additionalRecords[i].rdata, parser.additionalRecords[i].rdlength, subfieldDelimiter, rdataBuffer), 
                                recordDelimiter ); }

    // format extra data following resource records, if any
    if (parser.error==122) {
      int32_t length = (int32_t)( parser.dnsEnd - parser.dnsPointer );
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "    extra%slength=%d%sdata='",
                                fieldDelimiter,
                                length,
                                fieldDelimiter ); 
      for (int32_t i=0; i<length; i++) {
        bufferLength += snprintf( buffer+bufferLength,
                                  sizeof(buffer)-bufferLength,
                                  "%02x%s",
                                  *(parser.dnsPointer+i),
                                  ( i<length-1 ? fieldDelimiter : "" ) ); }
      bufferLength += snprintf( buffer+bufferLength,
                                sizeof(buffer)-bufferLength,
                                "'%s",
                                recordDelimiter ); }

    // return the completed results as an SPL 'rstring' attribute
    return SPL::rstring(buffer, bufferLength-strlen(recordDelimiter));
  }

};

#endif /* DNS_PACKET_FLATTENER_H_ */
