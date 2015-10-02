#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

namespace=sample
composite=LiveDNSMessageParserBasic

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
toolkitDirectory=$( cd $here/../../.. ; pwd )

buildDirectory=$projectDirectory/output/build/$composite

unbundleDirectory=$projectDirectory/output/unbundle/$composite

dataDirectory=$projectDirectory/data

libpcapDirectory=$HOME/libpcap-1.7.4

leasePort=23456

leaseWindowOptions=(
-title "$self: DNS Lookups"
-geometry 130x20
+sb
)

coreCount=$( cat /proc/cpuinfo | grep processor | wc -l )

toolkitList=(
$toolkitDirectory/com.ibm.streamsx.network
$toolkitDirectory/samples/SampleNetworkToolkitData
)

compilerOptionsList=(
--verbose-mode
--rebuild-toolkits
--spl-path=$( IFS=: ; echo "${toolkitList[*]}" )
--standalone-application
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
networkInterface=ens6f3
metricsInterval=1.0
timeoutInterval=60.0
leasePort=$leasePort
)

traceLevel=3 # ... 0 for off, 1 for error, 2 for warn, 3 for info, 4 for debug, 5 for trace

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"

#[ ! -d $buildDirectory ] || rm -rf $buildDirectory || die "Sorry, could not delete old '$buildDirectory', $?"
[ -d $dataDirectory ] || mkdir -p $dataDirectory || die "Sorry, could not create '$dataDirectory, $?"
[ -d $libpcapDirectory ] && export STREAMS_ADAPTERS_LIBPCAP_INCLUDEPATH=$libpcapDirectory
[ -d $libpcapDirectory ] && export STREAMS_ADAPTERS_LIBPCAP_LIBPATH=$libpcapDirectory

step "configuration for standalone application '$namespace.$composite' ..."
( IFS=$'\n' ; echo -e "\nStreams toolkits:\n${toolkitList[*]}" )
( IFS=$'\n' ; echo -e "\nStreams compiler options:\n${compilerOptionsList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite compile-time parameters:\n${compileTimeParameterList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ntrace level: $traceLevel"

step "building standalone application '$namespace.$composite' ..."
sc ${compilerOptionsList[*]} -- ${compileTimeParameterList[*]} || die "Sorry, could not build '$namespace::$composite', $?" 

step "unbundling standalone application '$namespace.$composite' ..."
bundle=$buildDirectory/$namespace.$composite.sab
[ -f $bundle ] || die "sorry, bundle '$bundle' not found"
spl-app-info $bundle --unbundle $unbundleDirectory || die "sorry, could not unbundle '$bundle', $?"

step "setting capabilities for standalone application '$namespace.$composite' ..."
standalone=$unbundleDirectory/$composite/bin/standalone
[ -f $standalone ] || die "sorry, standalone application '$standalone' not found"
sudo /usr/sbin/setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' $standalone || die "sorry, could not set capabilities for application '$composite', $?"

step "opening window for TCP stream from standalone application '$namespace.$composite' ..."
( xterm "${leaseWindowOptions[@]}" -e " while [ true ] ; do ncat --recv-only localhost $leasePort && break ; sleep 1 ; done " ) &

step "executing standalone application '$namespace.$composite' ..."
$standalone -t $traceLevel "${submitParameterList[@]}" || die "sorry, application '$composite' failed, $?"

exit 0


