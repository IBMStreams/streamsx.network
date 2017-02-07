#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

namespace=sample
composite=LivePacketLiveSourceBasic2

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
[[ -f $STREAMS_INSTALL/toolkits/com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$STREAMS_INSTALL/toolkits
[[ -f $here/../../../../toolkits/com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$( cd $here/../../../../toolkits ; pwd )
[[ -f $here/../../../com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$( cd $here/../../.. ; pwd )
[[ $toolkitDirectory ]] || die "sorry, could not find 'toolkits' directory"

buildDirectory=$projectDirectory/output/build/$composite

unbundleDirectory=$projectDirectory/output/unbundle/$composite

dataDirectory=$projectDirectory/data

networkInterface=$( ifconfig eno1 1>/dev/null 2>&1 && echo eno1 || echo eth0 ) 
#etworkInterface=enP2p1s0
#networkInterface=enP3p5s0f3

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
networkInterface=$networkInterface
processorAffinity=1
metricsInterval=1.0
timeoutInterval=10.0
)

traceLevel=3 # ... 0 for off, 1 for error, 2 for warn, 3 for info, 4 for debug, 5 for trace

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"

#[ ! -d $buildDirectory ] || rm -rf $buildDirectory || die "Sorry, could not delete old '$buildDirectory', $?"
[ -d $dataDirectory ] || mkdir -p $dataDirectory || die "Sorry, could not create '$dataDirectory, $?"

step "configuration for standalone application '$namespace.$composite' ..."
( IFS=$'\n' ; echo -e "\nStreams toolkits:\n${toolkitList[*]}" )
( IFS=$'\n' ; echo -e "\nStreams compiler options:\n${compilerOptionsList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite compile-time parameters:\n${compileTimeParameterList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ntrace level: $traceLevel"

step "building standalone application '$namespace.$composite' ..."
sc ${compilerOptionsList[*]} -- ${compileTimeParameterList[*]} || die "Sorry, could not build '$composite', $?" 

step "executing standalone application '$namespace.$composite' ..."
executable=$buildDirectory/bin/standalone.exe
sudo STREAMS_INSTALL=$STREAMS_INSTALL $executable -t $traceLevel ${submitParameterList[*]} || die "sorry, application '$composite' failed, $?"

#step "unbundling standalone application '$namespace.$composite' ..."
#bundle=$buildDirectory/$namespace.$composite.sab
#[ -f $bundle ] || die "sorry, bundle '$bundle' not found"
#spl-app-info $bundle --unbundle $unbundleDirectory || die "sorry, could not unbundle '$bundle', $?"

#step "setting capabilities for standalone application '$namespace.$composite' ..."
#standalone=$unbundleDirectory/$composite/bin/standalone
#[ -f $standalone ] || die "sorry, standalone application '$standalone' not found"
#sudo /usr/sbin/setcap 'CAP_NET_RAW+eip CAP_NET_ADMIN+eip' $standalone || die "sorry, could not set capabilities for application '$composite', $?"
#/usr/sbin/getcap -v $standalone || die "sorry, could not get capabilities for application '$composite', $?"

#step "executing standalone application '$namespace.$composite' ..."
#$standalone -t $traceLevel ${submitParameterList[*]} || die "sorry, application '$composite' failed, $?"

exit 0
