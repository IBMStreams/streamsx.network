#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

namespace=sample
composite=TestPacketFileSourceConsistentRegion

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
toolkitDirectory=$( cd $here/../../.. ; pwd )

buildDirectory=$projectDirectory/output/build/$composite

libpcapDirectory=$HOME/libpcap-1.7.4

checkpointDirectory=$HOME/checkpoint

coreCount=$( cat /proc/cpuinfo | grep processor | wc -l )

instance=ConsistentInstance

toolkitList=(
$toolkitDirectory/com.ibm.streamsx.network
$toolkitDirectory/samples/SampleNetworkToolkitData
)

compilerOptionsList=(
--verbose-mode
--rebuild-toolkits
--spl-path=$( IFS=: ; echo "${toolkitList[*]}" )
--part-mode=FALL
--allow-convenience-fusion-options
--optimized-code-generation
--cxx-flags=-g3
--static-link
--main-composite=$namespace::$composite
--output-directory=$buildDirectory 
--data-directory=data
--num-make-threads=$coreCount
)

compileTimeParameterList=(
)

submitParameterList=(
pcapFilename=$toolkitDirectory/samples/SampleNetworkToolkitData/sample_dns+dhcp.pcap
)

tracing=info # ... one of ... off, error, warn, info, debug, trace

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"

#[ ! -d $buildDirectory ] || rm -rf $buildDirectory || die "Sorry, could not delete old '$buildDirectory', $?"

[ -d $libpcapDirectory ] && export STREAMS_ADAPTERS_LIBPCAP_INCLUDEPATH=$libpcapDirectory
[ -d $libpcapDirectory ] && export STREAMS_ADAPTERS_LIBPCAP_LIBPATH=$libpcapDirectory

step "configuration for distributed application '$namespace::$composite' ..."
( IFS=$'\n' ; echo -e "\nStreams toolkits:\n${toolkitList[*]}" )
( IFS=$'\n' ; echo -e "\nStreams compiler options:\n${compilerOptionsList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite compile-time parameters:\n${compileTimeParameterList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ninstance: $instance"
echo -e "\ntracing: $tracing"

step "building distributed application '$namespace::$composite' ..."
sc ${compilerOptionsList[*]} -- ${compileTimeParameterList[*]} || die "Sorry, could not build '$namespace::$composite', $?" 

step "submitting distributed application '$namespace::$composite' ..."
bundle=$buildDirectory/$namespace.$composite.sab
parameters=$( printf ' --P %s' ${submitParameterList[*]} )
streamtool submitjob --instance-id $instance --config tracing=$tracing $parameters $bundle || die "sorry, could not submit application '$composite', $?"

step "waiting while application runs ..."
sleep 5

step "cancelling distributed application '$namespace::$composite' ..."
jobs=$( streamtool lspes --instance-id $instance | grep $namespace::$composite | gawk '{ print $1 }' )
streamtool canceljob --instance-id $instance --collectlogs ${jobs[*]} || die "sorry, could not cancel application, $!"

exit 0


