# streamsx.network Samples

[Sample applications for Streams in Cloud Pak for Data](cpd/README.md) are located in the `cpd` directory.

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

Samples for the operators IPASNEnricher and IPSpatialEnricher will be available on GitHub: https://github.com/IBMStreams/streamsx.cybersecurity.starterApps/tree/develop/PredictiveBlocklistingSamples
