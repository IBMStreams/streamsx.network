#!/bin/bash

## Copyright (C) 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
set -o pipefail

here=$( cd ${0%/*} ; pwd )
projectDirectory=$( cd $here/.. ; pwd )

logDirectory=$projectDirectory/log

################################################################################

[ -n "$1" ] || die "sorry, no argument specified"
[ -x $1 ] || die "sorry, '$1' is not executable"
[ -d $logDirectory ] || mkdir -p $logDirectory || die "sorry, could not create directory '$logDirectory', $?"

name=$( basename $1 .sh )
$@ 2>&1 | tee $logDirectory/$name.log
exit $?
