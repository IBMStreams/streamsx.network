/*
** Copyright (C) 2011, 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef DNS_FIELD_LOCATOR_H_
#define DNS_FIELD_LOCATOR_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string>
#include <vector>

#include <SPL/Runtime/Type/SPLType.h>

// This class locates DNS fields within a DNS message

class DNSFieldLocator {

 public:

  // These structures .................................

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
# error	"sorry, __BYTE_ORDER not setm check <bits/endian.h>"
#endif
	uint16_t questionCount;
	uint16_t answerCount;
	uint16_t nameserverCount;
	uint16_t additionalCount;
	struct DNSResourceRecord rrFields[0]; // see 'struct DNSResourceRecord'
  } __attribute__((packed)) ;




  // This structure ................................

  struct ResourceRecord {
	uint8_t* name;
	uint16_t type;
	uint16_t classs;
	uint32_t ttl;
	uint16_t rdlength;
	uint8_t* rdata;
  };




  // This function skips over a DNS name in a resource record, one label at a
  // time, advancing the resource record pointer 'p' as it goes.  The function
  // returns 'true' if the DNS name is encoded correctly, and 'p' is left pointing at
  // the next resource record field.  The function returns 'false' if any encoding error is
  // found, and 'rrPointer' is left pointing at the incorrectly encoded label. 

  bool skipDNSName() {

	// step through the labels in this DNS name, skipping over each one     
	for (int32_t i = 0; i<253; i++) {

	  // get the length and compression flag from the first byte in the next label
	  const uint8_t flags = *rrPointer & 0xC0;
	  const uint8_t length = *rrPointer & 0x3F;

	  // handle compressed and uncompressed labels differently
	  uint16_t offset;
	  switch (flags) {

		// for uncompressed labels, step over the label length byte and text, and then
		// continue with the next label
	  case 0x00:
		if (rrPointer+1+length>dnsEnd) { error = "label overran packet"; return false; }
		if (length==0) { rrPointer++; return true; }
		rrPointer += 1+length;
		break;

		// for compressed labels, step over the offset and return, since compressed
		// labels are always the last in DNS name
	  case 0xC0:
		if (rrPointer+2>dnsEnd) { error = "label compression length overran packet"; return false; }
		offset = ntohs(*((uint16_t*)rrPointer)) & 0x03FF;
		if (offset<sizeof(DNSHeader)) { error = "label compression offset underran packet"; return false; }
		if (offset==rrPointer-dnsStart) { error = "label compression offset loop"; return false; }
		if (dnsStart+offset>dnsEnd) { error = "label compression offset overran packet"; return false; }
		rrPointer += 2;
		return true;
		break;

		// no DNS label should have other high-order bit settings in its length byte
	  default:
		error = "label flags invalid"; return false;
	  }
	}
	
	// no DNS name can have have this many labels in it
	error = "label limit exceeded"; 
	return false;
  }



  // This function decodes a DNS name into a string and returns the string if it is
  // encoded correctly.  If an encoding error is found, the function instead returns
  // a short description of the error.

  SPL::rstring convertDNSNameToString(uint8_t* q) {

	// step through the labels in this DNS name, skipping over each one     
	SPL::rstring name;
	for (int32_t i = 0; i<253; i++) {

	  // no DNS name can be this long
	  if (name.length()>253) { error = "label overran packet"; return name; }

	  // get the length and compression flag from the first byte in the next label
	  const uint8_t flags = *q & 0xC0;
	  const uint8_t length = *q & 0x3F;

	  // handle compressed and uncompressed labels differently
	  uint16_t offset;
	  switch (flags) {

		// for uncompressed labels, append the text of the label to the string, 
		// then step over the label length byte and text, and continue with the next label
	  case 0x00:
		if (q+1+length>dnsEnd) { error = "label overran packet"; return name; }
		if (length==0) { if (name.length()) name.erase(name.length()-1); return name; }
		name.append((char*)q+1, length);
		name.append(1, '.');
		q += 1+length;
		break;

		// for compressed labels, get the offset from the beginning of the DNS message 
		// to the next label, and continue decoding from there
	  case 0xC0:
		if (q+2>dnsEnd) { error = "label compression length overran packet"; return name; }
		offset = ntohs(*((uint16_t*)q)) & 0x03FF;
		if (offset<sizeof(DNSHeader)) { error = "label compression offset underran packet"; return name; }
		if (dnsStart+offset>dnsEnd) { error = "label compression offset overran packet"; return name; }
		if (offset==q-dnsStart) { error = " label compression offset loop"; return name; }
		q = dnsStart+offset;
		break; 

		// no DNS label should have other high-order bit settings in its length byte
	  default:
		error = "label flags invalid"; return name;
	  }
	}

	// no DNS name can have have this many labels in it
	error = "label limit exceeded"; 
	return name;
  }


  // This function ..............................

  SPL::rstring convertResourceDataToString(const struct ResourceRecord& record) {

	SPL::rstring empty;
	if (record.classs!=1) { error = "unexpected resource class"; return empty; }

	char buffer[INET_ADDRSTRLEN];
	char buffer6[INET6_ADDRSTRLEN];
	switch(record.type) {
	  /* A */     case 1: return inet_ntop(AF_INET, record.rdata, buffer, sizeof(buffer)); break;
		/* NS */    case 2:
		/* CNAME */ case 5:
		/* SOA */   case 6:
		/* PTR */   case 12: return convertDNSNameToString(record.rdata); break;
		/* MX */    case 15: return convertDNSNameToString(record.rdata + 2); break;
		/* TXT */   case 16: return std::string((char*)record.rdata, record.rdlength); break;
		/* AFSDB */ case 18: return convertDNSNameToString(record.rdata + 2); break;
		/* AAAA */  case 28: return inet_ntop(AF_INET6, record.rdata, buffer6, sizeof(buffer6)); break;
		/* SRV */   case 33: return std::string("[SRV data]"); break;
	default: break;
	}

	error = "unexpected resource type"; 
	return empty;
  }



  // This function ..............................

  SPL::list<SPL::rstring> convertResourceNamesToStringList(const struct ResourceRecord records[], const uint16_t count) {
   
	SPL::list<SPL::rstring> strings;
	for (int i=0; i<count; i++) strings.add(convertDNSNameToString(records[i].name));
	return strings;
  }


  // This function ..............................

  SPL::list<SPL::uint16> convertResourceTypesToIntegerList(const struct ResourceRecord records[], const uint16_t count) {

	SPL::list<SPL::uint16> integers;
	for (int i=0; i<count; i++) integers.add(records[i].type); 
	return integers;
  }



  // This function ..............................

  SPL::list<SPL::uint16> convertResourceClassesToIntegerList(const struct ResourceRecord records[], const uint16_t count) {

	SPL::list<SPL::uint16> integers;
	for (int i=0; i<count; i++) integers.add(records[i].classs); 
	return integers;
  }


  // This function ..............................

  SPL::list<SPL::uint32> convertResourceTTLsToIntegerList(const struct ResourceRecord records[], const uint16_t count) {

	SPL::list<SPL::uint32> integers;
	for (int i=0; i<count; i++) integers.add(records[i].ttl);
	return integers;
  }


  // This function ..............................

  SPL::list<SPL::rstring> convertResourceDataToStringList(const struct ResourceRecord records[], const uint16_t count) {

	SPL::list<SPL::rstring> strings;
	for (int i=0; i<count; i++) strings.add(convertResourceDataToString(records[i])); 
	return strings;
  }



  // This function ..............................

  bool locateResourceRecord(struct ResourceRecord& record, const bool fullRecord) {

	if (rrPointer>=dnsEnd) { error = "resource record missing"; return false; }

	record.name = rrPointer;
	if (!skipDNSName()) return false;

	if (!fullRecord && rrPointer+4>dnsEnd) { error = "resource record truncated"; return false; }
	if (fullRecord && rrPointer+sizeof(DNSResourceRecord)>dnsEnd) { error = "resource record truncated"; return false; }

	struct DNSResourceRecord* rr = (struct DNSResourceRecord*)rrPointer;
	record.type = ntohs(rr->type);
	record.classs = ntohs(rr->classs);

	if (!fullRecord) { rrPointer += 4; return true; }

	record.ttl = ntohl(rr->ttl);
	record.rdlength = ntohs(rr->rdlength);
	record.rdata = rr->rdata;
	if (record.rdata+record.rdlength>dnsEnd) { error = "resource record data truncated"; return false; }

	rrPointer += sizeof(struct DNSResourceRecord) + record.rdlength;
	return true;
  }



  // This function ..............................

  bool locateResourceRecords(struct ResourceRecord records[], const uint16_t recordCount, const bool fullRecords) {

	for (int i=0; i<recordCount; i++) { if (!locateResourceRecord(records[i], fullRecords)) return false; }
	return true;
  }


  // These variables .................................

  uint8_t* dnsStart;
  uint8_t* dnsEnd;
  uint8_t* rrPointer;
  char* dnsBuffer; // address of DNS message
  int dnsBufferLength; // length of DNS message, possibly truncated

  struct DNSHeader* dnsHeader;

  uint16_t questionCount;
  uint16_t answerCount;
  uint16_t nameserverCount;
  uint16_t additionalCount;
  uint16_t canonicalCount;
  uint16_t addressCount;

  static const uint32_t MAXIMUM_RRFIELDS = 64;

  struct ResourceRecord questionRecords[MAXIMUM_RRFIELDS];
  struct ResourceRecord answerRecords[MAXIMUM_RRFIELDS];
  struct ResourceRecord nameserverRecords[MAXIMUM_RRFIELDS];
  struct ResourceRecord additionalRecords[MAXIMUM_RRFIELDS];
  struct ResourceRecord canonicalRecords[MAXIMUM_RRFIELDS];
  struct ResourceRecord addressRecords[MAXIMUM_RRFIELDS];

  char const* error;




  // This function locates DNS fields within a DNS message and sets the member variables above.

  void locateDNSFields(char* buffer, int length) {

	// store DNS buffer address and length
	dnsBuffer = buffer;
	dnsBufferLength = length;

	// clear return fields
	dnsHeader = NULL;
	questionCount = 0;
	answerCount = 0;
	nameserverCount = 0;
	additionalCount = 0;
	canonicalCount = 0; 
	addressCount = 0; 
	error = NULL;

	// basic safety checks
	{
	  if (length<sizeof(struct DNSHeader)) return;
	  struct DNSHeader* header = (struct DNSHeader*)buffer;
	  if ( ntohs(header->questionCount)   > MAXIMUM_RRFIELDS || 
		   ntohs(header->answerCount)     > MAXIMUM_RRFIELDS || 
		   ntohs(header->nameserverCount) > MAXIMUM_RRFIELDS || 
		   ntohs(header->additionalCount) > MAXIMUM_RRFIELDS ) return;
	}
	
	// store pointer to DNS message header 
	dnsHeader = (struct DNSHeader*)buffer;

	// store pointers to the first and last+1 bytes of the DNS message
	dnsStart = (uint8_t*)buffer;
	dnsEnd = (uint8_t*)buffer + length;

	questionCount = ntohs(dnsHeader->questionCount);
	answerCount = ntohs(dnsHeader->answerCount);
	nameserverCount = ntohs(dnsHeader->nameserverCount);
	additionalCount = ntohs(dnsHeader->additionalCount);

	// store pointer to the first byte of the resource records in the DNS message
	rrPointer = (uint8_t*)dnsHeader->rrFields;

	// parse the variable-size DNSResourceRecord structures into fixed-size ResourceRecord structures
	if (!locateResourceRecords(questionRecords,   questionCount,   false)) return;
	if (!locateResourceRecords(answerRecords,     answerCount,     true))  return;
	if (!locateResourceRecords(nameserverRecords, nameserverCount, true))  return;
	if (!locateResourceRecords(additionalRecords, additionalCount, true))  return;

	// copy the CNAME and A/AAAA/TXT answer records into separate arrays
	for (int i=0; i<answerCount; i++) {
	  switch(answerRecords[i].type) {
	  case 1:  // type A record   
	  case 16: // type TXT record 
	  case 28: // type AAAA record
		addressRecords[addressCount++] = answerRecords[i]; break;
	  case 5:  // type CNAME record
		canonicalRecords[canonicalCount++] = answerRecords[i]; break;
	  default: break;
	  }
	}
	
	// allocate variables for setting attributes
	char ipv4Address[INET_ADDRSTRLEN];
	char ipv6Address[INET6_ADDRSTRLEN];
	

  }

};

#endif /* DNS_FIELD_LOCATOR_H_ */
