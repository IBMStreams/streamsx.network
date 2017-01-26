#!/bin/bash

set -e 

# verify that DPDK environment variables are set

[[ -z $RTE_SDK ]] && echo "RTE_SDK not set" && exit 1 
[[ -z $RTE_TARGET ]] && echo "RTE_TARGET not set" && exit 1 
[[ -z $DPDK_INTERFACE ]] && echo "DPDK_INTERFACE environment variable not set" && exit 1 
[[ -z $DPDK_DEVICE ]] && echo "DPDK_DEVICE environment variable not set" && exit 1 

# load 'ui' and DPDK 'igb_uio' device drivers into kernel

sudo modprobe uio
sudo insmod $RTE_SDK/build/kmod/igb_uio.ko || true

# stop Linux device driver

sudo ip link set dev $DPDK_INTERFACE down
ip link show dev $DPDK_INTERFACE

# bind ethernet device to DPDK device driver

sudo $RTE_SDK/tools/dpdk_nic_bind.py --bind=igb_uio $DPDK_DEVICE
sudo $RTE_SDK/tools/dpdk_nic_bind.py --status

# grant Linux 'dpdk' group read/write access to DPDK device driver files 

sudo chown root:dpdk /dev/uio0 /sys/class/uio/uio0/device/config /sys/class/uio/uio0/device/resource*
sudo chmod 664 /dev/uio0 /sys/class/uio/uio0/device/config /sys/class/uio/uio0/device/resource*

exit 0
