.PHONY: all distributed clean 

TOOLKIT_NAME=com.ibm.streamsx.network
STREAMS_NETWORK_TOOLKIT ?= $(shell ([ -e "../../../$(TOOLKIT_NAME)/toolkit.xml" ] && echo ../../../$(TOOLKIT_NAME)) ||\
			           echo $(STREAMS_INSTALL)/toolkits/$(TOOLKIT_NAME))

ifdef STREAMS_BUILDSERVICE
  STREAMS_NETWORK_TOOLKIT = ../toolkits:$(STREAMS_INSTALL)/toolkits
endif
THREAD_OPTION=--num-make-threads 4
ifdef STREAMS_CPU_LIMIT
    THREAD_OPTION=--num-make-threads ${STREAMS_CPU_LIMIT}
endif

SPLC_FLAGS = -a $(THREAD_OPTION) -t $(STREAMS_NETWORK_TOOLKIT)
SPL_CMD_ARGS ?= 
SPLC = $(STREAMS_INSTALL)/bin/sc
SPL_MAIN_COMPOSITE = sample::NetflowMessageParserSample

all: distributed

distributed:
	$(SPLC) $(SPLC_FLAGS) -M $(SPL_MAIN_COMPOSITE) $(SPL_CMD_ARGS)

clean: 
	$(SPLC) $(SPLC_FLAGS) -C -M $(SPL_MAIN_COMPOSITE)
	rm -rf output

