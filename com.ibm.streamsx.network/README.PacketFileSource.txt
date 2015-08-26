Copyright (C) 2011, 2015  International Business Machines Corporation
All Rights Reserved



Network Toolkit: PacketFileSource operator
------------------------------------------


_____Description_____


PacketFileSource is an operator for the IBM InfoSphere Streams product that
reads pre-recorded ethernet packets from 'packet capture (PCAP)' files and emits
them as output attributes.  Each tuple produced by the operator contains one
packet, including all network headers, in an attribute of type 'blob (binary
large object)'.  The tuple may also contain the time the packet was captured.

The PacketFileSource operator expects PCAP files to contain complete ethernet
packets, starting with the ethernet header, including all protocol-specific
headers and the packet payload.  The ethernet frame format is described here:

    http://en.wikipedia.org/wiki/Ethernet_frame
    http://linux.die.net/include/net/ethernet.h
    http://linux.die.net/include/linux/if_ether.h

Files containing complete ethernet packets can be created in PCAP format by a
variety of network diagnostic tools, such as the Linux 'tcpdump' command and the
'Wireshark' open-source tools.  For details, see for example:

    http://linux.die.net/man/8/tcpdump
    http://www.wireshark.org/

Each input tuple consumed by the PacketFileSource operator produces many output
tuples, since each PCAP input file typically contains many packets.  However,
the operator may optionally be configured to filter packets, that is, to produce
as tuples only packets that meet a particular specification.

This operator is part of the network toolkit. The toolkit also includes sample
applications that illustrate how to use the operator.


_____Dependencies_____


The PacketFileSource operator depends upon the Linux 'packet capture library
(libpcap)'.  The library must be installed on the machine where this operator
executes. It is available as an installable 'repository package (RPM)' from the
'base' RHEL and CentOS repositories.  It can be installed with administrator
tools such as 'yum'.  This requires root privileges, which can be acquired
temporarily with administrator tools such as 'sudo'.

To install 'libpcap', enter this command at a Linux command prompt:

    sudo yum install libpcap-devel

Alternatively, you can download the source code for a newer version of 'libpcap'
and build the library yourself. The new library can then be installed in system
directories, or used where built without being installed.

To do this, download the distribution package for the latest version of
'libpcap' from this address:

    http://www.tcpdump.org/

To build libpcap from source code, open a 'terminal' window and type this at a
command prompt:

    cd .../directory
    tar -xvf .../libpcap-X.Y.Z.tar.gz 
    cd .../directory/libpcap-X.Y.Z
    ./configure
    make

To instruct the Streams compiler (that is, the 'sc' command) to use your version
of 'libpcap' instead of the system version, set these environment variables
before compiling an application that contains the PacketLiveSource operator:

    export STREAMS_ADAPTERS_LIBPCAP_INCLUDEPATH=.../directory/libpcap-X.Y.Z
    export STREAMS_ADAPTERS_LIBPCAP_LIBPATH=.../directory/libpcap-X.Y.Z

See the 'TestPacketFileSource.standalone.sh' script in the 'scripts' directory
for an example of compiling an application with alternative versions of libpcap.

For more information on configuring, building, and installing 'libpcap', refer
to its 'INSTALL.txt' file.

This operator has been tested with these version of 'libpcap':

    libpcap 0.9.4, included in RHEL/CentOS 5
    libpcap 1.0.0, included in RHEL/CentOS 6.2
    libpcap 1.4.0, included in RHEL/CentOS 6.5
    libpcap 1.6.1
    libpcap 1.6.2

This operator is implemented in C/C++, and calls the library 'libpcap' via the
functions defined in the header file <pcap.h>.  These functions are documented
in the manual page 'pcap', for example here:

    http://linux.die.net/man/3/pcap


_____Input Ports_____


The PacketFileSource operator has one optional input port:  

    When the PacketFileSource operator is configured without an input port, a
    parameter specifies the pathname of a single input PCAP file for the
    operator to read.

    When the PacketFileSource operator is configured with an input port, the
    first attribute must be of type 'rstring', and specifies the pathname of an
    input PCAP file for the operator to read.


_____Output Ports_____


The PacketFileSource operator has one required output port and one optional
port: 

  * Output port 0 produces tuples that contain network packets received by the
    specified network adpater (see the 'networkInterface' parameter below),
    subject to filtering (see the 'filterExpression' parameter below).  The
    output attributes for port 0 must be assigned by output functions (see the
    'Assignments' section below).

  * Output port 1, if defined, produces tuples periodically (see the
    'statisticsInterval' parameter below) that contain statistics for the
    specified network adapter.  The output attributes for port 0 must be
    assigned by output functions (see the 'Assignments' section below).

The PacketFileSource operator does not produce punctuation.


