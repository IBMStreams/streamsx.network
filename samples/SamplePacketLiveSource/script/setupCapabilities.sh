#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )

administrator=streamsadmin
domain=CapabilitiesDomain
instance=CapabilitiesInstance

streamtool=$STREAMS_INSTALL/bin/streamtool
zkconnect=$( $streamtool getzk --short )
hostname=$( hostname )

export STREAMS_ZKCONNECT=$zkconnect
 
################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

[ -d $checkpointDirectory ] || mkdir -p $checkpointDirectory || die "sorry, could not create directory '$checkpointDirectory', $?"

step "checking zookeeper at '$zkconnect' ..."
$streamtool getzkstate || die "sorry, could not connect to zookeeper at '$zkconnect', $?"

$here/teardownCapabilities.sh

step "making Streams domain '$domain' ..."
$streamtool mkdomain -d $domain || die "sorry, could not make Streams domain '$domain', $?"
$streamtool genkey -d $domain || die "sorry, could not generate keys for Streams domain '$domain', $?"
sudo STREAMS_INSTALL=$STREAMS_INSTALL $streamtool registerdomainhost -d $domain --application --management --zkconnect $zkconnect || die "sorry, could not register Streams domain '$domain', $?"

step "starting Streams domain '$domain' ..."
$streamtool startdomain -d $domain || die "sorry, could not start Streams domain '$domain', $?"

step "adding users to Streams domain '$domain' ..."
$streamtool adduserdomainrole -d $domain DomainAdministrator $administrator || die "sorry, could not add administrator to Streams domain '$domain', $?"
$streamtool adduserdomainrole -d $domain DomainUser $USER || die "sorry, could not add user to Streams domain '$domain', $?"
$streamtool lsdomainrole -d CapabilitiesDomain

step "getting domain service URL for domain '$domain' ..."
$streamtool getjmxconnect -d $domain || die "sorry, could not domain service URL for Streams domain '$domain', $?"

step "getting Streams console URL for domain '$domain' ..."
$streamtool geturl -d $domain || die "sorry, could not get Streams console URL for Streams domain '$domain', $?"

step "making Streams instance '$instance' ..."
$streamtool mkinstance -i $instance -d $domain \
--hosts "$hostname" \
--property instanceTrace.defaultLevel=info \
--property instanceTrace.maximumFileCount=10 \
--property instanceTrace.maximumFileSize=1000000 \
--property instance.canSetPeOSCapabilities=true \
--property instance.runAsUser=$USER \
|| die "Sorry, could not create Streams instance '$instance', $?"

step "starting Streams instance '$instance' ..."
$streamtool startinstance -i $instance -d $domain || die "Sorry, could not start Streams instance '$instance', $?" 

exit 0

