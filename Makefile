.PHONY: all prepare build exec

all: prepare build

prepare:
	rm -rf build
	mkdir build

build:
	cmake -S . -B build
	@cmake --build build

exec:
	./build/bin/hack_asm --help -h