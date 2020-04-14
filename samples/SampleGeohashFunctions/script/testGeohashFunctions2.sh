#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

namespace=sample
composite=TestGeohashFunctions2

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
[[ -f $STREAMS_INSTALL/toolkits/com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$STREAMS_INSTALL/toolkits
[[ -f $here/../../../../toolkits/com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$( cd $here/../../../../toolkits ; pwd )
[[ -f $here/../../../com.ibm.streamsx.network/info.xml ]] && toolkitDirectory=$( cd $here/../../.. ; pwd )
[[ $toolkitDirectory ]] || die "sorry, could not find 'toolkits' directory"

buildDirectory=$projectDirectory/output/build/$composite

dataDirectory=$projectDirectory/data

geohashFilename=$projectDirectory/data/www.maxmind.com+w3.ibm.com/GeoLite2-City-Geohashes-en.csv

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
    geohashFilename=$geohashFilename
)

traceLevel=3 # ... 0 for off, 1 for error, 2 for warn, 3 for info, 4 for debug, 5 for trace

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

[[ ! -f $geohashFilename ]] && echo "Sorry, could not find file $geohashFilename" && exit 0

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
executable=$buildDirectory/bin/$namespace.$composite
$executable -t $traceLevel ${submitParameterList[*]} || die "sorry, application '$composite' failed, $?"

exit 0


