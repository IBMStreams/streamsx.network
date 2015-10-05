#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

namespace=sample
composite=LiveDNSMessageParserAnalysis

self=$( basename $0 .sh )
here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
toolkitDirectory=$( cd $here/../../.. ; pwd )

buildDirectory=$projectDirectory/output/build/$composite.distributed
dataDirectory=$projectDirectory/data
logDirectory=$projectDirectory/log

libpcapDirectory=$HOME/libpcap-1.7.4

coreCount=$( cat /proc/cpuinfo | grep processor | wc -l )

domain=CapabilitiesDomain
instance=CapabilitiesInstance

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
-P networkInterface=ens6f3
-P "inputFilter=udp port 53"
-P metricsInterval=1.0
-P timeoutInterval=30.0
-P errorStream=true
)

tracing=info # ... one of ... off, error, warn, info, debug, trace

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

[ -d $logDirectory ] || mkdir -p $logDirectory || echo "sorry, could not create directory '$logDirectory', $?"

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"

#[ ! -d $buildDirectory ] || rm -rf $buildDirectory || die "Sorry, could not delete old '$buildDirectory', $?"
[ -d $dataDirectory ] || mkdir -p $dataDirectory || die "Sorry, could not create '$dataDirectory, $?"
[ -d $libpcapDirectory ] && export STREAMS_ADAPTERS_LIBPCAP_INCLUDEPATH=$libpcapDirectory
[ -d $libpcapDirectory ] && export STREAMS_ADAPTERS_LIBPCAP_LIBPATH=$libpcapDirectory

step "configuration for distributed application '$namespace.$composite' ..."
( IFS=$'\n' ; echo -e "\nStreams toolkits:\n${toolkitList[*]}" )
( IFS=$'\n' ; echo -e "\nStreams compiler options:\n${compilerOptionsList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite compile-time parameters:\n${compileTimeParameterList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ndomain: $domain"
echo -e "\ninstance: $instance"
echo -e "\ntracing: $tracing"

step "building distributed application '$namespace.$composite' ..."
sc ${compilerOptionsList[*]} -- ${compileTimeParameterList[*]} || die "Sorry, could not build '$composite', $?" 

step "granting read permission for instance '$instance' log directory to user '$USER' ..."
sudo chmod o+r -R /tmp/Streams-$domain/logs/$HOSTNAME/instances

step "submitting distributed application '$namespace.$composite' ..."
bundle=$buildDirectory/$namespace.$composite.sab
streamtool submitjob -i $instance -d $domain --config tracing=$tracing "${submitParameterList[@]}" $bundle || die "sorry, could not submit application '$composite', $?"

step "waiting while application runs ..."
sleep 25

step "getting logs for instance $instance ..."
streamtool getlog -i $instance -d $domain --includeapps --file $logDirectory/$composite.distributed.logs.tar.gz || die "sorry, could not get logs, $!"

step "cancelling distributed application '$namespace.$composite' ..."
jobs=$( streamtool lspes -i $instance -d $domain | grep $namespace::$composite | gawk '{ print $1 }' )
streamtool canceljob -i $instance -d $domain --collectlogs ${jobs[*]} || die "sorry, could not cancel application, $!"

exit 0
