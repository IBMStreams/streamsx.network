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

parallelWidth=2

buildDirectory=$projectDirectory/output/build/$composite

dataDirectory=$projectDirectory/data

# logical cores for two NUMA nodes with a total of 56 or 95 cores 

node0CoresOf56=( $( seq  0 13 ; seq 28 41 ) )
node1CoresOf56=( $( seq 14 27 ; seq 42 55 ) )

node0CoresOf96=( $( seq  0 23 ; seq 48 71 ) )
node1CoresOf96=( $( seq 24 47 ; seq 72 95 ) )

# core assignments for DPDK on 56 and 96 core machines

masterCoreOf56=${node1CoresOf56[0]}
masterCoreOf96=${node1CoresOf96[0]} 

ingestCoresOf56=( ${node1CoresOf56[@]:1:$parallelWidth} )
ingestCoresOf96=( ${node1CoresOf96[@]:1:$parallelWidth} )

# submission-time parameters for specific machines

hostname=$( hostname -s )
#[[ $hostname == "c0321" ]] && machineType=Broadwell && adapterType=ConnectX3Pro && masterCore=$masterCoreOf56 && ingestCores=( ${ingestCoresOf56[*]} ) 
#[[ $hostname == "c0323" ]] && machineType=Broadwell && adapterType=ConnectX3    && masterCore=$masterCoreOf56 && ingestCores=( ${ingestCoresOf56[*]} ) 
#[[ $hostname == "c0325" ]] && machineType=Broadwell && adapterType=ConnectX3    && masterCore=$masterCoreOf56 && ingestCores=( ${ingestCoresOf56[*]} ) 
 [[ $hostname == "c0327" ]] && machineType=Broadwell && adapterType=ConnectX3    && masterCore=$masterCoreOf56 && ingestCores=( ${ingestCoresOf56[*]} ) 
#[[ $hostname == "c1221" ]] && machineType=Skylake   && adapterType=ConnectX3    && masterCore=$masterCoreOf96 && ingestCores=( ${ingestCoresOf96[*]} ) 
#[[ $hostname == "c1224" ]] && machineType=Skylake   && adapterType=ConnectX3    && masterCore=$masterCoreOf96 && ingestCores=( ${ingestCoresOf96[*]} ) 
 [[ $hostname == "h0711" ]] && machineType=Haswell   && adapterType=ConnectX3    && masterCore=$masterCoreOf56 && ingestCores=( ${ingestCoresOf56[*]} )
 [[ -n $machineType ]] || die "sorry, host '$hostname' not recognized"

# submission-time parameters
submitParameterList=(
    adapterPort=0
    dpdkBufferSize=200
    dpdkMasterCore=$masterCore
    dpdkIngestCores=$( IFS=, ; echo -e "[${ingestCores[*]}]" )
    parallelWidth=$parallelWidth
)
traceLevel=3 # ... 0 for off, 1 for error, 2 for warn, 3 for info, 4 for debug, 5 for trace

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"
[ -d $dataDirectory ] || mkdir -p $dataDirectory || die "Sorry, could not create '$dataDirectory, $?"
chmod go+w $dataDirectory

step "configuration for standalone application '$namespace.$composite' ..."
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ntrace level: $traceLevel"

step "debugging '$composite' on host $hostname ($machineType) with adapter $adapterType ..."
executable=$buildDirectory/bin/standalone.exe
#executable=$buildDirectory/bin/standalone
sudo -E gdb --args $executable -t $traceLevel ${submitParameterList[*]} 

exit 0
