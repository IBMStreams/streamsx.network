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

STREAMS_ZKCONNECT=localhost:21810
STREAMS_DOMAIN_ID=CapabilitiesDomain
STREAMS_INSTANCE_ID=CapabilitiesInstance

step "using zookeeper at $STREAMS_ZKCONNECT ..."

step "stopping and removing Streams instance $STREAMS_INSTANCE_ID ..."
streamtool stopinstance -i $STREAMS_INSTANCE_ID -d $STREAMS_DOMAIN_ID --force --zkconnect $STREAMS_ZKCONNECT
streamtool rminstance -i $STREAMS_INSTANCE_ID -d $STREAMS_DOMAIN_ID --force --noprompt --zkconnect $STREAMS_ZKCONNECT

step "stopping and removing Streams domain $STREAMS_DOMAIN_ID ..."
streamtool stopdomain -d $STREAMS_DOMAIN_ID --force --zkconnect $STREAMS_ZKCONNECT
streamtool rmdomain -d $STREAMS_DOMAIN_ID --force --noprompt --zkconnect $STREAMS_ZKCONNECT

step "stopping zookeeper ..."
/opt/ibm/InfoSphere_Streams/4.1.1.1/system/impl/bin/streams-zk.sh stop

exit 0
