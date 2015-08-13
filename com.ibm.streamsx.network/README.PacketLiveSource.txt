Copyright (C) 2011, 2015  International Business Machines Corporation
All Rights Reserved



Network Toolkit: PacketLiveSource operator
------------------------------------------


_____Description_____


PacketLiveSource is an operator for the IBM InfoSphere Streams product that
captures ethernet packets from a network interface and emits them as output
attributes.  Each tuple produced by the operator contains one packet, including
all network headers, in an attribute of type 'blob (binary large object)'.  The
tuple may also contain the time the packet was captured.

The PacketLiveSource operator must be configured to capture packets from one of
the network interfaces attached to the Linux machine where it executes.  It may
optionally be configured to filter captured packets, that is, to produce as
tuples only packets that meet a particular specification.

The PacketLiveSource operator captures complete ethernet packets, starting with
the ethernet header, including all protocol-specific headers and the packet
payload (subject to truncation by the 'maximumLength' parameter).  The ethernet
frame format is described here:

    http://en.wikipedia.org/wiki/Ethernet_frame
    http://linux.die.net/include/net/ethernet.h
    http://linux.die.net/include/linux/if_ether.h

Network interfaces attached to a shared ethernet segment normally ignore packets
that are not addressed to them.  However, when 'promiscious' mode is enabled on
a network interface, it can capture all network packets on its ethernet segment,
even those that are not addressed to it.  This is sometimes referred to as
"network sniffing".  Modern ethernet switches normally send network interfaces
only packets that are addressed to them; 'promiscious' mode is useful only when
a switch has been specifically configured to send packets that are not addressed
to a network interface.

The PacketLiveSource operator may enable 'promiscuous' mode in a ethernet
interface if its 'promiscous' parameter is set to 'true', or the value of its
'filterExpression' parameter requires it.  If so, the operator will require
special privileges to execute.  In this case, the Streams processing element
(PE) containing the operator must be compiled with the '--static-link' option,
and must run with special privileges.

In RHEL/CentOS 6, the special privileges needed by PacketLiveSource operators
can be granted to PEs that contain them after the application is compiled, but
before it runs, with this command:

    setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' pe-executable-file

In RHEL/CentOS 5, PEs containing PacketLiveSource operators must be executed
with 'root' privileges.  To run a 'standalone' SPL application with 'root'
privileges, it can be executed by the 'root' user, or it can be executed by a
normal user with the 'sudo' command.  To run a PE that is part of a
'distributed' SPL application with 'root' privileges, the PE's executable file
must be changed so that it is owned by the 'root' user, and its Linux 'setuid'
flag is set.

For examples of how to run SPL applications containing PacketLiveSource
operators with special privileges in RHEL/CentOS 5 and 6, see these scripts in
the toolkit:

    .../scripts/TestPacketLiveSource1.standalone.sh
    .../scripts/TestPacketLiveSource1.distributed.sh

This operator is part of the network toolkit. The toolkit also includes sample
applications that illustrate how to use the operator.


_____Dependencies_____


The PacketLiveSource operator depends upon the Linux 'packet capture library
(libpcap)'.  The library must be installed on the machine where this operator
executes. It is available as an installable 'repository package (RPM)' from the
'base' RHEL and CentOS repositories, and can be installed with administrator
tools such as 'yum'.  This requires root privileges, which can be acquired
temporarily with administrator tools such as 'sudo'.

To install 'libpcap' as an administrator for all users, enter this command at a
Linux command prompt:

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
of libpcap instead of the system version, set these environment variables before
compiling an application that contains the PacketLiveSource operator:

    export STREAMS_ADAPTERS_LIBPCAP_INCLUDEPATH=.../directory/libpcap-X.Y.Z
    export STREAMS_ADAPTERS_LIBPCAP_LIBPATH=.../directory/libpcap-X.Y.Z

See the 'TestPacketLiveSourceX.standalone.sh' scripts in the 'scripts' directory
for examples of compiling an application with alternative versions of libpcap.

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


The PacketLiveSource operator has no input ports.


_____Output Ports_____


The PacketLiveSource operator has one required output port and one optional
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

The PacketLiveSource operator does not produce punctuation.


