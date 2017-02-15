#!/bin/bash

## Copyright (C) 2016  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )

export STREAMS_DOMAIN_ID=CapabilitiesDomain
export STREAMS_INSTANCE_ID=CapabilitiesInstance

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

[[ -n "$STREAMS_ZKCONNECT" ]] && step "using zookeeper at $STREAMS_ZKCONNECT ..."
[[ -z "$STREAMS_ZKCONNECT" ]] && step "using embedded zookeeper ..." && zookeeper="--embeddedzk" 

step "stopping and removing Streams instance $STREAMS_INSTANCE_ID ..."
streamtool stopinstance -i $STREAMS_INSTANCE_ID -d $STREAMS_DOMAIN_ID --force $zookeeper
streamtool rminstance -i $STREAMS_INSTANCE_ID -d $STREAMS_DOMAIN_ID --force --noprompt $zookeeper

step "stopping and removing Streams domain $STREAMS_DOMAIN_ID ..."
streamtool stopdomain -d $STREAMS_DOMAIN_ID --force $zookeeper
streamtool rmdomain -d $STREAMS_DOMAIN_ID --force --noprompt $zookeeper

exit 0
