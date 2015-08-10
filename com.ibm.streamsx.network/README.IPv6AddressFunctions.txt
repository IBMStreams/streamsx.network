Copyright (C) 2015, International Business Machines Corporation
All Rights Reserved



Network Toolkit IPv6 Address functions
--------------------------------------


_____Description_____


The InfoSphere Streams 'network' toolkit includes functions that transform IP
version 6 addresses between string and sixteen-byte binary presentations.  The
toolkit also includes sample applications that illustrate how to use these
functions.

The toolkit includes these IP version 6 address functions in the namespace
'com.ibm.streamsx.network.ipv6':

    rstring convertIPV6AddressNumericToString(list<uint8>[16] ip6AddressNumeric)

    list<uint8>[16] convertIPV6AddressStringToNumeric(rstring ip6AddressString)

The functions use these representations of IP version 6 addresses:

  * The sixteen-byte binary representation of an IP version 6 address is a list
    of sixteen unsigned 8-bit integers (that is, a 'list<uint8>[16]' type), with
    bytes in network order.  For example, '[ ????????????  ]'.

  * The string representation of an IP version 6 address an SPL string (that is,
    an 'rstring' type) of hexadecimal characters separated by colons. For
    example, 'fdda:5cc1:23:4::1f'.

Notes:

  * If a string argument passed to convertIPV6AddressStringToNumeric() does not
    contain a valid IP version 6 address, a list of sixteen zeroes is returned.

For more information about IP version 6 addresses, see:

    https://en.wikipedia.org/wiki/IPv6_address

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