_____Parameters_____


   networkInterface -- This required parameter takes an expression of type
      'rstring' that specifies which network interface the operator will capture
      packets from.  The value must be one of the network interface names
      configured in Linux on the machine where the operator will execute.  To
      display a list of configured network interface names, type this at a Linux
      command prompt:

      /sbin/ifconfig

   maximumLength -- This optional parameter takes an expression of type 'uint16'
      that specifies the maximum length in bytes of the packet data that will be
      produced by the operator.  This length includes all network headers;
      packets longer than the specified length will be truncated.

      The default value of the 'maximumLength' parameter is 65,535 bytes.

   filterExpression -- This optional parameter takes an expression of type
      'rstring' that specifies which captured packets should be produced as
      tuples. The value of this string must be a valid 'pcap' filter expression,
      as documented in the manual page 'pcap-filter', for example here:

          http://linux.die.net/man/7/pcap-filter

      The default value of the 'filterExpression' parameter is an empty string,
      which causes all captured packets to be produced as tuples.

      Note that some values of the 'filterExpression' parameter, including the
      default value, require that 'promiscuous' mode be enabled on the network
      interface, which requires that the operator execute with 'root'
      privileges.

   promiscuous -- This optional parameter takes an expression of type 'boolean'
      that specifies whether or not 'promiscuous' mode should be enabled on the
      network interface.

      The default value of the 'promiscuous' parameter is 'false'.  However,
      some values of the 'filterExpression' parameter, including the default
      value, require that 'promiscuous' mode be enabled on the network
      interface, even if the 'promiscous' parameter is not specified, or set to
      'false'.

      Note that if 'promiscious' mode is enabled on the network interface, the
      operator must execute with 'root' privileges.

   initDelay -- This optional parameter takes an expression of type 'float64'
      that specifies the number of seconds the operator will wait before it
      begins to produced tuples.

      The default value of the 'initDelay' parameter is '0.0'.

   machineID -- This optional parameter takes an expression of type 'int32' that
      specifies a value for the 'machineID' field of the SPL type 'timestamp',
      to be used when the 'timestampp()' output function is assigned to an
      attribute of SPL type 'timestamp'.

      The default value of the 'machineID' parameter is '0'.

   bufferSize -- This optional parameter takes an expression of type 'uint32'
      that specifies the size, in bytes, of the 'libpcap' buffer used for
      receiving packets.  The maximum value allowed is 2,147,483,647 (that is,
      2**31-1).

      The default value of the 'bufferSize' parameter is determined by 'libpcap'.

   processorAffinity -- This optional parameter takes an expression of type
      'unit32' that specifies which processor core the operator's thread will
      run on.  The maximum value is 'P-1', where P is the number of processors
      on the machine where the operator will run.  If this parameter is
      specified, then the operator's thread will be dispatched only on the
      specified processor.

      The default is to dispatch the operator's thread on any available processor.

   timeout -- This optional parameter takes an expression of type 'float64' that
      specifies a timeout for the 'libpcap' interface.  If this value is
      specified, the operator will call the 'libpcap' interface again whenever
      it times out, until the PE containing the operator receives a shutdown
      signal.  

      The default is no timeout.  This may cause the thread that runs the
      'libpcap' interface to on shutdown until another packet is received.

   timestampType -- This optional parameter takes a value of 'host', 'adapter',
      or 'adapter_unsynced', which specifies which type of timestamp will be
      assigned to packets as they are received.

      The default value is 'host'.

   statisticsInterval -- This optional parameter is valid only if a second
      output port is defined for the operator.  It takes an expression of type
      'float64' that specifies the interval, in seconds, for producing
      statistics tuples on that output port.

      The default value of the 'statisticsInterval' parameter is '1.0'.


_____Windowing_____


The PacketLiveSource operator does not accept any window congfiguration.


_____Assignments____


