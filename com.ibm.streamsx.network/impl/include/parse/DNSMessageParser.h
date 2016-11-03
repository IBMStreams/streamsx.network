/*
** Copyright (C) 2011, 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef DNS_MESSAGE_PARSER_H_
#define DNS_MESSAGE_PARSER_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string>

#include <SPL/Runtime/Type/SPLType.h>
#include <SPL/Runtime/Utility/Mutex.h>

////////////////////////////////////////////////////////////////////////////////
// This class maps DNS parser error codes to error descriptions
////////////////////////////////////////////////////////////////////////////////

class DNSMessageParserErrorDescriptions {

 public:

  static const int maximumErrorCode = 200;
  const char* description[maximumErrorCode];

  DNSMessageParserErrorDescriptions() {
    for (int i = 0; i<sizeof(maximumErrorCode); i++) description[i] = "";
    description[102] = "label overruns packet";
    description[103] = "label compression length overruns packet";
    description[104] = "label compression offset underruns packet";
    description[105] = "label compression offset overruns packet";
    description[106] = "label compression offset loop";
    description[107] = "label flags invalid";
    description[108] = "label limit exceeded";
    description[109] = "";
    description[110] = "resource record missing";
    description[111] = "question resource record truncated";
    description[112] = "resource record truncated";
    description[113] = "resource record data truncated";
    description[114] = "invalid address family";
    description[115] = "unexpected resource type";
    description[116] = "message too short";
    description[117] = "counts too large";
  }
};


////////////////////////////////////////////////////////////////////////////////
// This class parses DNS fields within a DNS message
////////////////////////////////////////////////////////////////////////////////

class DNSMessageParser {


 private:

  // These structures define the variable-size format of DNS resource records,
  // as encoded in DNS network packets, according to RFC 1035 (see
  // https://www.ietf.org/rfc/rfc1035.txt)

  static const uint16_t EDNS0_TYPE = 41;
  struct DNSResourceRecord {
    uint8_t name[0];
    uint16_t type;
    uint16_t classs;
    uint32_t ttl;
    uint16_t rdlength;
    uint8_t rdata[0];
  } __attribute__((packed)) ;

  struct DNSHeader {
    uint16_t identifier;
    union {
      struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        uint8_t recursionDesiredFlag:1;
        uint8_t truncatedFlag:1;
        uint8_t authoritativeFlag:1;
        uint8_t opcodeField:4;
        uint8_t responseFlag:1;
        uint8_t responseCode:4;
        uint8_t nonauthenticatedFlag:1;
        uint8_t authenticatedFlag:1;
        uint8_t reserved:1;
        uint8_t recursionAvailableFlag:1;
#elif __BYTE_ORDER == __BIG_ENDIAN
        uint8_t responseFlag:1;
        uint8_t opcodeField:4;
        uint8_t authoritativeFlag:1;
        uint8_t truncatedFlag:1;
        uint8_t recursionDesiredFlag:1;
        uint8_t recursionAvailableFlag:1;
        uint8_t reserved:1;
        uint8_t authenticatedFlag:1;
        uint8_t nonauthenticatedFlag:1;
        uint8_t responseCode:4;
#else
# error "sorry, __BYTE_ORDER not setm check <bits/endian.h>"
#endif
      } indFlags;
      uint16_t allFlags;
    } flags;
    uint16_t questionCount;
    uint16_t answerCount;
    uint16_t nameserverCount;
    uint16_t additionalCount;
    struct DNSResourceRecord rrFields[0]; // see 'struct DNSResourceRecord'
  } __attribute__((packed)) ;


 public:

  // This structure defines the fixed-size copies of variable-size DNS resource
  // records created by the parseDNSMessage() function and returned in the
  // variables below().

  struct Record {
    uint8_t* name;
    uint16_t type;
    uint16_t classs;
    uint32_t ttl;
    uint16_t rdlength;
    uint8_t* rdata;
  };


 private:

  // This function skips over an encoded DNS name in the variable-size DNS
  // resource record located at 'dnsPointer'. If no problems are found, it
  // returns 'true' and 'dnsPointer' is advanced to the next field in the same
  // resource record. If an encoding problem is found, the function returns
  // 'false' and 'dnsPointer' is advanced to the encoding error within the DNS
  // name, and 'error' is set to a description of the problem.

  bool skipDNSEncodedName() {

    // don't proceed if we've already found an encoding error in this DNS message
    if (error) return false;

    // step through the labels in this DNS name, skipping over each one
    for (int32_t i = 0; i<253; i++) {

      // get the length and compression flag from the first byte in the next label
      const uint8_t flags = *dnsPointer & 0xC0;
      const uint8_t length = *dnsPointer & 0x3F;

      // handle compressed and uncompressed labels differently
      uint16_t offset;
      switch (flags) {

        // for uncompressed labels, step over the label length byte and text, and then
        // continue with the next label
      case 0x00:
        if (dnsPointer+1+length>dnsEnd) { error = 102; return false; } // ... "label overruns packet"
        if (length==0) { dnsPointer++; return true; }
        dnsPointer += 1+length;
        break;

        // for compressed labels, step over the offset and return, since compressed
        // labels are always the last in DNS name
      case 0xC0:
        if (dnsPointer+2>dnsEnd) { error = 103; return false; } // ... "label compression length overruns packet"
        offset = ntohs(*((uint16_t*)dnsPointer)) & 0x03FF;
        if (offset<sizeof(DNSHeader)) { error = 104; return false; } // ... "label compression offset underruns packet"
        if (offset==dnsPointer-dnsStart) { error = 106; return false; } // ... "label compression offset loop"
        if (dnsStart+offset>dnsEnd) { error = 105; return false; } // ... "label compression offset overruns packet"
        dnsPointer += 2;
        return true;
        break;

        // no DNS label should have other high-order bit settings in its length byte
      default:
        error = 107; return false; // ... "label flags invalid"
      }
    }

    // no DNS name can have have this many labels in it
    error = 108; // ... "label limit exceeded"
    return false;
  }

  // This function parses one variable-size DNS resource record located at
  // 'dnsPointer' and copies its fields into the fixed-size temporary resource
  // record located at 'record'. If no parsing problems are found, it returns
  // 'true', and 'dnsPointer' has been advanced to the next record, and 'record'
  // contains a complete copy. If parsing problems are found, the function
  // returns 'false', and 'error' is set to a description of the problem, and
  // 'dnsPointer' has been advanced only to the field in error, and 'record' is
  // not complete.

  bool copyResourceRecord(struct Record& record, const bool fullResource) {

    // don't proceed if we've already found an encoding error in this DNS message
    if (error) return false;

    // don't proceed if we've reached the end of the DNS message
    if (dnsPointer>=dnsEnd) { error = 110; return false; } // ... "resource record missing"

    // copy the address of this resource's name into the fixed-size structure, and
    // then step over it
    record.name = dnsPointer;
    if (!skipDNSEncodedName()) return false;

    // check for truncated resource record
    if (!fullResource && dnsPointer+4>dnsEnd) { error = 111; return false; } // ... "question resource record truncated"
    if (fullResource && dnsPointer+sizeof(DNSResourceRecord)>dnsEnd) { error = 112; return false; } // ... "resource record truncated"

    // copy the type and class of this resource into the fixed-size structure
    struct DNSResourceRecord* rr = (struct DNSResourceRecord*)dnsPointer;
    record.type = ntohs(rr->type);
    record.classs = ntohs(rr->classs);

    // if this is a question resource, that's all there is
    if (!fullResource) { dnsPointer += 4; return true; }

    // copy remaining resource fields to fixed-size structure
    record.ttl = ntohl(rr->ttl);
    record.rdlength = ntohs(rr->rdlength);
    record.rdata = rr->rdata;

    // check for truncated resource record
    if (record.rdata+record.rdlength>dnsEnd) { error = 113; return false; } // ... "resource record data truncated"

    // step over the remainder of this resource record
    dnsPointer += sizeof(struct DNSResourceRecord) + record.rdlength;
    return true;
  }

  // This function parses a sequence of 'resourceCount' variable-size DNS
  // resource record located at 'dnsPointer' and copies them into the array of
  // fixed-size records located at 'records'. For 'question' records that do not
  // have 'tt' or 'rdata' fields, 'fullResources' should be set to 'false'; for
  // all other records, 'fulRecords' should be set to 'true'. If no parsing
  // problems are found, it returns 'resourceCount', and 'dnsPointer' has been
  // advanced to the next DNS resource record, and 'records' contains complete
  // copies. If parsing problems are found, the function returns the number of
  // DNS resource records successfully copied, and 'error' is set to a
  // description of the problem, and 'dnsPointer' has been advanced only to the
  // DNS resource record in error, and 'records' is not complete.

  int copyResourceRecords(struct Record records[], const uint16_t resourceCount, const bool fullResources) {

    for (int i=0; i<resourceCount; i++) { if (!copyResourceRecord(records[i], fullResources)) return i; }
    return resourceCount;
  }


 public:

  // The parseDNSMessage() function below returns the address of the DNS message
  // header in this variable.

  struct DNSHeader* dnsHeader;

  // The parseDNSMessage() function below keeps track of its progress while
  // parsing the DNS message with these variables.

  uint8_t* dnsStart;
  uint8_t* dnsEnd;
  uint8_t* dnsPointer;

  // The parseDNSMessage() function below returns the counts of each type of
  // resource record specified in the DNS message's header in these
  // variables. Note that the actual number of resource records returned by the
  // function may be smaller if the message has been truncated or contains
  // encoding errors.

  uint16_t questionCount;
  uint16_t answerCount;
  uint16_t canonicalCount;
  uint16_t addressCount;
  uint16_t nameserverCount;
  uint16_t additionalCount;

  // The parseDNSMessage() function below returns the actual number of each type
  // of resource record returned in these variables. That is, these variables
  // give the the number of records in the corresponding arrays below.

  int questionRecordCount;
  int answerRecordCount;
  int canonicalRecordCount;
  int addressRecordCount;
  int nameserverRecordCount;
  int additionalRecordCount;

  // The parseDNSMessage() function below returns each type of resource record
  // in these variables. The number of records of each type is returned in the
  // variables above.

  static const uint32_t MAXIMUM_RRFIELDS = 64;
  struct Record questionRecords[MAXIMUM_RRFIELDS];
  struct Record answerRecords[MAXIMUM_RRFIELDS];
  struct Record nameserverRecords[MAXIMUM_RRFIELDS];
  struct Record additionalRecords[MAXIMUM_RRFIELDS];
  struct Record canonicalRecords[MAXIMUM_RRFIELDS];
  struct Record addressRecords[MAXIMUM_RRFIELDS];

  // The parseDNSMessage() function below returns an error code in this
  // variable, if an encoding error is found, or 0, if no errors are
  // found. Descriptions for the error codes are in the table.

  int error;
  DNSMessageParserErrorDescriptions errorDescriptions;

  // This function decodes an encoded DNS domain name located at '*p', writes the
  // decoded name in 'nameBuffer', sets '*nameLength' to the number of bytes
  // written, and advances '*p' to the next field. The function does not write a
  // trailing null byte after the string (that is, it does not write an ASCIIZ
  // string). If an encoding problem is found, 'error' is set to a description
  // of the problem, and a partially decoded DNS name may be left in
  // 'nameBuffer'.

  inline __attribute__((always_inline))
  void decodeDNSEncodedName(uint8_t** p, char* nameBuffer, int* nameLength) { 

    // alternate resource record pointer for '*p' (used for compressed DNS labels)
    uint8_t* pp;

    // step through the labels in the DNS domain name at '*p' and reconstruct it in 'nameBuffer'
    for (int32_t i = 0; i<255; i++) {

      // stop when the label encoding delimiter is reached
      if (**p==0) { (*p)++; break; }

      // no domain name can have this many labels or be this long
      if (i>253) { error = 108; break; }  // ... "label limit exceeded"
      if (*nameLength>253) { error = 102; break; }  // ... "label overruns packet"

      // get the length and compression flag from the first byte in the next label
      const uint8_t flags = **p & 0xC0;
      const uint8_t length = **p & 0x3F;

      // for uncompressed labels, append the text of the label to the string,
      // then step over the label length byte and text, and continue with the
      // next label until one with zero length is found
      if (flags==0x00) {
        if (*p+1+length>dnsEnd) { error = 102; break; } // ... "label overruns packet"
        memcpy(&nameBuffer[*nameLength], (const char*)(*p+1), length); 
        nameBuffer[(*nameLength)+length] = '.'; 
        *p += length + 1;
        *nameLength += length + 1;
      }

      // for compressed labels, get the offset from the beginning of the DNS
      // message to the next label, and continue decoding from there, leaving
      // the caller's '*p' pointing at the next field in the resource record,
      // using an alternate '*p' for the remainder of this DNS name

      else if (flags==0xC0) { 
        if (*p+2>dnsEnd) { error = 103; break; } // ... "label compression length overruns packet"
        const uint16_t offset = ntohs(*((uint16_t*)*p)) & 0x03FF;
        if (offset<sizeof(DNSHeader)) { error = 104; break; } // ... "label compression offset underruns packet"
        if (dnsStart+offset>dnsEnd) { error = 105; break; } // ... "label compression offset overruns packet"
        if (offset==*p-dnsStart) { error = 106; break; } // ... "label compression offset loop"
        *p += 2;
        p = &pp;
        *p = dnsStart + offset;
      }

      // no DNS label should have other high-order bit settings in its length byte
      else {
        error = 107; break; // ... "label flags invalid"
      }
    }

    // back up over the '.' character appended to the last label in the domain name, if there os one
    if (*nameLength>0 && nameBuffer[(*nameLength)-1]=='.') (*nameLength)--;
  }


  // This function decodes an encoded DNS name located at 'p'. If no problems
  // are found, it returns an SPL::rstring containing the decoded name. If an
  // encoding problem is found, the function returns an empty string, or perhaps
  // a partially decoded DNS name, and 'error' is set to a description of the
  // problem.

  inline __attribute__((always_inline))
  SPL::rstring convertDNSEncodedNameToString(uint8_t* p) { 

    // decode the DNS-encoded name at '*p' into a local buffer
    char nameBuffer[4096]; 
    int nameLength = 0;
    decodeDNSEncodedName(&p, nameBuffer, &nameLength);

    // construct a string object containing the decoded DNS name and return it
    return SPL::rstring(nameBuffer, nameLength); 
  }


  // This function converts the SOA resource record at '*p' into a string
  // representation. If 'fieldDelimiter' is a non-empty string, all fields of
  // the record are included in the string, separated by 'fieldDelimiter'. If
  // 'fieldDelimiter' is an empty string, only the first field is included.

  inline __attribute__((always_inline))
  SPL::rstring convertSOAResourceDataToString(uint8_t* p, const char* fieldDelimiter) { 

    // buffer for string representation of resource record
    char stringBuffer[4096]; 
    int stringLength = 0;

    // decode the DNS-encoded 'MNAME' field at '*p' into a buffer
    decodeDNSEncodedName(&p, stringBuffer, &stringLength); 

    // if a field delimiter is specified, append the rest of the record's
    // fields, separated by the specified delimiter
    const int fieldDelimiterLength = strlen(fieldDelimiter);
    if (fieldDelimiterLength) {

      // append the field delimiter to the first field in the buffer
      memcpy(stringBuffer+stringLength, fieldDelimiter, fieldDelimiterLength); 
      stringLength += fieldDelimiterLength;

      // decode the DNS-encoded 'RNAME' field at '*p' into the buffer
      decodeDNSEncodedName(&p, stringBuffer, &stringLength); 

      // format the five unsigned integers in the remainder of the resource record
      const uint32_t* q = (uint32_t*)p;
      stringLength += sprintf(stringBuffer+stringLength, 
                              "%s%u%s%u%s%u%s%u%s%u", 
                              fieldDelimiter,
                              ntohl(q[0]),
                              fieldDelimiter,
                              ntohl(q[1]),
                              fieldDelimiter,
                              ntohl(q[2]),
                              fieldDelimiter,
                              ntohl(q[3]),
                              fieldDelimiter,
                              ntohl(q[4]) );
    }

    // construct a string object containing the fields in the buffer and return it
    return SPL::rstring(stringBuffer, stringLength); 
  }


  // This function converts the encoded DNS names in the array of 'count'
  // fixed-size records at 'records' into an SPL list of SPL strings. If no
  // problems are found, it returns a list of 'count' strings. If problems are
  // found while decoding the DNS names, 'error' is set to a description of the
  // problem, and the list returned will be incomplete.

  inline __attribute__((always_inline))
  SPL::list<SPL::rstring> convertResourceNamesToStringList(const struct Record records[], const uint16_t count) {

    SPL::list<SPL::rstring> strings;
    for (int i=0; i<count; i++) strings.add(convertDNSEncodedNameToString(records[i].name));
    return strings;
  }


  // This function converts the DNS types in the array of 'count' fixed-size records
  // at 'records' into an SPL list of integers.

  inline __attribute__((always_inline))
  SPL::list<SPL::uint16> convertResourceTypesToIntegerList(const struct Record records[], const uint16_t count) {

    SPL::list<SPL::uint16> integers;
    for (int i=0; i<count; i++) integers.add(records[i].type);
    return integers;
  }


  // This function converts the DNS classes in the array of 'count' fixed-size
  // records at 'records' into an SPL list of integers.

  inline __attribute__((always_inline))
  SPL::list<SPL::uint16> convertResourceClassesToIntegerList(const struct Record records[], const uint16_t count) {

    SPL::list<SPL::uint16> integers;
    for (int i=0; i<count; i++) integers.add(records[i].classs);
    return integers;
  }


  // This function converts the DNS 'ttl' fields in the array of 'count'
  // fixed-size records at 'records' into an SPL list of integers.

  inline __attribute__((always_inline))
  SPL::list<SPL::uint32> convertResourceTTLsToIntegerList(const struct Record records[], const uint16_t count) {

    SPL::list<SPL::uint32> integers;
    for (int i=0; i<count; i++) integers.add(records[i].ttl);
    return integers;
  }


  // This function converts a binary IPv4 or IPv6 address into a string representation of the 
  // address the first time its seen, and caches the result. Thereafter, the cached string is
  // returned.

  inline __attribute__((always_inline))
  SPL::rstring convertIPAddressToString(const int addressFamily, const void *ipAddress) {

    // these static variables cache the results of previous conversions
    static SPL::Mutex ipv4Mutex;
    static SPL::Mutex ipv6Mutex;
    static SPL::map<uint32_t, SPL::rstring> ipv4Cache;
    static SPL::map<SPL::list<SPL::uint8>, SPL::rstring> ipv6Cache;

    // return the string representation of an IPv4 address, either by reusing a
    // result cached during a previous call, or by converting and caching this one
    if (addressFamily==AF_INET) {
      SPL::AutoMutex m(ipv4Mutex); 

      // reuse the result of a previous conversion, if there is one
      const uint32_t ipv4Address = *(uint32_t*)ipAddress;
      const SPL::map<uint32_t, SPL::rstring>::iterator i = ipv4Cache.find(ipv4Address);
      //???if (i!=ipv4Cache.end()) printf("address cache hit on '%s'\n", (i->second).c_str());
      if (i!=ipv4Cache.end()) return i->second;

      // convert this address to a string, cache the result, and return it
      char buffer[INET_ADDRSTRLEN];
      const SPL::rstring ipv4String(inet_ntop(AF_INET, ipAddress, buffer, sizeof(buffer)));
      ipv4Cache.add(ipv4Address, ipv4String);
      //???printf("address cache miss on '%s'\n", ipv4String.c_str());
      return ipv4String; }

    // return the string representation of an IPv6 address, either by reusing a
    // result cached during a previous call, or by converting and caching this one
    else if (addressFamily==AF_INET6) {
      SPL::AutoMutex m(ipv6Mutex); 

      // reuse the result of a previous conversion, if there is one
      const SPL::list<SPL::uint8> ipv6Address((uint8_t*)ipAddress, (uint8_t*)ipAddress+16);
      const SPL::map<SPL::list<SPL::uint8>, SPL::rstring>::iterator i = ipv6Cache.find(ipv6Address);
      //???if (i!=ipv6Cache.end()) printf("address cache hit on '%s'\n", (i->second).c_str());
      if (i!=ipv6Cache.end()) return i->second;

      // convert this address to a string, cache the result, and return it
      char buffer[INET6_ADDRSTRLEN];
      const SPL::rstring ipv6String(inet_ntop(AF_INET6, ipAddress, buffer, sizeof(buffer))); 
      ipv6Cache.add(ipv6Address, ipv6String);
      //???printf("address cache miss on '%s'\n", ipv6String.c_str());
      return ipv6String; }

    // this should never happen
    else {
      error = 114; // ... "invalid address family"
      return std::string(); 
    }
  }


  // This function converts the DNS 'rdata' field in one fixed-size resource
  // record located at 'record' into an SPL string. The format of the converted
  // string depends upon the 'type' of the resource record. If the 'type' is not
  // recognized, an empty string is returned, and 'error' is set to a
  // description of the problem.

  inline __attribute__((always_inline))
  SPL::rstring convertResourceDataToString(const struct Record& record, const SPL::rstring fieldDelimiter = SPL::rstring()) {

    switch(record.type) {
      /* A */          case   1: return convertIPAddressToString(AF_INET, record.rdata); break;
        /* NS */         case   2: return convertDNSEncodedNameToString(record.rdata); break;
        /* CNAME */      case   5: return convertDNSEncodedNameToString(record.rdata); break;
        /* SOA */        case   6: return convertSOAResourceDataToString(record.rdata, fieldDelimiter.c_str()); break; // 7 subfields
        /* NULL */       case  10: return SPL::rstring((char*)record.rdata, record.rdlength); break;
        /* WKS */        case  11: return SPL::rstring("[WKS data]"); break; // multiple subfields
        /* PTR */        case  12: return convertDNSEncodedNameToString(record.rdata); break;
        /* HINFO */      case  13: return SPL::rstring("[HINFO data]"); break; // multiple subfields
        /* MINFO */      case  14: return SPL::rstring("[MINFO data]"); break; // multiple subfields
        /* MX */         case  15: return convertDNSEncodedNameToString(record.rdata + 2); break; // 2 subfields
        /* TXT */        case  16: return SPL::rstring((char*)record.rdata, record.rdlength); break;
        /* AFSDB */      case  18: return convertDNSEncodedNameToString(record.rdata + 2); break;
        /* SIG */        case  24: return SPL::rstring("[SIG data]"); break;
        /* KEY */        case  25: return SPL::rstring("[KEY data]"); break;
        /* AAAA */       case  28: return convertIPAddressToString(AF_INET6, record.rdata); break;
        /* SRV */        case  33: return convertDNSEncodedNameToString(record.rdata + 6); break; // 4 subfields
        /* NAPTR */      case  35: return SPL::rstring("[NAPTR data]"); break; // 6 subfields
        /* EDNS0 */      case  41: return SPL::rstring(""); break; // variable subfields
        /* DS */         case  43: return SPL::rstring("[DS data]"); break; // 4 subfields
        /* SSHFP */      case  44: return SPL::rstring("[SSHFP data]"); break;
        /* IPSECKEY */   case  45: return SPL::rstring("[IPSECKEY data]"); break;
        /* RRSIG */      case  46: return SPL::rstring("[RRSIG data]"); break; // 9 subfields
        /* NSEC */       case  47: return SPL::rstring("[NSEC data]"); break; // 2 subfields
        /* DNSKEY */     case  48: return SPL::rstring("[DNSKEY data]"); break; // 4 subfields
        /* NSEC3 */      case  50: return SPL::rstring("[NSEC3 data]"); break; // 8 subfields
        /* NSEC3PARAM */ case  51: return SPL::rstring("[NSEC3PARAM data]"); break; // 5 subfields
        /* TLSA */       case  52: return SPL::rstring("[TLSA data]"); break;
        /* CDNSKEY */    case  60: return SPL::rstring("[CDNSKEY data]"); break;
        /* SPF */        case  99: return SPL::rstring((char*)record.rdata, record.rdlength); break;
        /* TKEY */       case 249: return SPL::rstring("[TKEY data]"); break;
        /* TSIG */       case 250: return SPL::rstring("[TSIG data]"); break;
                         default:  break;
    }

    error = 115; // ... "unexpected resource type"
    return SPL::rstring();
  }


  // This function converts the DNS 'rdata' fields in the array of 'count'
  // resource records at 'records' into an SPL list of SPL strings.

  inline __attribute__((always_inline))
  SPL::list<SPL::rstring> convertResourceDataToStringList(const struct Record records[], const uint16_t count, const SPL::rstring fieldDelimiter = SPL::rstring()) {

    SPL::list<SPL::rstring> strings;
    for (int i=0; i<count; i++) strings.add(convertResourceDataToString(records[i], fieldDelimiter));
    return strings;
  }


  // This function converts the DNS 'rdata' fields in the array of 'count'
  // resource records at 'records' into an SPL list of IP version 4 addresses.

  inline __attribute__((always_inline))
  SPL::list<SPL::uint32> convertResourceDataToIPv4AddressList(const struct Record records[], const uint16_t count) {

    SPL::list<SPL::uint32> addresses;
    for (int i=0; i<count; i++) if ( records[i].type==1 ) addresses.add( ntohl(*((SPL::uint32*)records[i].rdata)) );
    return addresses;
  }


  // This function returns a non-zero error code if the DNS message contains
  // incompatible flags, mainly for column 23 of the 'flattened' DNS format

  inline __attribute__((always_inline))
  int32_t incompatibleFlags() {
    if (dnsHeader->flags.indFlags.authoritativeFlag && !dnsHeader->flags.indFlags.responseFlag) return 1;
    if (dnsHeader->flags.indFlags.truncatedFlag && !dnsHeader->flags.indFlags.responseFlag) return 2;
    if (dnsHeader->flags.indFlags.authoritativeFlag && dnsHeader->flags.indFlags.truncatedFlag) return 3;
    if (dnsHeader->flags.indFlags.authoritativeFlag && dnsHeader->flags.indFlags.truncatedFlag && dnsHeader->flags.indFlags.recursionDesiredFlag && dnsHeader->flags.indFlags.recursionAvailableFlag) return 4;
    return 0;
  }


  // This function converts the DNS 'rdata' fields in the array of 'count'
  // resource records at 'records' into an SPL list of IP version 6 addresses.

  inline __attribute__((always_inline))
  SPL::list<SPL::list<SPL::uint8> > convertResourceDataToIPv6AddressList(const struct Record records[], const uint16_t count) {

    SPL::list<SPL::list<SPL::uint8> > addresses;
    for (int i=0; i<count; i++) if ( records[i].type==28 ) addresses.add( SPL::list<SPL::uint8>(records[0].rdata, records[0].rdata+16)  );
    return addresses;
  }


  // This function parses the DNS message in the specified buffer and stores the
  // resource records of each type in the arrays above. The individual fields in
  // the resource records can be extracted with the functions above. If an
  // encoding error is found, the 'error' variable is set to a description of
  // the error; otherwise it is set to NULL.

  void parseDNSMessage(char* buffer, int length) {

    // clear return fields
    dnsHeader = NULL;
    dnsStart = NULL;
    dnsEnd = NULL;
    error = 0;
    dnsPointer = NULL;

    questionCount = 0;
    answerCount = 0;
    nameserverCount = 0;
    additionalCount = 0;
    canonicalCount = 0;
    addressCount = 0;

    questionRecordCount = 0;
    answerRecordCount = 0;
    nameserverRecordCount = 0;
    additionalRecordCount = 0;
    canonicalRecordCount = 0;
    addressRecordCount = 0;

    // basic safety checks
    if ( length < sizeof(struct DNSHeader) ) { error = 116; return; } // ... "message too short"
    if ( ntohs( ((struct DNSHeader*)buffer)->questionCount )   > MAXIMUM_RRFIELDS ||
         ntohs( ((struct DNSHeader*)buffer)->answerCount )     > MAXIMUM_RRFIELDS ||
         ntohs( ((struct DNSHeader*)buffer)->nameserverCount ) > MAXIMUM_RRFIELDS ||
         ntohs( ((struct DNSHeader*)buffer)->additionalCount ) > MAXIMUM_RRFIELDS ) { error = 117; return; } // ... "counts too large"

    // store pointers to the DNS message in the buffer
    dnsHeader = (struct DNSHeader*)buffer;
    dnsStart = (uint8_t*)buffer;
    dnsEnd = (uint8_t*)buffer + length;
    dnsPointer = (uint8_t*)dnsHeader->rrFields;

    // save the DNS header's resource record counts
    questionCount = ntohs(dnsHeader->questionCount);
    answerCount = ntohs(dnsHeader->answerCount);
    nameserverCount = ntohs(dnsHeader->nameserverCount);
    additionalCount = ntohs(dnsHeader->additionalCount);

    // parse the variable-size DNS resource records and copy them into fixed-size Record structures
    if ( ( questionRecordCount   = copyResourceRecords(questionRecords,   questionCount,   false) ) < questionCount)    return;
    if ( ( answerRecordCount     = copyResourceRecords(answerRecords,     answerCount,     true ) ) < answerCount)      return;
    if ( ( nameserverRecordCount = copyResourceRecords(nameserverRecords, nameserverCount, true ) ) < nameserverCount)  return;
    if ( ( additionalRecordCount = copyResourceRecords(additionalRecords, additionalCount, true ) ) < additionalCount)  return;

    // copy the CNAME and A/AAAA 'answer' records into separate arrays
    for (int i=0; i<answerRecordCount; i++) {
      switch(answerRecords[i].type) {
      case 1:  // type A record
      case 28: // type AAAA record
        addressRecords[addressRecordCount++] = answerRecords[i]; break;
      case 5:  // type CNAME record
        canonicalRecords[canonicalRecordCount++] = answerRecords[i]; break;
      default: break;
      }
    }
  }

};

#endif /* DNS_MESSAGE_PARSER_H_ */
