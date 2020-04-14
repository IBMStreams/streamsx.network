#!/bin/bash

## Copyright (C) 2011, 2017  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

namespace=sample
composite=LivePacketDPDKSourceBasic

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )

buildDirectory=$projectDirectory/output/build/$composite

dataDirectory=$projectDirectory/data

dpdkDirectory=$RTE_SDK/$RTE_TARGET/lib

coreCount=$( cat /proc/cpuinfo | grep processor | wc -l )

toolkitList=(
    $HOME/git/streamsx.network/com.ibm.streamsx.network
)

compilerOptionsList=(
    --verbose-mode
    --rebuild-toolkits
    --spl-path=$( IFS=: ; echo "${toolkitList[*]}" )
    --standalone-application
    --static-link
    --optimized-code-generation
    --main-composite=$namespace::$composite
    --output-directory=$buildDirectory 
    --data-directory=data
    --num-make-threads=$coreCount
)

gccOptions="-g3 -fPIC -m64 -pthread -fopenmp"

ldOptions="-Wl,-L -Wl,$dpdkDirectory -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,-libverbs -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=app.map -Wl,--cref"

precompileParameterList=(
)

compileTimeParameterList=(
)

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"
[ -d $dataDirectory ] || mkdir -p $dataDirectory || die "Sorry, could not create '$dataDirectory, $?"

step "configuration for compiling standalone application '$namespace.$composite' ..."
( IFS=$'\n' ; echo -e "\nStreams toolkits:\n${toolkitList[*]}" )
( IFS=$'\n' ; echo -e "\nStreams compiler options:\n${compilerOptionsList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite precompile parameters:\n${precompileParameterList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite compile-time parameters:\n${compileTimeParameterList[*]}" )
echo -e "\nGNU compiler parameters:\n$gccOptions" 
echo -e "\nGNU linker parameters:\n$ldOptions" 

step "compiling standalone application '$namespace.$composite' ..."
sc ${compilerOptionsList[*]} "--cxx-flags=$gccOptions" "--ld-flags=$ldOptions" ${compileTimeParameterList[*]} -- ${precompileParameterList[*]} || die "Sorry, could not build '$composite', $?" 

exit 0