The PacketLiveSource operator requires that all attributes of the first output
port be assigned values with one of these output functions:

   <numeric T> T length() -- This function assigns the length of a captured
      packet to an output attribute.  The value includes all network headers and
      the entire packet body.  Note that this value may be larger than the
      length of the binary data assigned to a 'blob' attribute by the 'packet()'
      function if the packet is truncated due to the parameter 'maximumLength'.

   blob packet() -- This function assigns the captured packet data to an output
      attribute.  The value includes all network headers and the entire packet
      body.  Note that the data may be truncated due to the parameter
      'maximumLength'.

   <numeric T> T packetNumber() -- This function assigns the sequence number of
      this packet tuple, starting at zero, incrementing by one for each packet
      tuple produced by the operator.

   <numeric T> T sessionKey() -- This function assigns a 32-bit key, based on
      the IP version 4 source and destination addresses, and for UDP and TCP
      packets, the source and destination port numbers.  The addresses and ports
      are combined such that packets flowing in either direction between
      endpoints have the same session key.  The key can be used to distribute
      packets across multiple parallel processing channels such that all packets
      that are part of a UDP or TCP session are routed to the same instance of
      the processor.

   <numeric|timestamp T> T timestampp() -- This function assigns the time that a
      packet was captured.  The value is represented as seconds and microseconds
      since the beginning of the Unix epoch (midnight in the GMT/UTC timezone on
      January 1st, 1970), according to the system clock.  The attribute must be
      either a numeric type or SPL type 'timestamp'.  For attributes of type
      'timestamp', the 'machineID' field is set according to the 'machineID'
      parameter.

   int64 timestampSeconds() -- This function assigns the integer portion of the
      time that a packet was captured, that is, the number of seconds since the
      beginning of the Unix epoch (midnight in the GMT/UTC timezone on January
      1st, 1970), according to the system clock.

   int32 timestampMicroseconds() -- This function assigns the fractional portion
      of the time that a packet was captured, that is, the number of
      microseconds since the value assigned by the TimestampSeconds() function.

The PacketLiveSource operator requires that all attributes of the second output
port, if defined, be assigned values with one of these output functions:

   <numeric T> T packetsReceived() -- This function assigns the number of
      packets received by the network interface during the current interval, as
      counted by 'libpcap'.

   <numeric T> T packetsDropped() -- This function assigns the number of packets
      dropped during the current interval because there was no room in the
      operating system's buffer when they arrived, as counted by 'libpcap'.  The
      value will always be zero on Linux kernels prior to version 2.4, which do
      not count dropped packets.  You can display the kernel version of your
      machine by typing this at a Linux command prompt:

          uname -r

   <numeric T> T packetsDroppedByInterface() -- This function assigns number of
      packets dropped by the network interface adapter or its kernel driver, as
      counted by 'libpcap'.  The value may be zero if the counter is not
      implemented by the adapter or driver.

   <numeric T> T packetsProduced() -- This function assigns the number of packet
      tuples produced by the first output port during the current interval, as
      counted by the operator.

   <numeric T> T octetsProduced() -- This function assigns the number of bytes,
      including network headers, in the packet tuples produced by the first
      output port during the current interval, as counted by the operator.

   <numeric|timestamp T> T timestampp() -- This function assigns the time that
      statistics were produced by the PacketLiveSource operator.  The value is
      represented as seconds and microseconds since the beginning of the Unix
      epoch (midnight in the GMT/UTC timezone on January 1st, 1970), according
      to the system clock.  The attribute must be either a numeric type or SPL
      type 'timestamp'.  For attributes of type 'timestamp', the 'machineID'
      field is set according to the 'machineID' parameter.



_____Threads_____


The PacketLiveSource operator contains two or three threads of execution:

  * The first thread executes 'libpcap', which receives network packets from the
    operating system and produces packet tuples containing them on the first output
    port.

  * The second thread provides Streams metrics when the operator is executed in a
    Distributed mode application.

  * The third thread produces statistics tuples when a second output port is
    defined.


_____Metrics_____


The PacketLiveSource operator provides these Streams metrics when it is executed
in Distributed mode application:


  * nPacketsReceivedCurrent: the number of packets received by the network
    interface, as counted by 'libpcap'.

  * nPacketsDroppedCurrent: the number of packets dropped because there was no
    room in the operating system's buffer when they arrived, as counted by
    'libpcap'.

  * nPacketsDroppedByInterfaceCurrent: number of packets dropped by the network
    interface adapter or its kernel driver, as counted by 'libpcap'.  The value
    may be zero if the counter is not implemented by the adapter or driver.



