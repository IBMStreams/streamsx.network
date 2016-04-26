#!/bin/bash

## Copyright (C) 2016  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

step "generating individual SPLDOCs for each Streams directory in toolkit ..."
directories=( $( find $here -name "info.xml" -exec dirname {} \; ) )
for directory in ${directories[*]} ; do 
    spl-make-doc --directory $directory --check-tags --include-all || die "sorry, could not make $directory/doc"
done

step "generating composite SPLDOC for entire toolkit ..."
toolkits=$( IFS=":" ; echo "${directories[*]}" )
spl-make-doc --output-directory $here/doc/spldoc --toolkit-path $toolkits --check-tags --include-all || die "sorry, could not make $here/doc"

exit 0
