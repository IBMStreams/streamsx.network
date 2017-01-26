#!/bin/bash

set -e

# verify that DPDK environment variables are set

[[ -z $RTE_SDK ]] && echo "RTE_SDK environment variable not set" && exit 1 
[[ -z $RTE_TARGET ]] && echo "RTE_TARGET environment variable not set" && exit 1 


# look for supported hardware device

device=""
[[ $( lspci | grep "ConnectX-3 Pro" ) ]] && device=mellanox
[[ $( lspci | grep "I350 Gigabit Network Connection" ) ]] && device=e1000
[[ -z $device ]] && echo "no supported ethernet device found" && exit 1

# install additional Linux software packages, if necessary

[[ $( rpm -q glibc-devel ) ]] || sudo yum -y install glibc-devel
[[ $( rpm -q libibverbs-devel ) ]] || sudo yum -y install libibverbs-devel
[[ $( rpm -q "kernel-devel-$( uname -r )" ) ]] || sudo yum -y install "kernel-devel-$( uname -r )"

# create 'dpdk' user group, if necessary

[[ ! $( grep "dpdk:" /etc/group ) ]] && sudo groupadd dpdk && sudo usermod -a -G dpdk $USER
[[ ! $( grep "@dpdk" /etc/security/limits.conf ) ]] && sudo cat "@dpdk - memlock unlimited" >/etc/security/limits.conf

# allocate 'hugepages'

[[ $( sysctl vm.nr_hugepages | gawk '{print $3}' ) == 0 ]] && sudo sysctl -w vm.nr_hugepages=1000
[[ ! -d /dev/hugepages ]] && sudo mkdir /dev/hugepages
[[ ! $( mount | grep "/dev/hugepages" ) ]] && sudo mount -t hugetlbfs nodev /dev/hugepages -o gid=dpdk,mode=1770

# unpack a fresh copy of the DPDK distribution package

[[ -f dpdk-2.2.0.tar.xz ]] || wget http://fast.dpdk.org/rel/dpdk-2.2.0.tar.xz
[[ -d $RTE_SDK ]] && rm -rf $RTE_SDK
mkdir -p $RTE_SDK
tar -x -v -f dpdk-2.2.0.tar.xz -C $RTE_SDK --strip-components=1

# configure DPDK for Streams

cd $RTE_SDK
make config T=$RTE_TARGET
sed -i.bak s/CONFIG_RTE_BUILD_COMBINE_LIBS=./CONFIG_RTE_BUILD_COMBINE_LIBS=y/ ./build/.config
sed -i.bak s/CONFIG_RTE_MAX_MEMSEG=[0-9]*/CONFIG_RTE_MAX_MEMSEG=1024/ ./build/.config
[[ $( lspci | grep Mellanox ) ]] && sed -i.bak s/CONFIG_RTE_LIBRTE_MLX4_PMD=./CONFIG_RTE_LIBRTE_MLX4_PMD=y/ ./build/.config

# build DPDK tools and utilities

cd $RTE_SDK
EXTRA_CFLAGS=-fPIC make
EXTRA_CFLAGS=-fPIC make install T=$RTE_TARGET

# re-build the Streams network toolkit, now including DPDK glue library

cd $HOME/git/streamsx.network
ant

exit


