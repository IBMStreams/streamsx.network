# /*********************************************************************
#  * Copyright (C) 2015, International Business Machines Corporation
#  * All Rights Reserved
#  *********************************************************************/

.PHONY: all standalone distributed clean 

SPLC_FLAGS ?=  $(OUTPUTDIR) -a -v --static-link -t /home/engebret/apps/network/apps.dpdk/com.ibm.streamsx.network  --cxx-flags="-fPIC -m64 -pthread -march=native" --ld-flags="-Wl,-rpath,/home/engebret/apps/InfoSphere_Streams/4.0.1.0/lib -Wl,-rpath,/home/engebret/apps/InfoSphere_Streams/4.0.1.0/impl/lib -Wl,-rpath,/home/engebret/apps/InfoSphere_Streams/4.0.1.0/ext/lib -Wl,-rpath,/home/engebret/apps/InfoSphere_Streams/4.0.1.0/system/impl/lib -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-lintel_dpdk -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=app.map -Wl,--cref"

SPLC = $(STREAMS_INSTALL)/bin/sc
SPL_CMD_ARGS =
SPL_MAIN_COMPOSITE = TestPacketDPDKSource
SPL_DATA_DIR ?= ./data

all: distributed

distributed: 	 
	spl-make-toolkit -i ../com.ibm.streamsx.network/
	$(SPLC) $(SPLC_FLAGS) -M $(SPL_MAIN_COMPOSITE)  --data-directory $(SPL_DATA_DIR)  $(SPL_CMD_ARGS) 

clean: 
	$(SPLC) $(SPLC_FLAGS) -C -M $(SPL_MAIN_COMPOSITE)