_____Exceptions_____


The PacketLiveSource operator will throw an exception and terminate in these
situations:

   -- The parameter 'networkInterface' does not specify a network interfaces
      defined in Linux on the machine where the operator executes.

   -- The parameter 'filterExpression' does not specify a valid 'pcap' filter
      expression.


_____Tuning_____


The PacketLiveSource operator is often used in applications that ingest packets
at very high rates.  In these situations, the rate may be limited by the size of
the Linux network buffers.  If so, you can increase the size of these buffers by
setting Linux network configuration parameters.  Note that root privileges are
required to do this.

You can display the current values of the ethernet and IP configuration
parameters by entering these commands at a Linux prompt:

    /sbin/sysctl net.core
    /sbin/sysctl net.ipv4
  
For example, these are typical default values for Linux configuration parameters
in RHEL/CentOS 5 and 6:

    net.core.wmem_default = 124928
    net.core.rmem_default = 124928
    net.core.rmem_max = 131071
    net.core.wmem_max = 131071
    net.core.netdev_max_backlog = 1000

    net.ipv4.tcp_rmem = 4096        87380   4194304
    net.ipv4.tcp_wmem = 4096        16384   4194304
    net.ipv4.tcp_window_scaling=1

You can change the current values of Linux configuration parameters by executing
these commands at a Linux prompt.  For example:

    # allow packet buffers to increase up to 64MB 
    sudo /sbin/sysctl -w net.core.rmem_max=67108864
    sudo /sbin/sysctl -w net.core.wmem_max=67108864 

    # increase the length of the processor input queue to 50,000
    sudo /sbin/sysctl -w net.core.netdev_max_backlog=50000

    # increase autotuning TCP buffer limit to 32MB
    sudo /sbin/sysctl -w net.ipv4.tcp_rmem=10240 87380 33554432
    sudo /sbin/sysctl -w net.ipv4.tcp_wmem=10240 87380 33554432

Changes to Linux configuration parameters with /sbin/sysctl persist only for the
current instance of Linux; they will revert to their default values when Linux
is rebooted. To change the default values used when Linux boots, specify them in
the Linux system file '/etc/sysctl.cfg'.

For more information, refer to 'http://fasterdata.es.net/host-tuning/linux/'.


_____Examples_____


This operator declaration will configure a PacketLiveSource operator that
captures all packets received by network interface "eth0", and produce tuples
containing all of the packet data plus the time the packets were captured:
	
    stream<
	float64 captureTime, 
    	blob rawPacket
    > PacketStream as Out = PacketLiveSource() {
        param
	    networkInterface: "eth0";
	output Out:
	    captureTime = timestampp(),
	    rawPacket = packet(); }

This operator declaration will configure a PacketLiveSource operator to capture
only DNS and DHCP packets received by network interface "eth4", and produce
tuples containing all of the packet data plus the time the packets were
captured:

    stream<
	float64 captureTime, 
	blob rawPacket
    > PacketStream as Out = PacketLiveSource() {
        param
	    networkInterface: "eth4";
	    filterExpression: "udp port 53 or udp port 67";
	output Out:
	    captureTime = timestampp(),
	    rawPacket = packet(); }

This operator declaration will configure a PacketLiveSource operator to capture
only the first 512 bytes of data from packets recevied on port 2055, and assign
the time of capture to several attributes of various types:

    stream<
	uint32 uint32CaptureTime,
	float64 float64CaptureTime,
	timestampCaptureTime,
	blob rawPacket
    > PacketStream as Out = PacketLiveSource() {
        param
	    networkInterface: "eth4";
	    filterExpression: "port 2055";
	    maximumLength: 512;
	output Out:
	    uint32CaptureTime = timestampp(),
	    float64CaptureTime = timestampp(),
	    timestampCaptureTime = timestampp(),
	    rawPacket = packet(); }

The PacketLiveSource operator may require 'root' privileges to execute properly.
The project contains two scripts that illustrate how to do this for 'standalone'
and 'distributed' applications:

    .../scripts/TestPacketLiveSource1.standalone.sh   
    .../scripts/TestPacketLiveSource1.distributed.sh   


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
