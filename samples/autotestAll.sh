#!/bin/bash

## Copyright (C) 2011, 2015  International Business Machines Corporation
## All Rights Reserved

################### parameters used in this script ##############################

#set -o xtrace
#set -o pipefail

here=$( cd ${0%/*} ; pwd )

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

libpcapDirectory=$HOME/libpcap-1.8.1
[[ -d $libpcapDirectory ]] && export STREAMS_ADAPTERS_LIBPCAP_INCLUDEPATH=$libpcapDirectory
[[ -d $libpcapDirectory ]] && export STREAMS_ADAPTERS_LIBPCAP_LIBPATH=$libpcapDirectory

zookeeperDirectory=$HOME/zookeeper-3.4.9
[[ -d $zookeeperDirectory ]] && export STREAMS_ZKCONNECT=localhost:2181

scripts=( $( find $here -name "autotest.sh" ) )

step "executing all sample applications ..."
for script in ${scripts[*]} ; do 
    echo -e "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n$script\n"
    $script 
done
echo -e "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\nDone."

exit 0
