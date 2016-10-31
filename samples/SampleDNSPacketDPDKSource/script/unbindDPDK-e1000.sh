#!/bin/bash

set -e 

# verify that DPDK environment variables are set

[[ -z $RTE_SDK ]] && echo "RTE_SDK environment variable not set" && exit 1 
[[ -z $RTE_TARGET ]] && echo "RTE_TARGET environment variable not set" && exit 1 
[[ -z $DPDK_INTERFACE ]] && echo "DPDK_INTERFACE environment variable not set" && exit 1 
[[ -z $DPDK_DEVICE ]] && echo "DPDK_DEVICE environment variable not set" && exit 1 

# bind ethernet device to Linux device driver

sudo $RTE_SDK/tools/dpdk_nic_bind.py --bind=igb $DPDK_DEVICE
sudo $RTE_SDK/tools/dpdk_nic_bind.py --status

# start Linux device driver

sudo ip link set dev $DPDK_INTERFACE up
ip link show dev $DPDK_INTERFACE

exit 0
