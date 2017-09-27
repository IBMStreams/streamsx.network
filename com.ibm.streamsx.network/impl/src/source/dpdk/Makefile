.PHONY: streams_source port-tool all clean

all: streams_source port-tool

streams_source:
	make -C streams_source O=../build

port-tool:
	make -C port-tool O=../build

clean:
	rm -rf build/

