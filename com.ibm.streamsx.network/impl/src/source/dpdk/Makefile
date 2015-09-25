#
# Copyright (C) 2015, International Business Machines Corporation
# All Rights Reserved
#
SRCS-y := streams_source.c init.c rxtx.c 

# Can run 'make remote-build' to build on a remote machine - see else statement
ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# this works w/standalone LIB = libstreams_source.a
LIB = libstreams_source.a

CFLAGS += -O3 -fPIC

LDFLAGS += --whole-archive

include $(RTE_SDK)/mk/rte.lib.mk

