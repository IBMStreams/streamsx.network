#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )

unbundleDirectory=$projectDirectory/output/unbundle

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

# construct the pathname of the Streams bundle from the first argument

[ -n "$1" ] || die "sorry, no standalone application specified"
[ -f $1 ] || die "sorry, standalone application '$1' not found"
[ -x $1 ] || die "sorry, standalone application '$1' not executable"
directory=$( dirname $1 )
application=$( basename $1 )
bundle=$directory/../$application.sab
[ -f $bundle ] || die "sorry, bundle '$bundle' not found"
shift

# unbundle the Streams application bundle

step "unbundling standalone application '$application' ..."
spl-app-info $bundle --unbundle $unbundleDirectory/$application || die "sorry, could not unbundle '$bundle', $?"
standalone=$( find $unbundleDirectory/$application -name "standalone" )
[ -n "$standalone" ] || die "sorry, no standalone application found in bundle"
[ -x $standalone ] || die "sorry, standalone application '$standalone' not executable"

# execute the Streams standalone application within the debugger

step "debugging standalone application '$application' ..."
gdb --args $standalone $*

exit 0
