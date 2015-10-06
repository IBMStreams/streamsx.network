#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

domain=ConsistentDomain
instance=ConsistentInstance

streamtool=$STREAMS_INSTALL/bin/streamtool
zkconnect=$( $streamtool getzk --short )

export STREAMS_ZKCONNECT=$zkconnect

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

step "tearing down previous Streams 'consistent' instance and domain ..."
$streamtool stopinstance -i $instance -d $domain
$streamtool rminstance -i $instance -d $domain --noprompt
$streamtool stopdomain -d $domain
$streamtool rmdomain -d $domain --noprompt

exit 0
