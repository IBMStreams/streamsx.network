#!/bin/bash

## Copyright (C) 2011, 2017  International Business Machines Corporation
## All Rights Reserved

#set -o xtrace
#set -o pipefail

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################### parameters used in this script ##############################

namespace=sample
composite=LivePacketDPDKSourceBasic

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )

parallelWidth=1

buildDirectory=$projectDirectory/output/build/$composite

dataDirectory=$projectDirectory/data

# CPU core and memory assignments for DPDK operators

step "get NUMA cores for DPDK interface $DPDK_INTERFACE..."
hostname=$( hostname -s )
numaNode=$( cat /sys/class/net/$DPDK_INTERFACE/device/numa_node )
numaCPUs=( $( ls -1 /sys/devices/system/node/node$numaNode | grep -E "cpu[0-9]+" | tr -d "cpu" | sort -n ) )
dpdkMasterCore=${numaCPUs[0]}
dpdkIngestCores=( ${numaCPUs[@]:1:$parallelWidth} )
echo "$hostname CPUs on NUMA node $numaNode: ${numaCPUs[@]}"
echo "$hostname CPUs for $DPDK_INTERFACE: master $dpdkMasterCore, ingest ${dpdkIngestCores[@]}"

step "get NUMA memory for DPDK interface $DPDK_INTERFACE..."
numaNodes=( $( ls -1 /sys/devices/system/node | grep -E "node" ) ) 
for node in ${numaNodes[@]} ; do dpdkBufferSizes="$dpdkBufferSizes$comma$( [[ $node == node$numaNode ]] && echo '200' || echo '0' )" ; comma=',' ; done
grep -i -h hugepages /sys/devices/system/node/node*/meminfo
echo "$hostname buffersizes for NUMA nodes: $dpdkBufferSizes"

# submission-time parameters

submitParameterList=(
    adapterPort=0
    dpdkMasterCore=$dpdkMasterCore
    dpdkIngestCores=$( IFS=, ; echo -e "[${dpdkIngestCores[*]}]" )
    dpdkBufferSizes=$dpdkBufferSizes
    parallelWidth=$parallelWidth
)
traceLevel=3 # ... 0 for off, 1 for error, 2 for warn, 3 for info, 4 for debug, 5 for trace

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"
[ -d $dataDirectory ] || mkdir -p $dataDirectory || die "Sorry, could not create '$dataDirectory, $?"
rm -f $dataDirectory/debug.*.out || die "sorry, could not delete old debugging output files"
chmod go+w $dataDirectory

step "configuration for standalone application '$namespace.$composite' ..."
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ntrace level: $traceLevel"

step "debugging '$composite' on host $hostname ($machineType) with adapter $adapterType ..."
executable=$buildDirectory/bin/standalone.exe
#executable=$buildDirectory/bin/standalone
#sudo -E ...
gdb --args $executable -t $traceLevel ${submitParameterList[*]} 

exit 0
