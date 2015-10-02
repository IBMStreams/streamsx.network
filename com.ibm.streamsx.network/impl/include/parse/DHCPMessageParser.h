/*
** Copyright (C) 2011, 2015  International Business Machines Corporation
** All Rights Reserved
*/

#ifndef DHCP_MESSAGE_PARSER_H_
#define DHCP_MESSAGE_PARSER_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string>

#include <SPL/Runtime/Type/SPLType.h>


////////////////////////////////////////////////////////////////////////////////
// This class parses DHCP fields within a DHCP message
////////////////////////////////////////////////////////////////////////////////

class DHCPMessageParser {


 private:
	
  // These structures define the format of DHCP resource records, as encoded in
  // DHCP network packets, according to RFC 2131 and 2132 (see
  // https://tools.ietf.org/html/rfc2131 and https://tools.ietf.org/html/rfc2132 )
	
  // structure of DHCP option record within a DHCP message
  struct DHCPOption {
	uint8_t code;		// option code
	uint8_t length;		// option data length
	uint8_t data[0];	// option data
  } __attribute__((packed)) ;
  
  // structure of DHCP header within a DHCP message
  struct DHCPHeader {
    uint8_t  op;			// Message op code / message type.
    uint8_t  htype; 		// Hardware address type
    uint8_t  hlen; 			// Hardware address length
    uint8_t  hops; 			// Client sets to zero, optionally used by relay agents
    uint32_t xid; 			// Transaction ID, a random number chosen by the client
    uint16_t secs; 			// Filled in by client, seconds elapsed since client began 
    uint16_t flags; 		// Flags 
    uint32_t ciaddr; 		// Client IP address
    uint32_t yiaddr; 		// 'your' (client) IP address.
    uint32_t siaddr; 		// IP address of next server to use in bootstrap
    uint32_t giaddr; 		// Relay agent IP address
    uint8_t  chaddr[16]; 	// Client hardware address
    char     sname[64]; 	// Optional server host name, null terminated string
    char     file[128]; 	// Boot file name, null terminated string
    uint8_t  magic[4];		// magic cookie == "DHCP"
    struct DHCPOption options[0]; // Optional parameters field, see DHCPOption
  } __attribute__((packed)) ;
  
 public:

  // addresses of options in DHCP packet, indexed by option code
  static const uint32_t MAX_OPTIONS = 256;
  struct DHCPOption* dhcpOptions[MAX_OPTIONS];
  
  // ambiguous fields are copied here
  char serverName[256];
  char bootfileName[256];


  // These functions convert DHCP options into various SPL types

  SPL::boolean dhcpOptionAsBoolean(const uint8_t index) { 
	if ( !dhcpOptions[index] ) return false;
	if ( dhcpOptions[index]->length!=1 ) { error = "option length not 1"; return false; }
	return dhcpOptions[index]->data[0]!=0; }

  SPL::rstring dhcpOptionAsString(const uint8_t index) { 
	if ( !dhcpOptions[index] ) return SPL::rstring();
	return SPL::rstring((char*)dhcpOptions[index]->data, (char*)dhcpOptions[index]->data + dhcpOptions[index]->length); }

  SPL::uint8 dhcpOptionAsUint8(const uint8_t index) { 
	if ( !dhcpOptions[index] ) return 0;
	if ( dhcpOptions[index]->length!=1 ) { error = "option length not 1"; return 0; }
	return dhcpOptions[index]->data[0]; }

  SPL::uint16 dhcpOptionAsUint16(const uint8_t index) { 
	if ( !dhcpOptions[index] ) return 0;
	if ( dhcpOptions[index]->length != 2 ) { error = "option length not 2"; return 0; }
	uint16_t* value = (uint16_t*)dhcpOptions[index]->data;
	return ntohs(*value); }

  SPL::int32 dhcpOptionAsInt32(const uint8_t index) { 
	if ( !dhcpOptions[index] ) return 0;
	if ( dhcpOptions[index]->length != 4 ) { error = "option length not 4"; return 0; }
	int32_t* p = (int32_t*)dhcpOptions[index]->data;
	return ntohl(*p); }

  SPL::uint32 dhcpOptionAsUint32(const uint8_t index) { 
	if ( !dhcpOptions[index] ) return 0;
	if ( dhcpOptions[index]->length != 4 ) { error = "option length not 4"; return 0; }
	uint32_t* p = (uint32_t*)dhcpOptions[index]->data;
	return ntohl(*p); }

  SPL::list<SPL::uint8> dhcpOptionAsListUint8(const uint8_t index) { 
	if ( !dhcpOptions[index] ) return SPL::list<SPL::uint8>();
	return SPL::list<SPL::uint8>(dhcpOptions[index]->data, dhcpOptions[index]->data + dhcpOptions[index]->length); }

