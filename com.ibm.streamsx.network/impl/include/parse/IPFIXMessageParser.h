/*
** Copyright (C) 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef IPFIX_MESSAGE_PARSER_H_
#define IPFIX_MESSAGE_PARSER_H_

#include <endian.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <tr1/unordered_map>

#include <SPL/Runtime/Type/SPLType.h>

// suppress " warning: array subscript is above array bounds [-Warray-bounds] " messages
// from GCC version 4.8.3 in RHEL 7.1

// ??? #pragma GCC diagnostic ignored "-Warray-bounds"


////////////////////////////////////////////////////////////////////////////////
// This class locates IPFIX fields within a IPFIX message
////////////////////////////////////////////////////////////////////////////////

class IPFIXMessageParser {


 private:

  struct IPFIXFieldSpecifier {
      uint16_t identifier; // enterprise flag (0x80000) and field type code (0x7FFF)
      uint16_t length; // length of this field as encoded in data records, or 65535 for variable length
      uint32_t enterpriseNumber[0]; // present only if enterprise flag in 'identifier' is set
  } __attribute__((packed)) ;

  struct IPFIXTemplate {
    uint16_t templateID; // templateID values are >255
    uint16_t fieldCount; // the number of type/length pairs in 'fieldSpecifiers' field
    struct IPFIXFieldSpecifier fieldSpecifiers[0];
  } __attribute__((packed)) ;

  struct IPFIXOption {
    uint16_t templateID; // templateID values are >255
    uint16_t fieldCount; // total number of fields in this Options record
    uint16_t scopeFieldCount; // number of scope fields in this Options record
    struct IPFIXFieldSpecifier fieldSpecifiers[0];
  } __attribute__((packed)) ;

  struct IPFIXData {
    uint8_t fields[0]; // flow fields (number and length according to template specified by templateID)
  } __attribute__((packed)) ;

  struct IPFIXSet {
    uint16_t setID; // set identifier: 2 for templates, 3 for options, >255 for data
    uint16_t length; // length of this set, including this header, all records, and any padding
    union {
      struct IPFIXTemplate templates[0];
      struct IPFIXOption options[0];
      struct IPFIXData data[0];
    } u;
    uint8_t padding[0];
  } __attribute__((packed)) ;

  struct IPFIXHeader {
    uint16_t version; // always '10' in IPFIX messages
    uint16_t length; // total length of IPFIX message, including header and all sets
    uint32_t exportTime; // time when packet was sent, according to sender's clock, in seconds since UNIX epoch
    uint32_t dataSequence; // data record sequence number, according to sender
    uint32_t sourceID; // unique identifier for source's observation domain, for correlating templates and data
    struct IPFIXSet sets[0];
  } __attribute__((packed)) ;

  // This table keeps track of the last sequence number in IPFIX messages received from
  // each processor engine in each source.

  struct SourceState {
    uint32_t sourceAddress; // part of table index
    uint32_t sourceID; // part of table index
    uint32_t previousSequenceNumber;
    uint16_t previousFlowCount;
  };
  std::tr1::unordered_map<uint64_t, struct SourceState*> sourceTable; // indexed by sourceAddress+sourceID

  // This table keeps track of the templates received from each source
  // Each template is stored as received
  // (in the 'template' variables) and as an offset/length array, indexed
  // by field number (in the 'flowFields' array), for faster access when flow
  // records are parsed.

  static const uint16_t MAXIMUM_TEMPLATE_LENGTH = 1024; 
  static const uint16_t MAXIMUM_IDENTIFIER_VALUE = 1024;
  struct TemplateState {
    uint32_t sourceAddress; // part of table index
    uint32_t sourceID; // part of table index
    uint16_t templateID; // part of table index
    uint16_t fieldCount; // number of fields in this template
    uint8_t templat[MAXIMUM_TEMPLATE_LENGTH]; // template, as received from source
    uint16_t templateLength; // length of this template, as received from source
    uint16_t dataLength; // length of 'data flow' records that use this template
    uint16_t identifierMaximum; // largest value of 'identifier' used in this template
    struct { 
      uint16_t standardOffset; // offset to standardized field in data record, or zero if absent
      uint16_t standardLength; // length of standardized field in data record, or zero if absent
      uint16_t enterpriseOffset; // offset to enterprise field in data record, or zero if absent
      uint16_t enterpriseLength; // length of enterprise field in data record, or zero if absent
      uint32_t enterpriseIdentifier; // identifier of enterprise field in data record, or zero if absent
    } dataFields[MAXIMUM_IDENTIFIER_VALUE+1]; // indexed by 'identifier' of 'template'
  };
  std::tr1::unordered_map<uint64_t, struct TemplateState*> templateTable; // indexed by sourceAddress+sourceID+templateID

  // The prepareIPFIXMessage() functions below stores the IP address of the
  // source that sent the IPFIX message in this variable.

  uint32_t sourceAddress;

  // The prepareIPFIXMessage() function below keeps track of the IPFIX
  // message being parsed in these variables.

  int messageLength;
  uint8_t* messageStart;
  uint8_t* messageEnd;
  uint64_t messageMissedCount;

  // The nextDataRecord() function keeps track of the template for the current
  // 'flow data' record in this variable.

  struct TemplateState *templateState;

  // The nextDataRecord() function keeps track of the number of fields in a
  // 'flow data' record, plus their combined length, in these variables.

  int ipfixDataCount;
  int ipfixDataLength;


 public:

  // The prepareIPFIXMessage() and nextDataRecord() functions below return the
  // address of the current IPFIX message header, set, and data message in these
  // variables.

  struct IPFIXHeader* ipfixHeader;
  struct IPFIXSet* ipfixSet;
  struct IPFIXData* ipfixData;

  // The prepareIPFIXMessage() and nextDataRecord() functions below set this
  // variable when they find an encoding error in the IPFIX message, or set it
  // to NULL if no problems are found.

  char const* error;

  // The nextDataRecord() function below sets this flag when there are no more
  // data records in the current IPFIX mesage.

  bool done;


 private:

  // This function stores all of the templates from a IPFIX template set
  // in a state table for use in decoding subsequent data records. If an
  // encoding error is found, the 'error' variable is set.

  void storeTemplates() {

    // store each template in this set in a separate state table
    for ( struct IPFIXTemplate* ipfixTemplate = &ipfixSet->u.templates[0];
          (uint8_t*)ipfixTemplate < (uint8_t*)ipfixSet + ntohs(ipfixSet->length); ) {

      // find the state table for this template, if there is one, or create a new one if not
      const uint16_t templateID = ntohs(ipfixTemplate->templateID);
      if (templateID<256) { error = "IPFIX templateID too small"; return; }
      const uint32_t sourceID = ntohl(ipfixHeader->sourceID);
      const uint64_t index = (((uint64_t)sourceAddress)<<32) + ((uint64_t)sourceID<<16) + ((uint64_t)templateID);
      struct TemplateState *templateState = templateTable[index];
      if (!templateState) {
        templateState = new TemplateState;
        templateState->sourceAddress = sourceAddress;
        templateState->sourceID = sourceID;
        templateState->templateID = templateID;
        templateState->fieldCount = 0;
        memset( templateState->templat, 0, sizeof(templateState->templat) );
        templateState->templateLength = 0;
        templateState->dataLength = 0;
        templateState->identifierMaximum = 0;
        memset( templateState->dataFields, 0, sizeof(templateState->dataFields) );
        templateTable[index] = templateState;
      }

      // if this same template has been received before, there is no need to
      // re-create the dataFields array
      if (templateState->templateLength && memcmp((uint8_t*)ipfixTemplate, templateState->templat, templateState->templateLength) == 0 ) {
        ipfixTemplate = (struct IPFIXTemplate*)((uint8_t*)ipfixTemplate + templateState->templateLength);
        continue; }

      // get the number of fields in this template
      templateState->fieldCount = ntohs(ipfixTemplate->fieldCount);
      if ( templateState->fieldCount < 1 ) { error = "IPFIX field count zero"; return; }

      // clear the portion of the field array used by the previous template
      memset( templateState->dataFields, 0, ( templateState->identifierMaximum + 1 ) * sizeof(templateState->dataFields[0]) );
      templateState->identifierMaximum = 0;
      templateState->dataLength = 0;
      templateState->templateLength = sizeof(struct IPFIXTemplate);

      // store the offset, length and perhaps enterprise identifier of each
      // field from this template in the state table
      struct IPFIXFieldSpecifier* ipfixField = &ipfixTemplate->fieldSpecifiers[0];
      for (int count=0; count < templateState->fieldCount; count++) { 

        // get the type and length of this field
        bool enterprise = ntohs(ipfixField->identifier) & 0x8000;
        uint16_t identifier = ntohs(ipfixField->identifier) & 0x7FFF;
        uint16_t length = ntohs(ipfixField->length);
        uint32_t enterpriseIdentifier = enterprise ? ntohl(ipfixField->enterpriseNumber[0]) : 0;
        if (identifier>MAXIMUM_IDENTIFIER_VALUE) { error = "IPFIX template field identifier too large"; return; }
        if (!length) { error = "IPFIX template field length zero"; return; }
        if (length==65535) { error = "IPFIX template field length variable"; return; }

        // store the offset and length this field will have in 'data flow' records
        templateState->dataFields[identifier].standardOffset = !enterprise ? templateState->dataLength : 0;
        templateState->dataFields[identifier].standardLength = !enterprise ? length : 0;
        templateState->dataFields[identifier].enterpriseOffset = enterprise ? templateState->dataLength : 0;
        templateState->dataFields[identifier].enterpriseLength = enterprise ? length : 0;
        templateState->dataFields[identifier].enterpriseIdentifier = enterpriseIdentifier;

        // keep track of how much of the field array this template uses
        if ( templateState->identifierMaximum < identifier ) templateState->identifierMaximum = identifier;

        // keep track of the length this data will have in 'flow data' records
        templateState->dataLength += length;
        templateState->templateLength += sizeof(struct IPFIXFieldSpecifier) + ( enterprise ? 4 : 0 );
        ipfixField = (IPFIXFieldSpecifier*)( (uint8_t*)ipfixField + sizeof(struct IPFIXFieldSpecifier) + ( enterprise ? 4 : 0 ) );
      }

      // store the template itself in the state table
      if ( templateState->templateLength > sizeof(templateState->templat) ) { error = "IPFIX template too long"; return; }
      memcpy(templateState->templat, (uint8_t*)ipfixTemplate, templateState->templateLength);

      // step over this template to the next one in this set, if any
      ipfixTemplate = (struct IPFIXTemplate*)( (uint8_t*)ipfixTemplate + templateState->templateLength );
    }
  }


  // This function should store something when an Options set is found in a
  // IPFIX message, but it doesn't yet.

  void storeOptions() {
  }


 public:

  // This function returns the value of the specified 'standard' field in the
  // current flow, after converting it to an SPL integer.

  inline __attribute__((always_inline))
  SPL::uint64 ipfixStandardFieldAsInteger(const uint16_t identifier) {

    // return zero if there is no such field in this flow
    if ( !ipfixData || !templateState || identifier<1 || identifier>MAXIMUM_IDENTIFIER_VALUE ) return 0;

    // get the length of the field and its offset within the flow record, according to the template
    const uint16_t offset =  templateState->dataFields[identifier].standardOffset;
    const uint16_t length =  templateState->dataFields[identifier].standardLength;
    if (length==0 || length>8) return 0;

    // get the value of the field from the flow record as an integer and return it
    const uint64_t* __attribute__((__may_alias__)) p = reinterpret_cast<uint64_t*>(ipfixData->fields+offset);
    return be64toh( *p ) >> (64-8*length) ; 
  }


  // This function returns the value of the specified 'standard' field in the
  // current flow, after converting it to an SPL string.

  inline __attribute__((always_inline))
  SPL::rstring ipfixStandardFieldAsString(const uint16_t identifier) {

    // return zero if there is no such field in this flow
    if ( !ipfixData || !templateState || identifier<1 || identifier>MAXIMUM_IDENTIFIER_VALUE ) return SPL::rstring();

    // get the length of the field and its offset within the flow record, according to the template
    const uint16_t offset =  templateState->dataFields[identifier].standardOffset;
    const uint16_t length =  templateState->dataFields[identifier].standardLength;
    if (!length) return SPL::rstring();

    // address the flow's byte array, and get the address and length of the field
    const uint8_t* fields = ipfixData->fields;
    const char* stringAddress = (char*)(&fields[offset]);
    const size_t stringLength = strnlen(stringAddress, length);

    // return the value of the field as a string
    return SPL::rstring(stringAddress, stringAddress+stringLength);
  }


  // This function returns the value of the specified 'standard' field in the
  // current flow, after converting it to an SPL byte list.

  inline __attribute__((always_inline))
  SPL::list<SPL::uint8> ipfixStandardFieldAsByteList(const uint16_t identifier) {

    // return zero if this flow does not contain the specified field
    if ( !ipfixData || !templateState || identifier<1 || identifier>MAXIMUM_IDENTIFIER_VALUE ) return SPL::list<SPL::uint8>();

    // get the length of the field and its offset within this 'data flow' record, according to the template
    const uint16_t offset =  templateState->dataFields[identifier].standardOffset;
    const uint16_t length =  templateState->dataFields[identifier].standardLength;
    if (!length) return SPL::list<SPL::uint8>();

    // address the flow's byte array
    const uint8_t* fields = ipfixData->fields;

    // get the value of the field from the flow record as an SPL byte list and return it
    return SPL::list<SPL::uint8>(&fields[offset], &fields[offset+length]);
  }


  // This function returns the value of the specified 'enterprise' field in the
  // current flow, after converting it to an SPL integer.

  inline __attribute__((always_inline))
  SPL::uint64 ipfixEnterpriseFieldAsInteger(const uint16_t identifier) {

    // return zero if there is no such field in this flow
    if ( !ipfixData || !templateState || identifier<1 || identifier>MAXIMUM_IDENTIFIER_VALUE ) return 0;

    // get the length of the field and its offset within the flow record, according to the template
    const uint16_t offset =  templateState->dataFields[identifier].enterpriseOffset;
    const uint16_t length =  templateState->dataFields[identifier].enterpriseLength;
    if (length==0 || length>8) return 0;

    // get the value of the field from the flow record as an integer and return it
    const uint64_t* __attribute__((__may_alias__)) p = reinterpret_cast<uint64_t*>(ipfixData->fields+offset);
    return be64toh( *p ) >> (64-8*length) ; 
  }


  // This function returns the value of the specified 'enterprise' field in the
  // current flow, after converting it to an SPL string.

  inline __attribute__((always_inline))
  SPL::rstring ipfixEnterpriseFieldAsString(const uint16_t identifier) {

    // return zero if there is no such field in this flow
    if ( !ipfixData || !templateState || identifier<1 || identifier>MAXIMUM_IDENTIFIER_VALUE ) return SPL::rstring();

    // get the length of the field and its offset within the flow record, according to the template
    const uint16_t offset =  templateState->dataFields[identifier].enterpriseOffset;
    const uint16_t length =  templateState->dataFields[identifier].enterpriseLength;
    if (!length) return SPL::rstring();

    // address the flow's byte array, and get the address and length of the field
    const uint8_t* fields = ipfixData->fields;
    const char* stringAddress = (char*)(&fields[offset]);
    const size_t stringLength = strnlen(stringAddress, length);

    // return the value of the field as a string
    return SPL::rstring(stringAddress, stringAddress+stringLength);
  }


  // This function returns the value of the specified 'enterprise' field in the
  // current flow, after converting it to an SPL byte list.

  inline __attribute__((always_inline))
  SPL::list<SPL::uint8> ipfixEnterpriseFieldAsByteList(const uint16_t identifier) {

    // return zero if this flow does not contain the specified field
    if ( !ipfixData || !templateState || identifier<1 || identifier>MAXIMUM_IDENTIFIER_VALUE ) return SPL::list<SPL::uint8>();

    // get the length of the field and its offset within this 'data flow' record, according to the template
    const uint16_t offset =  templateState->dataFields[identifier].enterpriseOffset;
    const uint16_t length =  templateState->dataFields[identifier].enterpriseLength;
    if (!length) return SPL::list<SPL::uint8>();

    // address the flow's byte array
    const uint8_t* fields = ipfixData->fields;

    // get the value of the field from the flow record as an SPL byte list and return it
    return SPL::list<SPL::uint8>(&fields[offset], &fields[offset+length]);
  }


  // This function returns the value of the 'enterprise identifier' corresponding to the 
  // specified 'field identifier', if there is one

  inline __attribute__((always_inline))
  SPL::uint32 ipfixEnterpriseIdentifier(const uint16_t identifier) {

    // return zero if there is no such field in this flow
    if ( !ipfixData || !templateState || identifier<1 || identifier>MAXIMUM_IDENTIFIER_VALUE ) return 0;

    // return the enterprise identifier for this field identifier, if there is one
    return templateState->dataFields[identifier].enterpriseIdentifier;
  }


  // If an encoding error has been found, this function returns the offset of
  // the misencoded field, relative to the beginning of the IPFIX message

  SPL::uint32 errorOffset() {

    if (!error) return 0;
    if (ipfixData) return (uint8_t*)ipfixData - messageStart;
    if (ipfixSet) return (uint8_t*)ipfixSet - messageStart;

    return 0;
  }


  // This function prepares the parser for a IPFIX message. It sets
  // 'ipfixHeader', or sets 'error' if a problem is found.

  void prepareIPFIXMessage(char* buffer, int length, uint32_t source) {

      // clear all of the variables results will be returned in
      messageLength = 0;
      messageStart = NULL;
      messageEnd = NULL;
      messageMissedCount = 0;
      sourceAddress = 0;
      ipfixHeader = NULL;
      ipfixSet = NULL;
      ipfixData = NULL;
      templateState = NULL;
      ipfixDataCount = 0;
      ipfixDataLength = 0;
      error = NULL;
      done = false;

      // basic safety checks
      if ( length < sizeof(struct IPFIXHeader) ) { error = "no header"; return; }
      if ( ntohs( ((struct IPFIXHeader*)buffer)->version) != 10 ) { error = "version not 10"; return; }
      if ( ntohs( ((struct IPFIXHeader*)buffer)->length) < length ) { error = "message too small"; return; }
      if ( ntohs( ((struct IPFIXHeader*)buffer)->length) > length   ) { error = "message too large"; return; }

      // store pointers to the IPFIX message in the buffer
      messageLength = length;
      messageStart = (uint8_t*)buffer;
      messageEnd = (uint8_t*)buffer + length;
      sourceAddress = source;

    // point at the IPFIX header structure in the message
    if ( messageLength < sizeof(struct IPFIXHeader) ) { error = "header too short"; return; }
    ipfixHeader = (struct IPFIXHeader*)messageStart;

    // find the state structure for this message's source, or create a new one
    const uint32_t sourceID = ntohl(ipfixHeader->sourceID);
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
    const uint32_t thisSequenceNumber = ntohl(ipfixHeader->dataSequence);
    const uint32_t previousSequenceNumber = sourceState->previousSequenceNumber;
    if ( thisSequenceNumber && previousSequenceNumber && thisSequenceNumber!=previousSequenceNumber+1 ) {
        messageMissedCount = thisSequenceNumber - previousSequenceNumber - 1;
    }
    sourceState->previousSequenceNumber = thisSequenceNumber; 
  }


  // This function advances the parser to the next IPFIXData record in the
  // prepared IPFIX message.  If there is a next data record, it sets
  // 'ipfixData' to point at it, or if there are no more data records in this
  // message, it sets 'done'.  If an encoding error is found, it sets 'error'.

  void nextDataRecord() {

      // don't do anything more with a IPFIX message that's mis-encoded
      if (error) return;

    // if we are currently parsing a 'flow data' record, and there are more
    // flows in it, return the next one
    if (ipfixData && ipfixDataCount>1) {
        if ( (uint8_t*)ipfixData + ipfixDataLength > messageEnd ) { error = "IPFIX data record truncated"; return; }
        ipfixData = (IPFIXData*)((uint8_t*)ipfixData + ipfixDataLength);
        ipfixDataCount--;
        return;
    }

    // if we will be starting to parse the next 'flow data' record, reset
    // data-related variables 
    templateState = NULL;
    ipfixData = NULL;
    ipfixDataCount = 0;
    ipfixDataLength = 0;

    // if we have finished parsing the current set of records, advance to the next set,
    // or, if we have not started parsing this IPFIX message yet, point at the first set in it
    if (ipfixSet) {
        ipfixSet = (IPFIXSet*)((uint8_t*)ipfixSet + ntohs(ipfixSet->length));
    } else {
        ipfixSet = &ipfixHeader->sets[0];
    }

    // find the next set of 'data flow' records in the message and return the
    // first record in it. If a set of templates or options is encountered,
    // store them in state tables before proceeding to process the 'data flow'
    // records'
    for (; (uint8_t*)ipfixSet<messageEnd; ipfixSet = (IPFIXSet*)((uint8_t*)ipfixSet + ntohs(ipfixSet->length))) {

      // check for truncated set 
      uint16_t setID = ntohs(ipfixSet->setID);
      uint16_t setLength = ntohs(ipfixSet->length);
      if ( setLength < sizeof(IPFIXSet) ) { error = "IPFIX set too small"; return; }
      if ( (uint8_t*)ipfixSet + setLength > messageEnd ) { error = "IPFIX set truncated"; return; }

      // if this set contains 'flow data' records, and we have stored its template, return the first flow in it
      if (setID>255) {

          // find the template for this set; if we have not stored its template, skip this flow
          const uint32_t sourceID = ntohl(ipfixHeader->sourceID);
          const uint64_t index = (((uint64_t)sourceAddress)<<32) + ((uint64_t)sourceID<<16) + ((uint64_t)setID);
          templateState = templateTable[index];
          if (!templateState) { continue; }

          // check for empty set
          ipfixDataLength = templateState->dataLength;
          if (!ipfixDataLength) { error = "IPFIX data length zero"; return; }
          ipfixDataCount = setLength / ipfixDataLength;
          if (!ipfixDataCount) { error = "IPFIX data set empty"; return; }

          // return the first 'flow data' record in this set
          ipfixData = &ipfixSet->u.data[0];
          return;
      }

      // if this set of records contains templates or options, store them and then try again with the next set
      switch(setID) {
      case 2:  storeTemplates(); break;
      case 3:  storeOptions(); break;
      default: error = "unsupported set type"; break;
      }
      if (error) return;
    }

    // when we have processed all of the sets in this message, set the 'done' flag
    done = true;
  }

};

#endif /* IPFIX_MESSAGE_PARSER_H_ */
