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

# check hostname

step "check host name ..."
hostname

# check environment variables

step "check Linux environment variables ..."
export | grep -E "RTE_|DPDK_"
[[ -n $RTE_SDK ]] || die "RTE_SDK environment variable not set" 
[[ -n $RTE_TARGET ]] || die "RTE_TARGET environment variable not set"
[[ -n $DPDK_DEVICE ]] || die "DPDK_DEVICE environment variable not set" 
[[ -n $DPDK_INTERFACE ]] || die "DPDK_INTERFACE environment variable not set" 

# check Streams version

step "check Streams version ..."
[[ $( streamtool version | grep -E "^Version" ) == "Version=4.1.1.4" ]] || die "Streams version is not 4.1.1.4"
streamtool version

# install additional Linux software packages, if necessary

step "install additional Linux packages, if necessary ..."
[[ $( rpm -q pciutils ) ]] || sudo yum -y install pciutils || die "sorry, could not install Linux package 'pciutils'"
[[ $( rpm -q glibc-devel ) ]] || sudo yum -y install glibc-devel || die "sorry, could not install Linux package 'glibc-devel'"
[[ $( rpm -q libibverbs-devel ) ]] || sudo yum -y install libibverbs-devel || die "sorry, could not install Linux package 'libibverbs-devel'"
[[ $( rpm -q "kernel-devel-$( uname -r )" ) ]] || sudo yum -y install "kernel-devel-$( uname -r )" || die "sorry, could not install Linux package 'kernel-devel'"

# check physical processor information

step "check processor information ..."
lscpu
#processorModel=$( grep 'model name' /proc/cpuinfo | head -n 1 | cut -d ':' -f 2  )
#processorPhysicalCores=$( grep 'cpu cores' /proc/cpuinfo | head -n 1 | cut -d ':' -f 2 )
#processorLogicalCores=$( grep 'cpu cores' /proc/cpuinfo | wc -l )

# check for supported network adapter

step "check PCI buses for network adapter ..."
/usr/sbin/lspci | grep -i -E "connectx|i350 gigabit"
[[ $( /usr/sbin/lspci | grep -i -E 'connectx|i350 gigabit' ) ]] || die "sorry, no supported network interface found"

step "check interfaces for network adapter ..."
DPDK_INTERFACES=( $( ls -l /sys/class/net | grep $DPDK_DEVICE | gawk '{print $9}' ) )
echo ${DPDK_INTERFACES[*]}
[[ -n ${DPDK_INTERFACES[0]} ]] || die "sorry, DPDK_INTERFACES[0] environment variable not set"
[[ -n ${DPDK_INTERFACES[1]} ]] || die "sorry, DPDK_INTERFACES[1] environment variable not set"

# print NUMA node information

step "check processor NUMA nodes for network adapter ..."
for interface in ${DPDK_INTERFACES[*]} ; do
    echo "network interface $interface on NUMA node" $( cat /sys/class/net/$interface/device/numa_node )
done

# disable flow control 

#for interface in ${DPDK_INTERFACES[@]} ; do
#    step "disabling flow control for network interface $interface ..."
#    sudo /usr/sbin/ethtool -A $interface autoneg off rx off tx off | true
#    /usr/sbin/ethtool -a $interface
#done

# create 'dpdk' user group and add current user to it

step "create DPDK user group, if necessary ..."
[[ $( grep -E "^dpdk:" /etc/group ) ]] || sudo groupadd dpdk  || die "sorry, could not add user group 'dpdk'"
[[ $( grep -E "^dpdk:.*$USER" /etc/group ) ]] || sudo usermod -a -G dpdk $USER || die "sorry, could not add user $USER to group 'dpdk'"
[[ $( grep -E "@dpdk " /etc/security/limits.conf ) ]] || sudo cat "@dpdk - memlock unlimited" >/etc/security/limits.conf || die "sorry, could not update /etc/security/limits.conf"

# set ownership and permissions for DPDK device files

step "set ownership and permissions for DPDK device files"
for interface in ${DPDK_INTERFACES[@]} ; do
    sudo chown root:dpdk /sys/class/infiniband/mlx4_0/device/net/$interface/flags
    sudo chmod 666 /sys/class/infiniband/mlx4_0/device/net/$interface/flags
done

# create mount points for 2MB and 1GB 'hugepages'

step "check mount points for 'hugepages' ..."

[[ -d /dev/hugepages-2MB ]] || sudo mkdir /dev/hugepages-2MB || die "sorry, could not create directory /dev/hugepages-2MB"
[[ -d /dev/hugepages-1GB ]] || sudo mkdir /dev/hugepages-1GB || die "sorry, could not create directory /dev/hugepages-1GB"

