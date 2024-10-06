CC = clang
DBG = lldb

export MACOSX_DEPLOYMENT_TARGET:=14

LSDL_FLAGS  :=$(shell sdl2-config --libs) -lSDL2_ttf
CSDL_FLAGS :=$(shell sdl2-config --cflags)

CLIBS=$(LSDL_FLAGS)
CFLAGS=-Wall -std=c99 -O0 $(CSDL_FLAGS)

.PHONY: build
build:
	if [ ! -d build ]; then mkdir build; fi
	clang $(CFLAGS) ./src/*.c -o build/game $(CLIBS)

.PHONY: check
check:
	@which $(CC) > /dev/null || echo "ERROR: $(CC) not found"
	@which $(DBG) > /dev/null || echo "ERROR: $(DBG) not found"
	@which sdl2-config > /dev/null || echo "ERROR: SDL2 not found"

.PHONY: run
run: build
	./build/game

.PHONY: debug
debug: build
	$(DBG) ./build/game

clean:
	@rm -rf build/ 
