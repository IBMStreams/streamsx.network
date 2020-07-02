# Changes
==========

## v3.4.1

* [#204](https://github.com/IBMStreams/streamsx.network/issues/204) CHANGELOG.md added

## v3.4.0:

* New operator com.ibm.streamsx.network.rtp::RtpDecode

## v3.3.1:

* Globalization support: Translated messages updated

## v3.3.0:

* PacketContentAssembler operator does not depend anymore on environment variable STREAMS_ADAPTERS_ISS_PAM_DIRECTORY at build and run time.
  The 'Packet Analysis Module (PAM)' library can be added to the application bundle and the location can be set with the parameters `pamLibrary` and `pamInclude`.
* IPAddressLocation operator: New parameter `initOnTuple` added in order to initialize the operator with loading the geography files on the first tuple and not during operator startup.
* IPAddressLocation operator supports dynamic loading of MaxMind database triggered by a tuple on control port.

## v3.2.2:

* Added the static keyword to a couple of IPv6 helper functions

## v3.2.1:

* Performance enhancements for the PacketDPDKSource.

## v3.1.0:

* Support for newer version of DPDK, in support of RHEL7.4 and forward.
* Additional CIDR functions to determine the range of addresses covered.
* Improved IPFilter performance by avoiding needless creation of local list of IP addresses
* IPFilter operator: Changed traces with WARN trace level to TRACE level.


