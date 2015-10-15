#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )

domain=CapabilitiesDomain
instance=CapabilitiesInstance

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

#step "getting zookeeper connection string ..."
zkconnect=$( streamtool getzk --short )
[ -n "$zkconnect" ] || die "sorry, could not get zookeeper connection string"
export STREAMS_ZKCONNECT=$zkconnect

step "tearing down previous Streams 'capabilities' instance and domain ..."
streamtool stopinstance -i $instance -d $domain
streamtool rminstance -i $instance -d $domain --noprompt
streamtool stopdomain -d $domain
streamtool rmdomain -d $domain --noprompt

exit 0
