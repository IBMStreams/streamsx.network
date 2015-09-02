#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

projectDirectory=$( cd ${0%/*} ; pwd )
checkpointDirectory=$HOME/checkpoint

instance=ConsistentInstance
hostname=$( hostname )

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

$projectDirectory/teardownConsistentInstance.sh

step "creating Streams instance '$instance' ..."

[ -d $checkpointDirectory ] || mkdir -p $checkpointDirectory || die "sorry, could not create directory '$checkpointDirectory', $!"

streamtool mkinstance -i $instance \
--hosts "$hostname" \
--property instanceTrace.defaultLevel=info \
--property instanceTrace.maximumFileCount=10 \
--property instanceTrace.maximumFileSize=1000000 \
--property instance.checkpointRepository=fileSystem \
--property instance.checkpointRepositoryConfiguration="{ \"Dir\" : \"$checkpointDirectory\" }" \
|| die "Sorry, could not create Streams instance, $?"

step "starting Streams instance '$instance' ..."
streamtool startinstance -i $instance || die "Sorry, could not start Streams runtime, $?" 

exit 0
