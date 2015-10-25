#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )
root=$( cd ${0%/*}/.. ; pwd )


################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

toolkits=( $( find $root -name "info.xml" -exec dirname {} \; ) )

step "remaking tookits ..."
for toolkit in ${toolkits[*]} ; do 
    echo "$toolkit ..."
    spl-make-toolkit --verbose-model-errors --directory $toolkit || die "sorry, could not remake toolkit '$toolkit'"
done

step "generating documentation ..."
spl-make-doc \
--check-tags \
--include-all \
--output-directory $root/doc/spldoc \
--toolkit-path $( IFS=":" ; echo "${toolkits[*]}" ) \
|| die "sorry, could not generate documentation"

exit 0
