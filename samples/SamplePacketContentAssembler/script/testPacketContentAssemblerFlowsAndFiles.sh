#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

namespace=sample
composite=TestPacketContentAssemblerFlowsAndFiles

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
[[ -f $STREAMS_INSTALL/toolkits/com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$STREAMS_INSTALL/toolkits
[[ -f $here/../../../../toolkits/com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$( cd $here/../../../../toolkits ; pwd )
[[ -f $here/../../../com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$( cd $here/../../.. ; pwd )
[[ $toolkitDirectory ]] || die "sorry, could not find 'toolkits' directory"

[[ -f $STREAMS_INSTALL/samples/com.ibm.streamsx.network/SampleNetworkToolkitData/info.xml ]] && samplesDirectory=$STREAMS_INSTALL/samples/com.ibm.streamsx.network
[[ -f $here/../../SampleNetworkToolkitData/info.xml ]] && samplesDirectory=$( cd $here/../.. ; pwd )
[[ $samplesDirectory ]] || die "sorry, could not find 'samples' directory"

[[ -f $HOME/com.ibm.iss.pam/pam.h ]] && libpamDirectory=$( cd $HOME/com.ibm.iss.pam ; pwd )
[[ -f $here/../../../../com.ibm.iss.pam/com.ibm.iss.pam/pam.h ]] && libpamDirectory=$( cd $here/../../../../com.ibm.iss.pam/com.ibm.iss.pam ; pwd )
[[ -f $STREAMS_ADAPTERS_ISS_PAM_DIRECTORY/pam.h ]] && libpamDirectory=$( cd $STREAMS_ADAPTERS_ISS_PAM_DIRECTORY ; pwd )
[[ $libpamDirectory ]] || die "sorry, could not find 'libpam' directory"

buildDirectory=$projectDirectory/output/build/$composite

dataDirectory=$projectDirectory/data

coreCount=$( cat /proc/cpuinfo | grep processor | wc -l )

toolkitList=(
$toolkitDirectory/com.ibm.streamsx.network
$samplesDirectory/SampleNetworkToolkitData
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
pcapFilename=$samplesDirectory/SampleNetworkToolkitData/data/sample_http+https.pcap
#pcapFilename=/mnt/scratch/data.garanti-bank/Ayca_Test_wireshark.pcap
)

traceLevel=3 # ... 0 for off, 1 for error, 2 for warn, 3 for info, 4 for debug, 5 for trace

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"

#[ ! -d $buildDirectory ] || rm -rf $buildDirectory || die "Sorry, could not delete old '$buildDirectory', $?"
[ -d $dataDirectory ] || mkdir -p $dataDirectory || die "Sorry, could not create '$dataDirectory, $?"
[ -d $libpamDirectory ] && export STREAMS_ADAPTERS_ISS_PAM_DIRECTORY=$libpamDirectory

step "configuration for standalone application '$namespace.$composite' ..."
( IFS=$'\n' ; echo -e "\nStreams toolkits:\n${toolkitList[*]}" )
( IFS=$'\n' ; echo -e "\nStreams compiler options:\n${compilerOptionsList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite compile-time parameters:\n${compileTimeParameterList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite submission-time parameters:\n${submitParameterList[*]}" )
echo -e "\ntrace level: $traceLevel"
echo -e "\nPAM library: $libpamDirectory"

step "building standalone application '$namespace.$composite' ..."
sc ${compilerOptionsList[*]} -- ${compileTimeParameterList[*]} || die "Sorry, could not build '$namespace::$composite', $?" 

step "executing standalone application '$namespace.$composite' ..."
executable=$buildDirectory/bin/$namespace.$composite
#$here/debugthis.sh ...
$executable -t $traceLevel ${submitParameterList[*]} || die "sorry, application '$composite' failed, $?"

exit 0


