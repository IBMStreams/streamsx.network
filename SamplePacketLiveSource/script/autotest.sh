#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )
dataDirectory=$( cd $here/../data ; pwd )
buildDirectory=$here/../output 
logDirectory=$here/../log

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

rm -rf $buildDirectory || die "sorry, could not clear directory '$buildDirectory', $!"
rm -rf $logDirectory || die "sorry, could not clear directory '$logDirectory', $!"
rm -f $dataDirectory/debug.*.out || die "sorry, could not clear directory '$dataDirectory', $!"

mkdir -p $logDirectory || die "sorry, could not create directory '$logDirectory', $!"

scriptCount=0
successCount=0
failureCount=0

for script in $here/test*.sh ; do 
	scriptname=$( basename $script .sh )
	echo $scriptname ...
	logname=$logDirectory/$scriptname.log
	$script 1>$logname 2>&1
	exitcode=$?
	[ $exitcode -eq 0 ] && echo "... OK" && (( successCount++ )) 
	[ $exitcode -ne 0 ] && echo "... failed" && mv $logname $logname.failed && (( failureCount++ )) 
	(( scriptCount++ ))
done

echo -e "\n$successCount of $scriptCount scripts succeeded"

[ $failureCount -ne 0 ] && echo -e "\n$failureCount scripts failed:" && ( cd $logDirectory ; ls -1 *.failed )

exit 0