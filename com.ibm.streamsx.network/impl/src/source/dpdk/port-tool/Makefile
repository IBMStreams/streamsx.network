#
# Copyright (C) 2015, International Business Machines Corporation
# All Rights Reserved
#
SRCS-y := port-tool.c

# Can run 'make remote-build' to build on a remote machine - see else statement
ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

APP = port-tool

CFLAGS += -O3 -fPIC

ifeq ($(CONFIG_RTE_LIBRTE_MLX4_PMD),y)
LDLIBS += -libverbs
endif

include $(RTE_SDK)/mk/rte.extapp.mk