  SPL::list<SPL::uint16> dhcpOptionAsListUint16(uint8_t index) { 
	SPL::list<SPL::uint16> values;
	if ( !dhcpOptions[index] ) return values;
	if ( dhcpOptions[index]->length % 2 != 0 ) { error = "option length not multiple of 2"; return values; }
	for (uint16_t* p = (uint16_t*)dhcpOptions[index]->data; (uint8_t*)p < dhcpOptions[index]->data + dhcpOptions[index]->length; p++) {
	  values.add(ntohs(*p)); }
	return values; }

  SPL::list<SPL::uint32> dhcpOptionAsListUint32(const uint8_t index) { 
	SPL::list<SPL::uint32> values;
	if ( !dhcpOptions[index] ) return values;
	if ( dhcpOptions[index]->length % 4 != 0 ) { error = "option length not multiple of 4"; return values; }
	for (uint32_t* p = (uint32_t*)dhcpOptions[index]->data; (uint8_t*)p < dhcpOptions[index]->data + dhcpOptions[index]->length; p++) {
	  values.add(ntohl(*p)); }
	return values; }



 private:

  // This function steps through a list of DHCP options and stores their addresses
  // in an array, indexed by option number.

  void parseDHCPOptions(DHCPOption* buffer, int length) {

	for (struct DHCPOption* p = buffer; p->code!=255; p = (struct DHCPOption*)((uint8_t*)p + 2 + p->length)) {
	    if ( (uint8_t*)p>=dhcpEnd ) { error = "options overrun";  return; }
	    dhcpOptions[p->code] = p; }
  }


 public:

  // The parseDHCPMessage() function below returns the address of the DHCP message
  // header in these variables.

  struct DHCPHeader* dhcpHeader;
  uint8_t* dhcpStart;
  uint8_t* dhcpEnd;
  
  // The parseDHCPMessage() function below returns an error description in this
  // variable, if an encoding error is found, or NULL, if no errors are found.

  char const* error;

  // This function parses the DHCP message in the specified buffer and stores the
  // resource records of each type in the arrays above. The individual fields in
  // the resource records can be extracted with the functions above. If an
  // encoding error is found, the 'error' variable is set to a description of
  // the error; otherwise it is set to NULL.

  void parseDHCPMessage(char* buffer, int length) {

	// clear return fields
	dhcpHeader = NULL;
	dhcpStart = NULL;
	dhcpEnd = NULL;
	serverName[0] = 0;
	bootfileName[0] = 0;
	error = NULL;

	// basic safety checks
	if ( length < sizeof(struct DHCPHeader) ) { error = "message too short"; return; }
	
	// store pointers to the DHCP message in the buffer
	dhcpHeader = (struct DHCPHeader*)buffer;
	dhcpStart = (uint8_t*)buffer;
	dhcpEnd = (uint8_t*)buffer + length;

	// more safety checks on the DHCP header
	static const uint8_t MAGIC_COOKIE[4] = { 99, 130, 83, 99 };
	if ( memcmp(dhcpHeader->magic, MAGIC_COOKIE, sizeof(MAGIC_COOKIE))!=0 ) { error = "magic cookie missing"; return; }

	// clear DHCP options index table
	memset(dhcpOptions, 0, sizeof(dhcpOptions));

	// locate all of the DHCP options in the message, indexed by option code
	parseDHCPOptions(dhcpHeader->options, dhcpEnd-(uint8_t*)dhcpHeader->options);

	// if 'sname' field contains more options, locate them three, otherwise copy the 'sname' field as the server name
	if ( dhcpOptions[52] && dhcpOptions[52]->length==1 && ( dhcpOptions[52]->data[0] & 0x02 ) ) {
	  parseDHCPOptions( (DHCPOption*)dhcpHeader->sname, sizeof(dhcpHeader->sname) );
	}  else {
	  strncpy(serverName, dhcpHeader->sname, sizeof(dhcpHeader->sname));
	  serverName[sizeof(dhcpHeader->sname)] = 0;
	}

	// if 'file' field contains more options, locate them too, otherwise copy the 'file' field as the bootfile name
	if ( dhcpOptions[52] && dhcpOptions[52]->length==1 && ( dhcpOptions[52]->data[0] & 0x01 ) ) {
	  parseDHCPOptions( (DHCPOption*)dhcpHeader->file, sizeof(dhcpHeader->file) );
	} else {
	  strncpy(bootfileName, dhcpHeader->file, sizeof(dhcpHeader->file));
	  bootfileName[sizeof(dhcpHeader->file)] = 0;
	}

	// if server name is in an option, copy it
	if ( dhcpOptions[66] ) {
	  strncpy(serverName, (char*)dhcpOptions[66]->data, dhcpOptions[66]->length);
	  serverName[dhcpOptions[66]->length] = 0;
	}

	// if bootfile name is in an option, copy it
	if ( dhcpOptions[67] ) {
	  strncpy(bootfileName, (char*)dhcpOptions[67]->data, dhcpOptions[67]->length);
	  bootfileName[dhcpOptions[67]->length] = 0;
	}
  }

};

#endif /* DHCP_MESSAGE_PARSER_H_ */
