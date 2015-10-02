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

set -v
sleep  1 ; xterm -hold -title "autotest SamplePacketFileSource" $here/SamplePacketFileSource/script/autotest.sh &
sleep 60 ; xterm -hold -title "autotest SamplePacketLiveSource" $here/SamplePacketLiveSource/script/autotest.sh &
sleep 60 ; xterm -hold -title "autotest SampleDHCPMessageParser" $here/SampleDCPMessageParser/script/autotest.sh &
sleep 60 ; xterm -hold -title "autotest SampleDNSMessageParser" $here/SampleDNSMessageParser/script/autotest.sh &
sleep 60 ; xterm -hold -title "autotest SampleNetflowMessageParser" $here/SampleNetflowMessageParser/script/autotest.sh &

exit 0