[[ $( mount | grep '/dev/hugepages-2MB' ) ]] || sudo mount -t hugetlbfs nodev /dev/hugepages-2MB -o pagesize=2MB || echo "sorry, could not mount /dev/hugepages-2MB"
[[ $( mount | grep '/dev/hugepages-1GB' ) ]] || sudo mount -t hugetlbfs nodev /dev/hugepages-1GB -o pagesize=1GB || echo "sorry, could not mount /dev/hugepages-1GB"

sudo chmod 770 /dev/hugepages-2MB
sudo chmod 770 /dev/hugepages-1GB

sudo chown root:dpdk /dev/hugepages-2MB
sudo chown root:dpdk /dev/hugepages-1GB

# allocate 'hugepages'

step "allocate 2MB and 1GB 'hugepages' ..."
sudo rm -f /dev/hugepages-2MB/rtemap_*
sudo rm -f /dev/hugepages-1GB/rtemap_*
for node in /sys/devices/system/node/node* ; do
    sudo /usr/bin/bash -c "echo 900 > $node/hugepages/hugepages-2048kB/nr_hugepages"
    sudo /usr/bin/bash -c "echo   1 > $node/hugepages/hugepages-1048576kB/nr_hugepages"
done

step "check 'hugepages' allocations ..."
grep -i hugepages /proc/meminfo
echo
grep -i -h hugepages /sys/devices/system/node/node*/meminfo

# get DPDK source code 

step "get DPDK source code, if necessary ..."
[[ -d $RTE_SDK ]] || mkdir -p $RTE_SDK || die "sorry, could not create directory $RTE_SDK"
[[ -f dpdk-17.11.3.tar.xz ]] || wget http://fast.dpdk.org/rel/dpdk-17.11.3.tar.xz || die "sorry, could not get DPDK source code from http://fast.dpdk.org/rel/dpdk-17.11.3.tar.xz"
[[ -f $RTE_SDK/Makefile ]] || tar -x -v -f dpdk-17.11.3.tar.xz -C $RTE_SDK --strip-components=1 || die "sorry, could not unpack DPDK source code"

# patch old DPDK 2.2.0 source code for RHEL/CentOS 7.3

###???cd $RTE_SDK
###???wget http://dpdk.org/dev/patchwork/patch/15911/raw/ -O dpdk-dev-v2-kni-support-RHEL-7.3.patch
###???patch -p1 < dpdk-dev-v2-kni-support-RHEL-7.3.patch

# build DPDK with support for Mellanox devices

step "configure DPDK for $RTE_TARGET, if necessary ..."
[[ -d $RTE_SDK ]] || mkdir -p $RTE_SDK || die "sorry, could not create directory $RTE_SDK"
cd $RTE_SDK || die "sorry, could not switch to directory $RTE_SDK"
[[ -d $RTE_SDK/$RTE_TARGET ]] || make config T=$RTE_TARGET O=$RTE_TARGET || die "sorry, could not configure DPDK for $RTE_TARGET"

step "configure DPDK for Mellanox adapter ..."
cd $RTE_SDK/$RTE_TARGET || die "sorry, could not switch to directory $RTE_SDK/$RTE_TARGET"
###???sed -i.bak s/CONFIG_RTE_MAX_MEMSEG=[0-9]*/CONFIG_RTE_MAX_MEMSEG=1024/ ./.config
sed -i.bak s/CONFIG_RTE_BUILD_SHARED_LIB=./CONFIG_RTE_BUILD_SHARED_LIB=n/ ./.config
sed -i.bak s/CONFIG_RTE_LIBRTE_MLX4_PMD=./CONFIG_RTE_LIBRTE_MLX4_PMD=y/ ./.config

step "compile DPDK, if necessary ..."
[[ -f $RTE_SDK/$RTE_TARGET/lib/libdpdk.a ]] || EXTRA_CFLAGS=-fPIC make || die "sorry, could not compile DPDK"

# build the Streams network toolkit, including DPDK glue library

step "get Streams network toolkit, if necessary ..."
[[ -f $HOME/git ]] || mkdir -p $HOME/git || die "sorry, could not create directory $HOME/git"
cd $HOME/git || die "sorry, could not switch to directory $HOME/git"
[[ -d $HOME/git/streamsx.network ]] || git clone git@github.com:IBMStreams/streamsx.network.git
cd $HOME/git/streamsx.network die "sorry, could not create directory $HOME/git/streams.network"

step "index Streams network toolkit ..."
ant toolkit || die "sorry, could not index $HOME/git/streams.network"

step "compile Streams DPDK 'glue' libary ..."
EXTRA_CFLAGS=-fPIC ant dpdkglue || die "sorry, could not compile Streams DPDK 'glue' libary ..."

exit 0
