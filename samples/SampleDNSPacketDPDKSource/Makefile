# /*********************************************************************
#  * Copyright (C) 2015, International Business Machines Corporation
#  * All Rights Reserved
#  *********************************************************************/

.PHONY: all standalone distributed clean 

SPLC = $(STREAMS_INSTALL)/bin/sc
SPL_CMD_ARGS =
SPL_DATA_DIR ?= ./data

# Example flags used with an Intel e1000 NIC.
###???SPLC_FLAGS ?=  $(OUTPUTDIR) -T -s -a -v -t ../../com.ibm.streamsx.network  --cxx-flags="-fPIC -m64 -pthread" --ld-flags="-Wl,-L -Wl,../../../com.ibm.streamsx.network/impl/lib/source -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=app.map -Wl,--cref"

# Example flags used with a Mellanox CX3 NIC.  
# Key libraries added is -libverbs.

# original flags ...
###???SPLC_FLAGS ?=  $(OUTPUTDIR) -T -s -a -v -t ../../com.ibm.streamsx.network  --cxx-flags="-fPIC -m64 -pthread" --ld-flags="-Wl,-L -Wl,../../../com.ibm.streamsx.network/impl/lib/source -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,-libverbs -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=app.map -Wl,--cref"

# -L 
# ../../../com.ibm.streamsx.network/impl/lib/source 
# --no-as-needed 
# -export-dynamic 
# --whole-archive 
# -ldpdk 
# -libverbs 
# --start-group 
# -lrt 
# -lm 
# -ldl 
# --end-group 
# --no-whole-archive 
# -Map=app.map 
# --cref

# without original compiler flags ...
###???SPLC_FLAGS ?=  $(OUTPUTDIR) -T -s -a -v -t ../../com.ibm.streamsx.network --cxx-flags="-g3" --ld-flags="-Wl,-L -Wl,../../../com.ibm.streamsx.network/impl/lib/source -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,-libverbs -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive -Wl,-Map=app.map -Wl,--cref"

# without original linker map and cross-reference output ...
###???SPLC_FLAGS ?=  $(OUTPUTDIR) -T -s -a -v -t ../../com.ibm.streamsx.network --cxx-flags="-g3" --ld-flags="-Wl,-L -Wl,../../../com.ibm.streamsx.network/impl/lib/source -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,-libverbs -Wl,--start-group -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--end-group -Wl,--no-whole-archive"

# without original linker --start-group and --end-group options ...
SPLC_FLAGS ?=  $(OUTPUTDIR) -T -s -a -v -t ../../com.ibm.streamsx.network --cxx-flags="-g3" --ld-flags="-Wl,-L -Wl,../../../com.ibm.streamsx.network/impl/lib/source -Wl,--no-as-needed -Wl,-export-dynamic -Wl,--whole-archive -Wl,-ldpdk -Wl,-libverbs -Wl,-lrt -Wl,-lm -Wl,-ldl -Wl,--no-whole-archive"







#SPL_MAIN_COMPOSITE = sample::LiveDNSPacketDPDKSourceBasic
SPL_MAIN_COMPOSITE = sample::LiveDNSPacketDPDKSourceFilter

all: standalone

standalone: 	 
	spl-make-toolkit -i ../../com.ibm.streamsx.network/
	$(SPLC) $(SPLC_FLAGS) -M $(SPL_MAIN_COMPOSITE)  --data-directory $(SPL_DATA_DIR)  $(SPL_CMD_ARGS) 

clean: 
	$(SPLC) $(SPLC_FLAGS) -C -M $(SPL_MAIN_COMPOSITE)

