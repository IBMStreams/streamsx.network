#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

step "updating toolkit metadata files ..."
directories=( $( find $here -name "info.xml" -exec dirname {} \; ) )
for directory in ${directories[*]} ; do 
    echo "$directory ..."
    spl-make-toolkit --verbose-model-errors --directory $directory || die "sorry, could not update metadata files in directory '$directory'"
done

step "generating documentation ..."
samples=( $( find $here/samples -name "info.xml" -exec dirname {} \; ) )
spl-make-doc \
--check-tags \
--include-all \
--output-directory $here/doc/spldoc \
--toolkit-path $( IFS=":" ; echo "${directories[*]}" ) \
|| die "sorry, could not generate documentation"

exit 0
