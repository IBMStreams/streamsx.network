#!/bin/bash

set -e 

# verify that DPDK environment variables are set

[[ -z $RTE_SDK ]] && echo "RTE_SDK not set" && exit 1 
[[ -z $RTE_TARGET ]] && echo "RTE_TARGET not set" && exit 1 
[[ -z $DPDK_INTERFACE ]] && echo "DPDK_INTERFACE environment variable not set" && exit 1 
[[ -z $DPDK_DEVICE ]] && echo "DPDK_DEVICE environment variable not set" && exit 1 

# Add this line to '/etc/modprobe.d/mlnx.conf' ... 'options ib_uverbs disable_raw_qp_enforcement=1' ...

# load Mellanox device driver into kernel

###???sudo modprobe mlx4_en

# verify ...

###???/etc/init.d/openibd restart
###???sudo ibstat
###???sudo /etc/init.d/openibd status

# stop Linux device driver

###???sudo ip link set dev $DPDK_INTERFACE down
###???ip link show dev $DPDK_INTERFACE

# bind ethernet device to DPDK device driver

###???sudo $RTE_SDK/tools/dpdk_nic_bind.py --bind=igb_uio $DPDK_DEVICE
###???sudo $RTE_SDK/tools/dpdk_nic_bind.py --status

exit 0