_____Parameters_____


   pcapFilename -- This parameter takes an expression of type 'rstring'
      that specifies the pathname of a single input PCAP file for the operator
      to read. If the operator is configured without an inport port, this
      parameter is required; if the operator has an input port, this parameter
      is not allowed.

   filterExpression -- This optional parameter takes an expression of type
      'rstring' that specifies which captured packets should be produced as
      tuples. The value of this string must be a valid PCAP filter expression,
      as documented in the manual page 'pcap-filter', for example here:

          http://linux.die.net/man/7/pcap-filter

      The default value of the 'filterExpression' parameter is an empty string,
      which causes all packets read from the PCAP file to be produced as tuples.

   initDelay -- This optional parameter takes an expression of type 'float64'
      that specifies the number of seconds the operator will wait before it
      begins to produce tuples.  This parameter is allowed only when the
      'pcapFilename' parameter is also specified.  

      The default value of the 'initDelay' parameter is '0.0'.

   processorAffinity -- This optional parameter takes an expression of type
      'unit32' that specifies which processor core the operator's thread will
      run on.  The maximum value is 'P-1', where P is the number of processors
      on the machine where the operator will run.  If this parameter is
      specified, then the operator's thread will be dispatched only on the
      specified processor.  This parameter is allowed only if the 'pcapFilename'
      parameter is also specified.  If instead the operator is configured with
      an input port, then the operator runs on the thread of the upstream
      operator, and is dispatched according to that thread's processor affinity.

      The default is to dispatch the operator's thread on any available processor.

   statisticsInterval -- This optional parameter is valid only if a second
      output port is defined for the operator.  It takes an expression of type
      'float64' that specifies the interval, in seconds, for producing
      statistics tuples on that output port.

      The default value of the 'statisticsInterval' parameter is '1.0'.


_____Windowing_____


The PacketFileSource operator does not accept any window congfiguration.


_____Assignments____


The PacketFileSource operator requires that all attributes of the first output
port be assigned values with one of these output functions:

   float64 captureTime() -- This function assigns the time that a packet was
      originally captured (not the time it was read from the PCAP file) to an
      output attribute.  The value is represented as seconds since the beginning
      of the Unix epoch (midnight in the GMT/UTC timezone on January 1st, 1970),
      according to the system clock, and has a resolution of microseconds.

   uint32 sessionKey() -- This function assigns a 32-bit key, based on the IP
      version 4 source and destination addresses, and for UDP and TCP packets,
      the source and destination port numbers.  The addresses and ports are
      combined such that packets flowing in either direction between endpoints
      have the same session key.  The key can be used to distribute packets
      across multiple parallel processing channels such that all packets that
      are part of a UDP or TCP session are routed to the same instance of the
      processor.

   uint32 packetLength() -- This function assigns the length of a packet to an
      output attribute.  The value includes all network headers and the entire
      packet body.  Note that this value may be larger than the length of the
      binary data assigned to a 'blob' attribute by the 'packetData()' function
      if the packet was truncated when the PCAP file was created.

   uint64 packetNumber() -- This function assigns the sequence number of this
      packet tuple, starting at zero, incrementing by one for each packet tuple
      produced by the operator.

   blob packetData() -- This function assigns the packet data to an output
      attribute.  The value includes all network headers and the entire packet
      body.  Note that the data may be truncated when the PCAP file was created.

The PacketFileSource operator requires that all attributes of the second output
port, if defined, be assigned values with one of these output functions:

   float64 statisticsTime() -- This function assigns the time that statistics
      were produced by the PacketFileSource operator.  The value is represented
      as seconds since the beginning of the Unix epoch (midnight in the GMT/UTC
      timezone on January 1st, 1970), according to the system clock, and has a
      resolution of microseconds.

   uint64 packetsConsumed() -- This function assigns the number of packets read
      from the input file during the current interval.

   uint64 packetsProduced() -- This function assigns the number of tuples
      produced by the first output port during the current interval.

   uint64 octetsProduced() -- This function assigns the number of bytes,
      including network headers, in the tuples produced by the first output port
      during the current interval.



_____Threads_____


When the PacketFileSource operator is configured without an input port, it
contains a thread of execution which runs asynchronously from other
threads in the PE containing the operator.  The operator reads the entire input
PCAP file specified by the 'pcapFilename' operator.  When the operator reaches
end-of-file, it terminates the thread, which terminates the operator.

When the operator has an input port, and is fused in a PE with upstream
operators, it runs on the thread of the upstream operator that sends an input
tuple containing an input PCAP filename.  The PacketFileSource operator reads
the entire input PCAP file specified by each tuple before the thread returns
to the upstream operator.

When the PacketFileSource operator has a second output port, it contains
another thread that produceds statistics tuples on that port.


_____Metrics_____


The PacketFileSource operator has no Streams metrics.

The operator counts the total number of packets and bytes read during the
lifetime of the PE containing it.  When the operator terminates, it logs these
counts, along with its average throughput.  The lifetime of the PE includes the
processing of other operators that may be fused with the PacketFileSource
operator, but excludes all initialization and termination.


_____Exceptions_____


The PacketFileSource operator will throw an exception and terminate in these
situations:

   -- An input PCAP file specified with the 'pcapFilename' parameter, or by an
      input tuple, does not exist, or cannot be read.

   -- The parameter 'filterExpression' does not specify a valid PCAP filter
      expression.


_____Examples_____


The SampleNetworkToolkitData project in this toolkit contains examples of this
operator.


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
