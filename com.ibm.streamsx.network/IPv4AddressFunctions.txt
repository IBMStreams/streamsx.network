Copyright (C) 2011, 2015  International Business Machines Corporation
All Rights Reserved



Network Toolkit: IPv4 Address functions
---------------------------------------


_____Description_____


The 'network nacket' toolkit includes functions that transform IP version 4
addresses between string and four-byte binary representations. The toolkit also
includes sample applications that illustrate how to use these functions.

The toolkit includes these IP version 4 address functions in the
'com.ibm.streamsx.network.ipv4' namespace:

    rstring convertIPV4AddressNumericToString(uint32 ip4AddressNumeric)

    uint32  convertIPV4AddressStringToNumeric(rstring ip4AddressString)

    rstring convertIPV4AddressStringToSubnet(rstring ipv4Address, int32 maskbits)

    rstring convertIPV4AddressNumericToSubnet(uint32 ipv4Address, int32 maskbits)

    rstring convertIPV4AddressStringToHostname(rstring ipv4Address)

    rstring convertIPV4AddressNumericToHostname(uint32 ipv4AddressNumeric)

    rstring convertHostnameToIPV4AddressString(rstring hostname)

    uint32  convertHostnameToIPV4AddressNumeric(rstring hostname)

The functions use these representations of IP version 4 addresses:

  * The four-byte binary representation of an IP version 4 address is an
    unsigned 32-bit integer (that is, a 'uint32' type), with bytes in network
    order.  For example, '1247382016'.

  * The string representation of an IP version 4 address is an SPL string (that
    is, an 'rstring' type) of decimal characters separated by periods. For
    example, '74.89.138.153'.

  * The string representation of an IP version 4 subnet uses CIDR notation, that
    is, an SPL string of decimal characters separated by periods, followed by a
    slash and an integer indicting the number of mask bits. For example,
    '74.89.138.0/24'.

Notes:

  * If a string argument passed to one of these functions does not
    contain a valid IP version 4 address, a value of zero is returned.

  * The convert*Hostname*() functions convert between string representations of
    IP verion 4 addresses and fully-qualified host domain names.  The system's
    usual name resolution mechanisms are used, which may include lookups in
    local files (such as the system file '/etc/hosts') and external servers
    (that is, domain name severs configured with the system file
    '/etc/resolv.conf').  If name resolution is unsuccessful in either
    direction, an error message is written into the Streams log file, and the
    original string is returned.

  * The system may take a long time to resolve hostnames, up to many seconds, if
    the system must consult remote name servers.  To minimze this delay, these
    functions cache successful resolutions in memory, so that subsequent
    conversions are quick.  However, use of these functions should be confined
    to branches of a Streams flow graph that can tolerate long delays.

For more information about IP version 4 addresses, see:

    https://en.wikipedia.org/wiki/IP_address

    https://en.wikipedia.org/wiki/Classless_Inter-Domain_Routing#CIDR_notation

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
