.PHONY: all prepare build exec test

all: prepare build

prepare:
	rm -rf build
	mkdir build

build:
	cmake -S . -B build
	@cmake --build build

exec:
	./build/bin/hack_asm --help -h

test:
	cmake -S . -B build -DBUILD_TESTING=ON
	cmake --build build
	ctest --test-dir build --output-on-failure