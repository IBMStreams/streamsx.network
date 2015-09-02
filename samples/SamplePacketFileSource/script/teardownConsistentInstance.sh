#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

instance=ConsistentInstance

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

step "stopping the Streams instance '$instance' ..."
streamtool stopinstance -i $instance --force

step "removing the Streams instance '$instance' ..."
streamtool rminstance -i $instance --noprompt

exit 0
