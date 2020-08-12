---
title: "Toolkit Usage Overview"
permalink: /docs/user/overview/
excerpt: "How to use this toolkit."
last_modified_at: 2017-08-04T12:37:48-04:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "userdocs"
---
{% include toc %}
{%include editme %}


The network toolkit enables SPL applications to analyze low level network packets such as parsing DHCP, DNS, Netflow, IPFIX messages, enriching IPV4 and IPV6 addresses with geospatial data as well as converting IP address between binary and string representation. The following section will provide you with a high level overview on the capabilities of the operators within this toolkit and a simple example on how to use the operator to process network packet data.

## Operator Overview (alphabetically sorted)

**DHCPMessageParser**
:
 
The DHCPMessageParser is an operator that parses the DHCP message fields received as input tuples and emits tuples containing the DHCP message data. Users can then use DHCP parser result functions to further process the payload data. For example the DHCP_DOMAIN_NAME() function returns the domain name of the client and DHCP_SERVER_ADDRESS() function returns the IPV4 address of the DHCP server.

**DNSMessageParser**
:
 
This operator parses individual DNS message field received in input tuples and emits tuples containing DNS message data. Users can then use DNS parser result functions to further process the payload data.

**IPAddressLocation**
:
 
The IPAddressLocation operator utilizes geographical location data provided by the MaxMind Inc
and finds the geographical location of IP address received in the input tuples and emits output tuples containing the country, state, province, city , latitude and logitude of the subnet.

**IPASNEnricher**
:
 
This operator enriches IPV4 and IPV6 addresses by mapping them to ASN (Autonomous System Numbers) used by MaxMind GeoLite ASN database.

**IPFIXMessageParser**
:
 
The IPFIXMessageParser is an operator that parses the IPFIX (Internet Protocol Flow Information Export protocol) message fields received in input tuples and emits tuples containing message data. Users can then uses IPFIX parser result functions to further process packet payload data.

**IPSpatialEnricher**
:
 
This operator enriches IPV4 and IPV6 addresses by mapping them to geospatial data used by MaxMind GeoIP2 database.

**PacketContentAssembler**
:
 
This operator reassembles flows such as SMTP,FTP,HTTP and files such as GIF,JPEG,HTML,PDF from raw network packets received as tuples and emits the fully assembled content to users.

**PacketDPDKSource**
:
 
PacketDPDKSource operator provides the same functionality as the PacketLiveSource operator. The primary difference between these two operators is that the PacketDPDKSource operator utilizes the DPDK libraries at
http://www.dpdk.org
for improved performance with lower processing overhead.

**PacketFileSource**
:
 
This operator reads network packet from ?pcap? packet capture files and parses network headers and emits tuples containing packet data. Users can then use the network toolkit functions to further process the packet such as retrieving its source and destination ip address, ip version,payload and protocol information etc?

**PacketLiveSource**
:
 
PacketLiveSource operator is very similar to PacketFileSource operator. The difference is that instead of parsing static pcap files, it is able to capture live network packets from an ethernet interface. Users can then use a downstream operator such as DNS,DHCP or IPFIX message parser to further process its payload data.

## SPLDOC

[SPLDoc for the com.ibm.streamsx.network toolkit](https://ibmstreams.github.io/streamsx.network/doc/spldoc/html/index.html)


## Environment Setup

1. Install libpcap-devel rpm from your linux repository.
2. Import the SampleNetworkToolkitData spl project which contains sample PCAP files needed to run network sample applications.
![Import](/doc/images/pcap1.jpg)
3. Alternatively you can also generate a pcap file by running the tcpdump command against your ethernet interface.

```
[root@oc2756228212 tmp]# tcpdump -i wlp3s0 -w test.pcap

tcpdump: listening on wlp3s0, link-type EN10MB (Ethernet), capture size 65535 bytes

178 packets captured

178 packets received by filter

0 packets dropped by kernel
```
4. Import network sample spl applications to your Streams Studio work space.
5. Locate the pcap file by editing the following spl code : expression $pcapFilename: getSubmissionTimeValue("pcapFilename", "../../SampleNetworkToolkitData/data/(your pcap file)" )
6. Compile and submit the job.
![Import](/doc/images/studio.jpg)

## Sample applications

The sample projects contain several SPL applications to demonstrate the usage of an operator.

* The sample applications for the "...File..." operators can be executed on any machine with ordinary user privileges.
* The sample applications for the "...Live..." operators require root privileges to execute, and some of them require specially-configured routers and physical wiring, as described in their documentation.
* The sample applications for the "...DPDK..." operators require special network adapters and system software to execute, as well as root privileges, specially-configured routers, and physical wiring.

### Building and launching the sample applications

To build and launch the sample application you can launch a script.

For example:

    cd SamplePacketFileSource/script
    ./testPacketFileSourceBasic1.sh

* The `test...sh` scripts can be executed on any machine with ordinary user privileges.
* The `live...sh` scripts require root privileges to execute.

## SampleNetworkToolkitData

This project contains the sample (PCAP) input files for several sample applications.

## Samples for the IPASNEnricher and IPSpatialEnricher operators

Samples for the operators IPASNEnricher and IPSpatialEnricher will be available on github:

[IPASNEnricher and IPSpatialEnricher samples](https://github.com/IBMStreams/streamsx.cybersecurity.starterApps/tree/main/PredictiveBlocklistingSamples)
