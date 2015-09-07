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

// This class locates DNS fields within a DNS message

class DNSMessageParser {

 public:
	
	// These structures define the variable-size format of DNS resource records, as
	// encoded in DNS network packets
	
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




  // This structure defines the fixed-size copies of variable-size DNS resource
  // records used temporarily by the parser in this class.

  struct Record {
	uint8_t* name;
	uint16_t type;
	uint16_t classs;
	uint32_t ttl;
	uint16_t rdlength;
	uint8_t* rdata;
  };




  // This function skips over an encoded DNS name in the variable-size DNS resource record located
  // at 'dnsPointer'. If no problems are found, it returns 'true' and 'dnsPointer' is advanced to the 
  // next field in the same resource record. If an encoding problem is found, the function returns 
  // 'false' and 'dnsPointer' is advanced to the encoding error within the DNS name, and 'error' is 
  // set to a description of the problem.

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
		if (dnsPointer+1+length>dnsEnd) { error = "label overruns packet"; return false; }
		if (length==0) { dnsPointer++; return true; }
		dnsPointer += 1+length;
		break;

		// for compressed labels, step over the offset and return, since compressed
		// labels are always the last in DNS name
	  case 0xC0:
		if (dnsPointer+2>dnsEnd) { error = "label compression length overruns packet"; return false; }
		offset = ntohs(*((uint16_t*)dnsPointer)) & 0x03FF;
		if (offset<sizeof(DNSHeader)) { error = "label compression offset underruns packet"; return false; }
		if (offset==dnsPointer-dnsStart) { error = "label compression offset loop"; return false; }
		if (dnsStart+offset>dnsEnd) { error = "label compression offset overruns packet"; return false; }
		dnsPointer += 2;
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


  // This function parses one variable-size DNS resource record located at 'dnsPointer' and copies
  // its fields into the fixed-size temporary resource record located at 'record'. If no parsing
  // problems are found, it returns 'true', and 'dnsPointer' has been advanced to the next record,
  // and 'record' contains a complete copy. If parsing problems are found, the function returns
  // 'false', and 'error' is set to a description of the problem, and 'dnsPointer' has been advanced 
  // only to the field in error, and 'record' is not complete.

  bool copyResourceRecord(struct Record& record, const bool fullResource) {

	  // don't proceed if we've already found an encoding error in this DNS message
	  if (error) return false;

	  // don't proceed if we've reached the end of the DNS message
	  if (dnsPointer>=dnsEnd) { error = "resource record missing"; return false; }

	  // copy the address of this resource's name into the fixed-size structure, and
	  // then step over it
	  record.name = dnsPointer;
	  if (!skipDNSEncodedName()) return false;

	  // check for truncated resource record
	  if (!fullResource && dnsPointer+4>dnsEnd) { error = "question resource record truncated"; return false; }
	  if (fullResource && dnsPointer+sizeof(DNSResourceRecord)>dnsEnd) { error = "resource record truncated"; return false; }

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
	  if (record.rdata+record.rdlength>dnsEnd) { error = "resource record data truncated"; return false; }

	  // step over the remainder of this resource record
	  dnsPointer += sizeof(struct DNSResourceRecord) + record.rdlength;
	  return true;
  }




  // This function parses a sequence of 'resourceCount' variable-size DNS resource record located at 
  // 'dnsPointer' and copies them into the array of fixed-size records located at 'records'. For
  // 'question' records that do not have 'tt' or 'rdata' fields, 'fullResources' should be set to 'false';
  // for all other records, 'fulRecords' should be set to 'true'. If no parsing
  // problems are found, it returns 'resourceCount', and 'dnsPointer' has been advanced to the next DNS resource record,
  // and 'records' contains complete copies. If parsing problems are found, the function returns
  // the number of DNS resource records successfully copied, and 'error' is set to a description of the problem, 
  // and 'dnsPointer' has been advanced only to the DNS resource record in error, and 'records' is not complete.

  int copyResourceRecords(struct Record records[], const uint16_t resourceCount, const bool fullResources) {

	for (int i=0; i<resourceCount; i++) { if (!copyResourceRecord(records[i], fullResources)) return i; }
	return resourceCount;
  }





  // This function decodes an encoded DNS name located at 'q'. If no problems are found, it returns 
  // an STL string containing the decoded name. If an encoding problem is found, the function returns
  // an STL string that is empty or only partially decoded, and 'error' is set to a description of 
  // the problem.

