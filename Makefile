.PHONY: default build distclean doc all

default: build

doc: lazy.pdf

all: lazy.pdf build

build: beman-task stdexec libunifex
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD:STRING=23 -B build 
	cmake --build build

beman-task:
	git clone git@github.com:dietmarkuehl/beman-task.git

stdexec:
	git clone git@github.com:NVIDIA/stdexec.git

libunifex:
	git clone git@github.com:facebookexperimental/libunifex.git

wg21/Makefile:
	git clone https://github.com/mpark/wg21.git

distclean: clean
	$(RM) -r generated build
	$(RM) mkerr olderr

include wg21/Makefile

