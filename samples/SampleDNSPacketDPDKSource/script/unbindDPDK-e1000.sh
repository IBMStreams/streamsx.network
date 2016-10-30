#!/bin/bash

set -e 
set -o xtrace

interface=ens6f0

# verify that DPDK environment variables are set

[[ -z $RTE_SDK ]] && echo "RTE_SDK not set" && exit 1 
[[ -z $RTE_TARGET ]] && echo "RTE_TARGET not set" && exit 1 

# switch this shell to Linux group 'dpdk'

newgrp dpdk

# bind network interface to Linux device driver

$RTE_SDK/tools/dpdk-nic_bind.py --bind=igb $interface
$RTE_SDK/tools/dpdk_nic_bind.py --status

# start Linux device driver for network interface

sudo ifconfig $interface up
ifconfig $interface

exit 0
