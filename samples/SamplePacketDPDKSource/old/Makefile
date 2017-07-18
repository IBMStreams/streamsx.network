# /*********************************************************************
#  * Copyright (C) 2015, International Business Machines Corporation
#  * All Rights Reserved
#  *********************************************************************/

.PHONY: all standalone distributed clean 

# Example flags used with an Intel e1000 NIC.
###???SPLC_FLAGS ?=  $(OUTPUTDIR) -T -s -a -v -t ../../com.ibm.streamsx.network  --cxx-flags="-fPIC -m64 -pthread" --ld-flags="-Wl,-L -Wl,../../../com.ibm.streamsx.network/impl/lib/source -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=app.map -Wl,--cref"

# Example flags used with a Mellanox CX3 NIC.  
# Key libraries added is -libverbs.
#SPLC_FLAGS ?=  $(OUTPUTDIR) -T -s -a -v -t ../../com.ibm.streamsx.network  --cxx-flags="-fPIC -m64 -pthread" --ld-flags="-Wl,-L -Wl,../../../com.ibm.streamsx.network/impl/lib/source -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,-libverbs -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=app.map -Wl,--cref"
SPLC_FLAGS ?=  $(OUTPUTDIR) -T -s -a -v -t ../../com.ibm.streamsx.network  --cxx-flags="-fPIC -m64 -pthread" --ld-flags="-Wl,-L -Wl,$(RTE_SDK)/build/lib -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,-libverbs -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=app.map -Wl,--cref"

SPLC = $(STREAMS_INSTALL)/bin/sc
SPL_CMD_ARGS =
SPL_MAIN_COMPOSITE = TestPacketDPDKSource
SPL_DATA_DIR ?= ./data

all: distributed

distributed: 	 
	spl-make-toolkit -i ../../com.ibm.streamsx.network/
	$(SPLC) $(SPLC_FLAGS) -M $(SPL_MAIN_COMPOSITE)  --data-directory $(SPL_DATA_DIR)  $(SPL_CMD_ARGS) 

clean: 
	$(SPLC) $(SPLC_FLAGS) -C -M $(SPL_MAIN_COMPOSITE)

