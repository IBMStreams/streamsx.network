#!/bin/bash

set -e 

interface=ens6f0

# verify that DPDK environment variables are set

[[ -z $RTE_SDK ]] && echo "RTE_SDK not set" && exit 1 
[[ -z $RTE_TARGET ]] && echo "RTE_TARGET not set" && exit 1 

# switch this shell to Linux group 'dpdk'

newgrp dpdk

# load 'ui' and DPDK 'igb_uio' device drivers into kernel

sudo modprobe uio
sudo insmod $RTE_SDK/build/kmod/igb_uio.ko || true

# grant Linux 'dpdk' group read/write access to DPDK device driver files 

sudo chown root:dpdk /dev/uio0
sudo chown root:dpdk /sys/class/uio/uio0/device/config
sudo chown root:dpdk /sys/class/uio/uio0/device/resource*

sudo chmod 660 /dev/uio0
sudo cdmod 660 /sys/class/uio/uio0/device/config
sudo chmod 660 /sys/class/uio/uio0/device/resource*

# stop Linux device driver for the network interface

sudo ifconfig $interface down
ifconfig $interface

# bind network interface to DPDK device driver

$RTE_SDK/tools/dpdk_nic_bind.py --bind=igb_uio $interface
$RTE_SDK/tools/dpdk_nic_bind.py --status


