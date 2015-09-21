/*
** Copyright (C) 2011, 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef NETFLOW_MESSAGE_PARSER_H_
#define NETFLOW_MESSAGE_PARSER_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <tr1/unordered_map>

#include <SPL/Runtime/Type/SPLType.h>

// suppress " warning: array subscript is above array bounds [-Warray-bounds] " messages
// from GCC version 4.8.3 in RHEL 7.1 ... maybe better to change "xxxx[0]" array declarations 
// to "xxxx[]" declarations ???????

#pragma GCC diagnostic ignored "-Warray-bounds"

////////////////////////////////////////////////////////////////////////////////
// This class locates Netflow fields within a Netflow message
////////////////////////////////////////////////////////////////////////////////

class NetflowMessageParser {

 private:

  // This is the structure of Netflow version 5 messages, according to:
  // http://www.cisco.com/c/en/us/td/docs/net_mgmt/netflow_collection_engine/3-6/user/guide/format.html#wp1006108

  struct Netflow5Flow {
	uint32_t srcAddr; // Source IP address
	uint32_t dstAddr; // Destination IP address
	uint32_t nexthop; // IP address of next hop router
	uint16_t input; // SNMP index of input interface
	uint16_t output; // SNMP index of output interface
	uint32_t packets; // Packets in the flow
	uint32_t octets; // Total number of Layer 3 bytes in the packets of the flow
	uint32_t first; // SysUptime at start of flow
	uint32_t last; // SysUptime at the time the last packet of the flow was received
	uint16_t srcPort; // TCP/UDP source port number or equivalent
	uint16_t dstPort; // TCP/UDP destination port number or equivalent
	uint8_t  pad1; // Unused (zero) byte
	uint8_t  tcpFlags; // Cumulative OR of TCP flags
	uint8_t  prot; // IP protocol type (for example, TCP = 6; UDP = 17)
	uint8_t  tos; // IP type of service (ToS)
	uint16_t srcAS; // Autonomous system number of the source, either origin or peer
	uint16_t dstAS; // Autonomous system number of the destination, either origin or peer
	uint8_t  srcMask; // Source address prefix mask bits
	uint8_t  dstMask; // Destination address prefix mask bits
	uint16_t pad2; // Unused (zero) bytes
  } __attribute__((packed)) ;

  struct Netflow5Header {
	uint16_t version; // '5' in Netflow version 5 messages
	uint16_t count; // number of flows in this Netflow version 5 message (1 to 30)
	uint32_t systemUptime; // elapsed time since sender was booted, in milliseconds
	uint32_t unixSeconds; // time when packet was sent, according to sender's clock, in seconds since UNIX epoch
	uint32_t unixNanoseconds; // time when packet was sent, according to sender's clock, in nonseconds since 'unixSeconds'
	uint32_t flowSequence; // total number of flows sent, according to sender
	uint8_t  engineType; // type of flow-switching engine
	uint8_t  engineID; // slot number of flow-switching engine
	uint16_t samplingInterval; // First two bits hold the sampling mode; remaining 14 bits hold value of sampling interval
	struct Netflow5Flow flows[0];
  } __attribute__((packed)) ;
	
  // This is the struture of Netflow version 9 messages, according to:
  // http://www.cisco.com/en/US/technologies/tk648/tk362/technologies_white_paper09186a00800a3db9.html 
  // https://www.ietf.org/rfc/rfc3954.txt
  // http://www.iana.org/assignments/ipfix/ipfix.xhtml

  struct Netflow9Template {
	uint16_t templateID; // templateID values are >255
	uint16_t fieldCount; // the number of type/length pairs in 'fields' field
	struct { uint16_t type; uint16_t length; } fieldTemplate[0];
  } __attribute__((packed)) ;
	
  struct Netflow9Option {
	uint16_t templateID; // templateID values are >255
	uint16_t scopeLength; // length of 'scopeFields' field
	uint16_t optionLength; // length of 'optionFields' field
	struct { uint16_t type; uint16_t length; } scopeFields[0];
	struct { uint16_t type; uint16_t length; } optionFields[0];
	uint8_t padding[0];
  } __attribute__((packed)) ;
	
  struct Netflow9Flow {
	uint8_t fields[0]; // flow fields (number and length according to template specified by templateID)
	uint8_t padding[0];
  } __attribute__((packed)) ;
	
  struct Netflow9Flowset {
	uint16_t flowsetID; // flowsetID is 0 for templates, 1 for options, >255 for flows
	uint16_t length; // length of 'flowsetID' plus 'length' plus 'templates/options/flows' fields
	union {
	  struct Netflow9Template templates[0];
	  struct Netflow9Option options[0];
	  struct Netflow9Flow flows[0];
	} u;
  } __attribute__((packed)) ;
	
  struct Netflow9Header {
	uint16_t version; // '9' in Netflow version 9 messages
	uint16_t count; // number of records in this message (including templates, options, and flows)
	uint32_t systemMilliseconds; // elapsed time since sender was booted, in milliseconds
	uint32_t unixSeconds; // time when packet was sent, according to sender's clock, in seconds since UNIX epoch
	uint32_t packetSequence; // packet sequence number, according to sender
	uint32_t sourceID; // unique identifier for source of flows, for correlating templates and data
	struct Netflow9Flowset flowsets[0];
  } __attribute__((packed)) ;

  struct NetflowCommonHeader {
	uint16_t version; // version of this Netflow message
	uint16_t count; // number of records in this message (including templates, options, and flows)
  } __attribute__((packed)) ;

  struct NetflowHeader {
	union {
	  struct NetflowCommonHeader common;
	  struct Netflow5Header version5;
	  struct Netflow9Header version9;
	};
  } __attribute__((packed)) ;

  // This table keeps track of the last sequence number in Netflow messages received from 
  // each processor engine in each switch/router device.

  struct SourceState {
	uint32_t sourceAddress; // part of table index
	uint32_t sourceID; // part of table index
	uint32_t previousSequenceNumber;
	uint16_t previousFlowCount;
  };
  std::tr1::unordered_map<uint64_t, struct SourceState*> sourceTable; // indexed by sourceAddress+sourceID

  // This table keeps track of the flow templates received from each processor
  // engine in each switch/router device. Each template is stored as received
  // (in the 'fieldTemplate' variables) and as an offset/length array, indexed
  // by field number (in the 'flowFields' array), for faster access when flow
  // records are parsed.
	
  static const uint16_t FIELD_TEMPLATE_MAXIMUM = 256;
  static const uint16_t FLOW_FIELDS_MAXIMUM = 1024;
  struct TemplateState {
	uint32_t sourceAddress; // part of table index
	uint32_t sourceID; // part of table index
	uint16_t templateID; // part of table index
	uint16_t fieldCount; // number of fields in fieldTemplate
	struct { uint16_t type; uint16_t length; } fieldTemplate[FIELD_TEMPLATE_MAXIMUM+1]; // types and lengths of fields in flows that use this template, indexed by field count
	uint16_t flowLength; // length of flows that use this template
	uint16_t flowTypeMaximum; // largest value of 'type' used in this template
	struct { uint16_t offset; uint16_t length; } flowFields[FLOW_FIELDS_MAXIMUM+1]; // offsets and lengths of fields in flows that use this template, indexed by field type
  };
  std::tr1::unordered_map<uint64_t, struct TemplateState*> templateTable; // indexed by sourceAddress+sourceID+templateID

  // the prepareNetflowMessage() function below stores the IP address of the
  // switch/router that sent the Netflow message in this variable.

  uint32_t sourceAddress; 

  // the prepareNetflowMessage() function below keeps track of the Netflow
  // message being parsed in these variables.

  int messageLength;
  uint8_t* messageStart;
  uint8_t* messageEnd;
  uint64_t messageMissedCount;

  // ?????

  struct Netflow9Flowset* netflow9Flowset;

  // The nextFlowRecord() function keeps track of the template for the current
  // flow in this variable.

  struct TemplateState *templateState;

  // the nextFlowRecord() function keeps track of the number and length of the
  // current flow in these variables

  int flowCount;
  int flowLength;

 public:

  // The prepareNetflowMessage() function below returns the address of the
  // Netflow message header in one of these variables, depending upon its version.
  // The other variable is set to NULL.

  struct Netflow9Header* netflow9Header;
  struct Netflow5Header* netflow5Header;

  // The nextFlowRecord() function below returns the address of the next flow
  // record in one these variables, depending upon its version.  The other
  // variable is set to NULL.
 
  struct Netflow9Flow* netflow9Flow;
  struct Netflow5Flow* netflow5Flow;

  // The prepareNetflowMessage() and nextFlowRecord() functions below set this
  // variable when they find an encoding error in the Netflow message, or set it
  // to NULL if no problems are found.

  char const* error;

 private: 

  // If the current Netflow message is version 5, and there is another flow record in it, this function
  // will return 'true' after setting the 'netflow5Flow' pointer, or else it will return 'false'.


  bool nextFlow5Record() {

	// if we are already parsing the message, and there are more flows in it, return the next one
	if (netflow5Flow && flowCount>1) {
	  //???printf("nextFlow5 A, flowCount=%u ...\n", flowCount);
	  if ( (uint8_t*)netflow5Flow + flowLength > messageEnd ) { error = "netflow5 flow truncated"; return false; }
	  netflow5Flow = (Netflow5Flow*)((uint8_t*)netflow5Flow + flowLength);
	  flowCount--;
	  return true;
	}

	// if we were already parsing the message, but there are no more flows in it, return 'false'
	if (netflow5Flow) return false;

	// if we have not yet started parsing the message, and there is at least one flow in it, return the first one
	flowCount = ntohs(netflow5Header->count);
	flowLength = sizeof(struct Netflow5Flow);
	if (flowCount>0) { 
	  netflow5Flow = netflow5Header->flows;
	  return true;
	}

	return false;
  }

  // This function parses a Netflow version 5 message ...................

  void prepareNetflow5Message() {

	if ( messageLength < sizeof(struct Netflow5Header) ) { error = "netflow5 message too short"; return; }
	netflow5Header = (struct Netflow5Header*)messageStart;

	//???printf("prepareMessage5(), source=0x%08x count=0x%04x sequence=0x%08x seconds=0x%08x nano=0x%08x\n", sourceAddress, ntohs(netflow5Header->count), ntohl(netflow5Header->flowSequence), ntohl(netflow5Header->unixSeconds), ntohl(netflow5Header->unixNanoseconds));

	// find the state structure for this message's source, or create a new one
	const uint32_t sourceID = netflow5Header->engineID;
	const uint64_t index = ((uint64_t)sourceAddress)<<32 | (uint64_t)sourceID;
	struct SourceState* sourceState = sourceTable[index];
	if (!sourceState) {
		sourceState = new SourceState;
		sourceState->sourceAddress = sourceAddress;
		sourceState->sourceID = sourceID;
		sourceState->previousSequenceNumber = 0;
		sourceState->previousFlowCount = 0;
		sourceTable[index] = sourceState;
	}

	// check for missed messages from this message's source
	const uint32_t thisSequenceNumber = ntohl(netflow5Header->flowSequence);
	const uint32_t thisFlowCount = ntohs(netflow5Header->count);
	const uint32_t previousSequenceNumber = sourceState->previousSequenceNumber;
	const uint32_t previousFlowCount = sourceState->previousFlowCount;
	if ( thisSequenceNumber && previousSequenceNumber && thisSequenceNumber!=previousSequenceNumber+previousFlowCount ) {
	  messageMissedCount = thisSequenceNumber - ( previousSequenceNumber + previousFlowCount + 1);
	}
	sourceState->previousSequenceNumber = thisSequenceNumber;
	sourceState->previousFlowCount = thisFlowCount;
  }
  
  // This function stores all of the templates from a Netflow version 9 template flowset in a state table.

  void storeTemplateFlowset() {

	// store each template in this flowset in a separate state table
	//???printf("storeTemplateFlowset() ...\n");
	for ( struct Netflow9Template* netflow9Template = netflow9Flowset->u.templates;
		  (uint8_t*)netflow9Template < (uint8_t*)netflow9Flowset + ntohs(netflow9Flowset->length);
		  netflow9Template = (struct Netflow9Template*)( (uint8_t*)netflow9Template + sizeof(struct Netflow9Template) + ntohs(netflow9Template->fieldCount)*sizeof(netflow9Template->fieldTemplate[0]) ) ) {

	  // find the state table for this template, if there is one, or create a new one if not
	  const uint16_t templateID = ntohs(netflow9Template->templateID);
	  if (templateID<256) { error = "netflow9 templateID too small"; return; }
	  const uint32_t sourceID = ntohl(netflow9Header->sourceID);
	  const uint64_t index = (((uint64_t)sourceAddress)<<32) + ((uint64_t)sourceID<<16) + ((uint64_t)templateID);
	  struct TemplateState *templateState = templateTable[index];
	  if (!templateState) {
		templateState = new TemplateState;
		templateState->sourceAddress = sourceAddress;
		templateState->sourceID = sourceID;
		templateState->templateID = templateID;
		templateState->fieldCount = 0;
		templateState->flowTypeMaximum = 0;
		templateState->flowLength = 0;
		memset( templateState->fieldTemplate, 0, sizeof(templateState->fieldTemplate) );
		memset( templateState->flowFields, 0, sizeof(templateState->flowFields) );
		templateTable[index] = templateState;
	  }

	  // get the number of fields in this template
	  const uint16_t fieldCount = ntohs(netflow9Template->fieldCount);
	  if (fieldCount<1) { error = "netflow9 field count zero"; return; }
	  if (fieldCount>FIELD_TEMPLATE_MAXIMUM) { error = "netflow9 field count too large"; return; }

	  // if this templateID has been parsed before and the template itself is unchanged, don't reparse it
	  const uint32_t templateLength = fieldCount * sizeof(netflow9Template->fieldTemplate[0]);
	  //???printf("storeTemplateFlowset(), sourceAddress=0x%08x sourceID=%d templateID=%u, fieldCount=%u comparison=%d\n", sourceAddress, sourceID, templateID, fieldCount, memcmp(templateState->fieldTemplate, netflow9Template->fieldTemplate, templateLength));
	  if ( memcmp(templateState->fieldTemplate, netflow9Template->fieldTemplate, templateLength) == 0 ) continue;

	  // clear the portion of the field array used by the previous template
	  templateState->flowLength = 0;
	  templateState->flowTypeMaximum = 0;
	  memset( templateState->flowFields, 0, ( templateState->flowTypeMaximum + 1 ) * sizeof(templateState->flowFields[0]) );

	  // store the offset and length of each field from this template in the state table
	  for (int i=0; i<fieldCount; i++) {

		// get the type and length of this field
		uint16_t fieldType = ntohs(netflow9Template->fieldTemplate[i].type);
		uint16_t fieldLength = ntohs(netflow9Template->fieldTemplate[i].length);
		//???printf("template loop, templateID=%u fieldCount=%u i=%d type=%u length=%u offset=%u\n", templateID, fieldCount, i, fieldType, fieldLength, templateState->flowLength);
		if (fieldType>FLOW_FIELDS_MAXIMUM) continue;
		if (!fieldLength) { error = "netflow9 template field length zero"; return; } 
		
		// store the offset and length this field will have in flow records
		templateState->flowFields[fieldType].offset = templateState->flowLength;
		templateState->flowFields[fieldType].length = fieldLength;

		// keep track of the length this flow will have in flow records
		templateState->flowLength += fieldLength;

		// keep track of how much of the flow field array this template uses
		if ( templateState->flowTypeMaximum < fieldType ) templateState->flowTypeMaximum = fieldType;
	  }

	  // store the template itself in the state table
	  templateState->fieldCount = fieldCount;
	  memcpy(templateState->fieldTemplate, netflow9Template->fieldTemplate, templateLength);
	}
  }
  
  // ...................................

  void storeOptionsFlowset() {
  }
    
  // If the current Netflow message is version 5, and there is another flow record in it, this function
  // will return 'true' after setting the 'netflow5Flow' pointer, or else it will return 'false'.

  bool nextFlow9Record() {

	// if we are currently parsing a flowset that contains flows, and there are more flows in it, return the next one
	if (netflow9Flow && flowCount>1) {
	  //???printf("nextFlow9 A, flowCount=%u ...\n", flowCount);
		if ( (uint8_t*)netflow9Flow + flowLength > messageEnd ) { error = "netflow9 flow truncated"; return false; }
		netflow9Flow = (Netflow9Flow*)((uint8_t*)netflow9Flow + flowLength);
		flowCount--;
		return true;
	}

	// or, if we have finished parsing the current flowset, advance to the next flowset,
	// or, if we have not started parsing this Netflow message yet, point at the first flowset in it
	if (netflow9Flowset) {
	  //???printf("nextFlow9 B, flowCount=%u ...\n", flowCount);
		netflow9Flowset = (Netflow9Flowset*)((uint8_t*)netflow9Flowset + ntohs(netflow9Flowset->length));
	} else {
	  //???printf("nextFlow9 C, flowCount=%u ...\n", flowCount);
		netflow9Flowset = netflow9Header->flowsets;
	}
	
	// reset flow-related variables in preparation for parsing the next flowset
	netflow9Flow = NULL;
	templateState = NULL;
	flowCount = 0;
	flowLength = 0;

	// find the next flowset containing flows and return the first flow in it, storing
	// any floatsets that contain templates or options that precede it in the message
	for (; (uint8_t*)netflow9Flowset<messageEnd; netflow9Flowset = (Netflow9Flowset*)((uint8_t*)netflow9Flowset + ntohs(netflow9Flowset->length))) {

	  // check for truncated flowset
	  uint16_t flowsetID = ntohs(netflow9Flowset->flowsetID);
	  uint16_t flowsetLength = ntohs(netflow9Flowset->length);
	  //???printf("nextFlow9 E, flowsetID=%u, flowsetLength=%u ...\n", flowsetID, flowsetLength);
	  if ( flowsetLength < sizeof(Netflow9Flowset) ) { error = "netflow9 flowset too small"; return false; }
	  if ( (uint8_t*)netflow9Flowset + flowsetLength > messageEnd ) { error = "netflow9 flowset truncated"; return false; }

	  // if this flowset contains flows, and we have stored its template, return the first flow in it
	  if (flowsetID>255) { 

		const uint32_t sourceID = ntohl(netflow9Header->sourceID);
		const uint64_t index = (((uint64_t)sourceAddress)<<32) + ((uint64_t)sourceID<<16) + ((uint64_t)flowsetID);
		templateState = templateTable[index];
		if (!templateState) { continue; }

		flowLength = templateState->flowLength;
		if (!flowLength) { error = "netflow9 flow length zero"; return false; }

		flowCount = ntohs(netflow9Flowset->length) / flowLength; 
		//???printf("nextFlow9 F, flowsetID=%u, flowsetLength=%u flowCount=%u...\n", flowsetID, flowsetLength, flowCount);
		if (!flowCount) { error = "netflow9 flowset empty"; return false; }

		netflow9Flow = netflow9Flowset->u.flows;
		return true;
	  }
	  
	  // if this flowset contains templates or options, store them and then try again with the next flowset
	  switch(flowsetID) {
  	      case 0:  storeTemplateFlowset(); break;
	      case 1:  storeOptionsFlowset(); break;
	      default: error = "unsupported flowset type"; break;
	  }
	  if (error) return false;
	}

	// return 'false' when all flowsets in this message have been processed
	return false;
  }

  // This function prepares the parser for a Netflow version 9 message .................

  void prepareNetflow9Message() {

	// point at the Netflow version 9 header structure in the messages
	//???printf("prepareMessage9() ...\n");
	if ( messageLength < sizeof(struct Netflow9Header) ) { error = "header too short"; return; }
	netflow9Header = (struct Netflow9Header*)messageStart;

	// find the state structure for this message's source, or create a new one
	const uint32_t sourceID = ntohl(netflow9Header->sourceID);
	const uint64_t index = ((uint64_t)sourceAddress)<<32 | (uint64_t)sourceID;
	struct SourceState* sourceState = sourceTable[index];
	if (!sourceState) {
		sourceState = new SourceState;
		sourceState->sourceAddress = sourceAddress;
		sourceState->sourceID = sourceID;
		sourceState->previousSequenceNumber = 0;
		sourceState->previousFlowCount = 0;
		sourceTable[index] = sourceState;
	}

	// check for missed messages from this message's source
	const uint32_t thisSequenceNumber = ntohl(netflow9Header->packetSequence);
	const uint32_t previousSequenceNumber = sourceState->previousSequenceNumber;
	if ( thisSequenceNumber && previousSequenceNumber && thisSequenceNumber!=previousSequenceNumber+1 ) {
		messageMissedCount = thisSequenceNumber - previousSequenceNumber - 1;
	}
	sourceState->previousSequenceNumber = thisSequenceNumber;
  }

 public: 

  //.............................

  SPL::uint64 netflow9FieldAsInteger(const uint16_t fieldType) {

	// return zero if there is no such field in this flow
	if ( !netflow9Flow || !templateState || fieldType<1 || fieldType>FLOW_FIELDS_MAXIMUM ) return 0;

	// get the length of the field and its offset within the flow record, according to the template
	uint16_t offset =  templateState->flowFields[fieldType].offset;
	uint16_t length =  templateState->flowFields[fieldType].length;

	// get the value of the field from the flow record as an integer and return it
	uint64_t value = 0;
	switch(length) {
	case 8: value =              netflow9Flow->fields[offset++];
	case 7: value = (value<<8) | netflow9Flow->fields[offset++];
	case 6: value = (value<<8) | netflow9Flow->fields[offset++];
	case 5: value = (value<<8) | netflow9Flow->fields[offset++];
	case 4: value = (value<<8) | netflow9Flow->fields[offset++];
	case 3: value = (value<<8) | netflow9Flow->fields[offset++];
	case 2: value = (value<<8) | netflow9Flow->fields[offset++];
	case 1: value = (value<<8) | netflow9Flow->fields[offset++]; break;
	default: break;
	}
	return value;
  }

  //.............................

  SPL::rstring netflow9FieldAsString(const uint16_t fieldType) {

	// return zero if there is no such field in this flow
	if ( !netflow9Flow || !templateState || fieldType<1 || fieldType>FLOW_FIELDS_MAXIMUM ) return SPL::rstring();

	// get the length of the field and its offset within the flow record, according to the template
	const uint16_t offset =  templateState->flowFields[fieldType].offset;
	const uint16_t length =  templateState->flowFields[fieldType].length;
	if (!length) return SPL::rstring();

	// address the flow's byte array
	const uint8_t* fields = netflow9Flow->fields;

	// get the value of the field from the flow record as an SPL string and return it
	return SPL::rstring(&fields[offset], &fields[offset+length]);
  }

  //.............................

  SPL::list<SPL::uint8> netflow9FieldAsByteList(const uint16_t fieldType) {

	// return zero if this flow does not contain the specified field
	if ( !netflow9Flow || !templateState || fieldType<1 || fieldType>FLOW_FIELDS_MAXIMUM ) return SPL::list<SPL::uint8>();

	// get the length of the field and its offset within the flow record, according to the template
	const uint16_t offset =  templateState->flowFields[fieldType].offset;
	const uint16_t length =  templateState->flowFields[fieldType].length;
	if (!length) return SPL::list<SPL::uint8>();

	// address the flow's byte array
	const uint8_t* fields = netflow9Flow->fields;

	// get the value of the field from the flow record as an SPL byte list and return it
	return SPL::list<SPL::uint8>(&fields[offset], &fields[offset+length]);
  }

  // ............................

  SPL::uint32 errorOffset() {
	if (!error) return 0;
	if (netflow5Flow) return (uint8_t*)netflow5Flow - messageStart;
	if (netflow9Flow) return (uint8_t*)netflow9Flow - messageStart;
	if (netflow9Flowset) return (uint8_t*)netflow9Flowset - messageStart;
	return 0;
  }

  // This function prepares the parser for a Netflow message. It sets 'netflow9Header' or 
  // 'netflow5Header', depending upon the version of the message. It sets 'error' if a 
  // problem is found.

  void prepareNetflowMessage(char* buffer, int length, uint32_t source) {

	  // clear all of the variables results will be returned in 
	  messageLength = 0;
	  messageStart = NULL;
	  messageEnd = NULL;
	  messageMissedCount = 0;
	  sourceAddress = 0; 
	  netflow9Header = NULL;
	  netflow5Header = NULL;
	  netflow9Flowset = NULL;
	  netflow9Flow = NULL;
	  netflow5Flow = NULL;
	  templateState = NULL;
	  flowCount = 0;
	  error = NULL;

	  // basic safety checks
	  if ( length < sizeof(struct NetflowCommonHeader) ) { error = "no header"; return; }
	  if ( ntohs( ((struct NetflowCommonHeader*)buffer)->version) != 5 && 
		   ntohs( ((struct NetflowCommonHeader*)buffer)->version) != 9 ) { error = "version not 5 or 9"; return; } 
	  if ( ntohs( ((struct NetflowCommonHeader*)buffer)->count) > 32 ) { error = "header count too large"; return; }
	
	  // store pointers to the Netflow message in the buffer
	  messageLength = length;
	  messageStart = (uint8_t*)buffer;
	  messageEnd = (uint8_t*)buffer + length;
	  sourceAddress = source;

	  // call the appropriate preparation function
	  switch (ntohs(((struct NetflowCommonHeader*)messageStart)->version)) {
	      case 5: prepareNetflow5Message(); break;
	      case 9: prepareNetflow9Message(); break;
	      default: error = "unsupported Netflow version"; break;
	  }
  }

  // This function advances the parser to the next flow record in the prepared Netflow message.
  // It there is a next flow record, it returns 'true' after setting 'netflow9Flow' or 'netflow5Flow'.
  // If there are no more flow records in this Netfow message, it returns 'false'. It sets 'error'
  // if a problem is found.

  bool nextFlowRecord() {

	  if (error) return false;

	  switch (ntohs(((struct NetflowCommonHeader*)messageStart)->version)) {
	      case 5:  return nextFlow5Record(); break;
	      case 9:  return nextFlow9Record(); break;
	      default: return false;       break;
	  }
	  return false;
  }


};

#endif /* NETFLOW_MESSAGE_PARSER_H_ */
