#!/bin/bash

set -e

# verify that DPDK environment variables are set

[[ -z $RTE_SDK ]] && echo "RTE_SDK environment variable not set" && exit 1 
[[ -z $RTE_TARGET ]] && echo "RTE_TARGET environment variable not set" && exit 1 

# unpack a fresh copy of the DPDK distribution package

[[ -f dpdk-2.2.0.tar.xz ]] || wget http://fast.dpdk.org/rel/dpdk-2.2.0.tar.xz
[[ -d $RTE_SDK ]] && rm -rf $RTE_SDK
mkdir -p $RTE_SDK
tar -x -v -f dpdk-2.2.0.tar.xz -C $RTE_SDK --strip-components=1

# configure DPDK for Streams

cd $RTE_SDK
make config T=$RTE_TARGET
sed -i.bak s/CONFIG_RTE_BUILD_COMBINE_LIBS=./CONFIG_RTE_BUILD_COMBINE_LIBS=y/ ./build/.config
sed -i.bak s/CONFIG_RTE_MAX_MEMSEG=[0-9]+/CONFIG_RTE_MAX_MEMSEG=1024/ ./build/.config
[[ $( lspci | grep Mellanox ) ]] && sed -i.bak s/CONFIG_RTE_LIBRTE_MLX4_PMD=./CONFIG_RTE_LIBRTE_MLX4_PMD=y/ ./build/.config

# build DPDK tools and utilities

cd $RTE_SDK
EXTRA_CFLAGS=-fPIC make
EXTRA_CFLAGS=-fPIC make install T=$RTE_TARGET

# re-build the Streams network toolkit, now including DPDK glue library

cd $HOME/git/streamsx.network
ant

exit


