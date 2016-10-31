#!/bin/bash

## Copyright (C) 2016  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

namespace=sample
composite=LiveDNSPacketDPDKSourceBasic

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
toolkitDirectory=$( cd $here/../../.. ; pwd )

buildDirectory=$projectDirectory/output/build/$composite

unbundleDirectory=$projectDirectory/output/unbundle/$composite

dataDirectory=$projectDirectory/data

dpdkDirectory=$RTE_SDK/build/lib

coreCount=$( cat /proc/cpuinfo | grep processor | wc -l )

toolkitList=(
$toolkitDirectory/com.ibm.streamsx.network
)

compilerOptionsList=(
--verbose-mode
--rebuild-toolkits
--spl-path=$( IFS=: ; echo "${toolkitList[*]}" )
--standalone-application
--optimized-code-generation
--static-link
--main-composite=$namespace::$composite
--output-directory=$buildDirectory 
--data-directory=data
--num-make-threads=$coreCount
)

gccOptions="-g3"

ldOptions="-Wl,-L -Wl,$dpdkDirectory -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,-libverbs -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--no-whole-archive"

compileTimeParameterList=(
)

submitParameterList=(
nicPort=0
nicQueue=0
timeoutInterval=10.0
)

traceLevel=3 # ... 0 for off, 1 for error, 2 for warn, 3 for info, 4 for debug, 5 for trace

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"

step "checking for DPDK libraries ..."
[[ -d $dpdkDirectory ]] || die "sorry, could not find DPDK directory '$dpdkDirectory'"
[[ -f $dpdkDirectory/libdpdk.a ]] || die "sorry, could not find DPDK library '$dpdkDirectory/libdpdk.a'"
[[ -d $toolkitDirectory/com.ibm.streamsx.network/impl/src/source/dpdk/build/lib ]] || die "sorry, could not find DPDK glue library directory '$toolkitDirectory/com.ibm.streamsx.network/impl/src/source/dpdk/build/lib'"
[[ -f $toolkitDirectory/com.ibm.streamsx.network/impl/src/source/dpdk/build/lib/libstreams_source.a ]] || die "sorry, could not find DPDK glue library '$toolkitDirectory/com.ibm.streamsx.network/impl/src/source/dpdk/build/lib/libstreams_source.a'"

#[ ! -d $buildDirectory ] || rm -rf $buildDirectory || die "Sorry, could not delete old '$buildDirectory', $?"
[ -d $dataDirectory ] || mkdir -p $dataDirectory || die "Sorry, could not create '$dataDirectory, $?"

step "configuration for standalone application '$namespace.$composite' ..."
( IFS=$'\n' ; echo -e "\nStreams toolkits:\n${toolkitList[*]}" )
( IFS=$'\n' ; echo -e "\nStreams compiler options:\n${compilerOptionsList[*]}" )
echo -e "\nGNU compiler parameters:\n$gccOptions" 
echo -e "\nGNU linker parameters:\n$ldOptions" 
( IFS=$'\n' ; echo -e "\n$composite compile-time parameters:\n${compileTimeParameterList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ntrace level: $traceLevel"

step "building standalone application '$namespace.$composite' ..."
sc ${compilerOptionsList[*]} "--cxx-flags=$gccOptions" "--ld-flags=$ldOptions" -- "${compileTimeParameterList[*]}" || die "Sorry, could not build '$composite', $?" 

step "deleting old '/dev/hugepages/rtemap_*' files, if necessary ..."
[[ ! -f /dev/hugepages/rtemap_* ]] || sudo rm /dev/hugepages/rtemap_* || die "sorry, could not delete /dev/hugepages/rtemap_* files, $?"

step "executing standalone application '$namespace.$composite' ..."
executable=$buildDirectory/bin/$namespace.$composite
$executable -t $traceLevel ${submitParameterList[*]}

step "deleting '/dev/hugepages/rtemap_*' files ..."
[[ ! -f /dev/hugepages/rtemap_* ]] || sudo rm /dev/hugepages/rtemap_* || die "sorry, could not delete /dev/hugepages/rtemap_* files, $?"

exit 0
