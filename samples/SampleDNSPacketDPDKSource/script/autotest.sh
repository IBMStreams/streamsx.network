#!/bin/bash

set -e

# verify that DPDK environment variables are set

[[ -z $RTE_SDK ]] && echo "RTE_SDK not set" && exit 1 
[[ -z $RTE_TARGET ]] && echo "RTE_TARGET not set" && exit 1 

# delete old DPDK directories and files, if necessary

[[ -d $RTE_SDK ]] && rm -rf $RTE_SDK
[[ -f dpdk-2.2.0.tar.xz ]] && rm dpdk-2.2.0.tar.xz

# get DPDK

wget http://fast.dpdk.org/rel/dpdk-2.2.0.tar.xz
tar -xvf dpdk-2.2.0.tar.xz

# configure DPDK for build below

cd $RTE_SDK
make config T=$RTE_TARGET
sed -i.bak s/CONFIG_RTE_BUILD_COMBINE_LIBS=n/CONFIG_RTE_BUILD_COMBINE_LIBS=y/ ./build/.config

# configure DPDK for Mellanox ethernet adapter, if necessary

[[ $( lspci | grep Mellanox ) ]] && sed -i.bak s/CONFIG_RTE_LIBRTE_MLX4_PMD=n/CONFIG_RTE_LIBRTE_MLX4_PMD=y/ ./build/.config

# build DPDK tools and utilities

EXTRA_CFLAGS=-fPIC make
EXTRA_CFLAGS=-fPIC make install T=$RTE_TARGET

# get Streams network toolkit

[[ -d $HOME/git/streamsx.network ]] && rm -rf $HOME/git/streamsx.network
cd $HOME/git
git clone git@github.com:ejpring/streamsx.network.git

# index the toolkit's operators and sample applications

cd $HOME/git/streamsx.network
ant

# build the toolkit's DPDK glue library
cd $HOME/git/streamsx.network/com.ibm.streamsx.network/impl/src/source/dpdk
make

# run the DPDK sample applications

cd $HOME/git/streamsx.network/samples/SampleDNSPacketDPDKSource/script
for test in ./liveDNSPacketDPDKSource*.sh ; do $test ; done

exit


