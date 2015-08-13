Copyright (C) 2011, 2015  International Business Machines Corporation
All Rights Reserved



Network Toolkit: MAC Address functions
--------------------------------------


_____Description_____


The InfoSphere Streams 'network' toolkit includes functions that transform
ethernet MAC addresses between string and six-byte binary presentations.  The
toolkit also includes sample applications that illustrate how to use these
functions.

The toolkit includes these MAC address functions:

    rstring convertMACAddressNumericToString(list<uint8>[6] macAddressNumeric)

    list<uint8>[6] convertMACAddressStringToNumeric(rstring macAddressString)

The functions use these representations of ethernet MAC addresses:

  * The six-byte binary representation of a MAC address is an SPL list
    of six unsigned eight-bit integers (that is, a 'list<unit8>[6]' type), with
    bytes in network order.  For example, [ 152, 90, 235, 203, 55, 219 ].

  * The string representation of a MAC address is passed as an SPL string (that
    is, an 'rstring' type) of hexadecimal characters separated by colons. For
    example, '98:5a:eb:cb:37:db'.

Notes:

  * If a string argument passed to convertMACAddressStringToNumeric() does not
    contain a valid ethernet MAC address, a list of six zeroes is returned.

For more information about ethernet MAC addresses, see:

    https://en.wikipedia.org/wiki/MAC_address

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