  SPL::rstring convertDNSEncodedNameToString(uint8_t* q) {

	  // step through the labels in this DNS name, reconstructing the name as we go 
	SPL::rstring name;
	for (int32_t i = 0; i<253; i++) {

	  // no DNS name can be this long
	  if (name.length()>253) { error = "label overruns packet"; return name; }

	  // get the length and compression flag from the first byte in the next label
	  const uint8_t flags = *q & 0xC0;
	  const uint8_t length = *q & 0x3F;

	  // handle compressed and uncompressed labels differently
	  uint16_t offset;
	  switch (flags) {

		// for uncompressed labels, append the text of the label to the string, 
		// then step over the label length byte and text, and continue with the next label
	  case 0x00:
		if (q+1+length>dnsEnd) { error = "label overruns packet"; return name; }
		if (length==0) { if (name.length()) name.erase(name.length()-1); return name; }
		name.append((char*)q+1, length);
		name.append(1, '.');
		q += 1+length;
		break;

		// for compressed labels, get the offset from the beginning of the DNS message 
		// to the next label, and continue decoding from there
	  case 0xC0:
		if (q+2>dnsEnd) { error = "label compression length overruns packet"; return name; }
		offset = ntohs(*((uint16_t*)q)) & 0x03FF;
		if (offset<sizeof(DNSHeader)) { error = "label compression offset underruns packet"; return name; }
		if (dnsStart+offset>dnsEnd) { error = "label compression offset overruns packet"; return name; }
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


  // This function converts the encoded DNS names in the array of 'count' fixed-size records 
  // at 'records' into an SPL list of SPL strings. If no problems are found, it returns a list 
  // of 'count' strings. If problems are found while decoding the DNS names, 'error' is set to 
  // a description of the problem, and the list returned will be incomplete.

  SPL::list<SPL::rstring> convertResourceNamesToStringList(const struct Record records[], const uint16_t count) {
   
	SPL::list<SPL::rstring> strings;
	for (int i=0; i<count; i++) strings.add(convertDNSEncodedNameToString(records[i].name));
	return strings;
  }


  // This function converts the DNS types in the array of 'count' fixed-size records 
  // at 'records' into an SPL list of integers. 

  SPL::list<SPL::uint16> convertResourceTypesToIntegerList(const struct Record records[], const uint16_t count) {

	SPL::list<SPL::uint16> integers;
	for (int i=0; i<count; i++) integers.add(records[i].type); 
	return integers;
  }



  // This function converts the DNS classes in the array of 'count' fixed-size records 
  // at 'records' into an SPL list of integers. 

  SPL::list<SPL::uint16> convertResourceClassesToIntegerList(const struct Record records[], const uint16_t count) {

	SPL::list<SPL::uint16> integers;
	for (int i=0; i<count; i++) integers.add(records[i].classs); 
	return integers;
  }


  // This function converts the DNS 'ttl' fields in the array of 'count' fixed-size records 
  // at 'records' into an SPL list of integers. 

  SPL::list<SPL::uint32> convertResourceTTLsToIntegerList(const struct Record records[], const uint16_t count) {

	SPL::list<SPL::uint32> integers;
	for (int i=0; i<count; i++) integers.add(records[i].ttl);
	return integers;
  }


  // This function converts the DNS 'rdata' field in one fixed-size resource record located at 'record'
  // into an SPL string. The format of the converted string depends upon the 'class' and 'type' 
  // of the resource record. If the 'class' or 'type' are not recognized, an empty string is returned,
  // and 'error' is set to a description of the problem.

  SPL::rstring convertResourceDataToString(const struct Record& record) {

	SPL::rstring empty;
	if (record.classs!=1) { error = "unexpected resource class"; return empty; }

	char buffer[INET_ADDRSTRLEN];
	char buffer6[INET6_ADDRSTRLEN];
	switch(record.type) {
	  /* A */     case 1: return inet_ntop(AF_INET, record.rdata, buffer, sizeof(buffer)); break;
		/* NS */    case 2:
		/* CNAME */ case 5:
		/* SOA */   case 6:
		/* PTR */   case 12: return convertDNSEncodedNameToString(record.rdata); break;
		/* MX */    case 15: return convertDNSEncodedNameToString(record.rdata + 2); break;
		/* TXT */   case 16: return std::string((char*)record.rdata, record.rdlength); break;
		/* AFSDB */ case 18: return convertDNSEncodedNameToString(record.rdata + 2); break;
		/* AAAA */  case 28: return inet_ntop(AF_INET6, record.rdata, buffer6, sizeof(buffer6)); break;
		/* SRV */   case 33: return std::string("[SRV data]"); break;
	default: break;
	}

	error = "unexpected resource type"; 
	return empty;
  }



  // This function converts the DNS 'rdata' fields in the array of 'count' fixed-size records 
  // at 'records' into an SPL list of SPL strings. 

  SPL::list<SPL::rstring> convertResourceDataToStringList(const struct Record records[], const uint16_t count) {

	SPL::list<SPL::rstring> strings;
	for (int i=0; i<count; i++) strings.add(convertResourceDataToString(records[i])); 
	return strings;
  }





  // These variables contain the results of the parser function below.

  char* dnsBuffer; // address of DNS message
  int dnsBufferLength; // length of DNS message, possibly truncated

  struct DNSHeader* dnsHeader;
  uint8_t* dnsStart;
  uint8_t* dnsEnd;
  uint8_t* dnsPointer;

  uint16_t questionCount;
  uint16_t answerCount;
  uint16_t canonicalCount;
  uint16_t addressCount;
  uint16_t nameserverCount;
  uint16_t additionalCount;

  static const uint32_t MAXIMUM_RRFIELDS = 64;

  struct Record questionRecords[MAXIMUM_RRFIELDS];
  struct Record answerRecords[MAXIMUM_RRFIELDS];
  struct Record nameserverRecords[MAXIMUM_RRFIELDS];
  struct Record additionalRecords[MAXIMUM_RRFIELDS];
  struct Record canonicalRecords[MAXIMUM_RRFIELDS];
  struct Record addressRecords[MAXIMUM_RRFIELDS];

  int questionRecordCount;
  int answerRecordCount;
  int canonicalRecordCount;
  int addressRecordCount;
  int nameserverRecordCount;
  int additionalRecordCount;

  char const* error;




  // This function parses DNS fields within a DNS message and sets the member variables above.

  void parseDNSMessage(char* buffer, int length) {

	// store DNS buffer address and length
	dnsBuffer = buffer;
	dnsBufferLength = length;

	// clear return fields
	dnsHeader = NULL;
	dnsStart = NULL;
	dnsEnd = NULL;
	error = NULL;
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
	  if (length<sizeof(struct DNSHeader)) return;
	  if ( ntohs( ((struct DNSHeader*)buffer)->questionCount )   > MAXIMUM_RRFIELDS || 
		   ntohs( ((struct DNSHeader*)buffer)->answerCount )     > MAXIMUM_RRFIELDS || 
		   ntohs( ((struct DNSHeader*)buffer)->nameserverCount ) > MAXIMUM_RRFIELDS || 
		   ntohs( ((struct DNSHeader*)buffer)->additionalCount ) > MAXIMUM_RRFIELDS ) return;
	
	// store pointers to the DNS message in the buffer
	dnsHeader = (struct DNSHeader*)buffer;
	dnsStart = (uint8_t*)buffer;
	dnsEnd = (uint8_t*)buffer + length;
	dnsPointer = (uint8_t*)dnsHeader->rrFields;

	// save the DNS resource record counts
	questionCount = ntohs(dnsHeader->questionCount);
	answerCount = ntohs(dnsHeader->answerCount);
	nameserverCount = ntohs(dnsHeader->nameserverCount);
	additionalCount = ntohs(dnsHeader->additionalCount);

	// parse the variable-size DNS resource records and copy them into fixed-size Record structures
	if ( ( questionRecordCount   = copyResourceRecords(questionRecords,   questionCount,   false) ) < questionCount)    return;
	if ( ( answerRecordCount     = copyResourceRecords(answerRecords,     answerCount,     true ) ) < answerCount)      return;
	if ( ( nameserverRecordCount = copyResourceRecords(nameserverRecords, nameserverCount, true ) ) < nameserverCount)  return;
	if ( ( additionalRecordCount = copyResourceRecords(additionalRecords, additionalCount, true ) ) < additionalCount)  return;

	// copy the CNAME and A/AAAA/TXT 'answer' records into separate arrays
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
