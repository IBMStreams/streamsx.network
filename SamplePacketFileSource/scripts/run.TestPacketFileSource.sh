#!/bin/bash
#set -o xtrace
#set -o pipefail

################### variables used in this script ##############################

namespace=sample
composite=TestPacketFileSource

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
buildDirectory=$projectDirectory/output/build/$composite
dataDirectory=$projectDirectory/data

traceLevel=3 # ... 0 for off, 1 for error, 2 for warn, 3 for info, 4 for debug, 5 for trace

submitParameterList=(
#pcapFilename=dns_sample_100_packets.pcap
)

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"

step "configuration for application '$namespace::$composite' ..."
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ntrace level: $traceLevel"

step "executing standalone application '$namespace::$composite' ..."
executable=$buildDirectory/bin/$namespace.$composite
### ??? gdb --args ...
$executable -t $traceLevel ${submitParameterList[*]} || die "sorry, application '$composite' failed, $?"

exit 0
