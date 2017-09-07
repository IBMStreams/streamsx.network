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


## SPLDOC

[SPLDoc for the com.ibm.streamsx.network toolkit](https://ibmstreams.github.io/streamsx.network/doc/spldoc/html/index.html)


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

[IPASNEnricher and IPSpatialEnricher samples](https://github.com/IBMStreams/streamsx.cybersecurity.starterApps/tree/master/PredictiveBlacklistingSamples/com.ibm.streamsx.cybersecurity.sample)
