#!/bin/bash
#set -o xtrace
#set -o pipefail

################### parameters used in this script ##############################

namespace=sample
composite=TestPacketFileSourceFilters

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )
workspaceDirectory=$( cd $here/../.. ; pwd )
buildDirectory=$projectDirectory/output/build/$composite
dataDirectory=$projectDirectory/data

coreCount=$( cat /proc/cpuinfo | grep processor | wc -l )

toolkitList=(
$workspaceDirectory/com.ibm.streamsx.network
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
--data-directory=$dataDirectory
--num-make-threads=$coreCount
)

compileTimeParameterList=(
)

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

cd $projectDirectory || die "Sorry, could not change to $projectDirectory, $?"

[ ! -d $outputDirectory ] || rm -rf $outputDirectory || die "Sorry, could not delete old '$outputDirectory', $?"

step "configuration for composite '$namespace::$composite' ..."
( IFS=$'\n' ; echo -e "\nStreams toolkits:\n${toolkitList[*]}" )
( IFS=$'\n' ; echo -e "\nStreams compiler options:\n${compilerOptionsList[*]}" )
( IFS=$'\n' ; echo -e "\n$composite compile-time parameters:\n${compileTimeParameterList[*]}" )

step "building composite '$namespace::$composite' ..."
sc ${compilerOptionsList[*]} -- ${compileTimeParameterList[*]} || die "Sorry, could not build '$namespace::$composite', $?" 

exit 0


