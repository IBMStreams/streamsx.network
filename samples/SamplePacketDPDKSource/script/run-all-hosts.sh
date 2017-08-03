#!/bin/bash

## Copyright (C) 2017  International Business Machines Corporation
## All Rights Reserved

#set -o xtrace
#set -o pipefail

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

here=$( cd ${0%/*} ; pwd )

#hosts=( c0321 c0323 c0325 c0327 c1221 c1224 h0711 h0701 )
hosts=( c0327 c1221 h0711 )
for host in ${hosts[@]} ; do 
    echo
    echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ $host ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
    #ssh $host rpm -qa | grep -i libmlx4
    #ssh $host ibv_devinfo 
    #ssh $host cat /etc/modprobe.d/mlnx.conf

    ssh $host $here/runLivePacketDPDKSourceBasic.sh 2>&1 | tee $here/../log/runLivePacketDPDKSourceBasic.$host.log
done

exit 0


