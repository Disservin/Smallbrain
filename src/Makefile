_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
EVALFILE   = $(_ROOT)/default.nnue
CC        := g++
TARGET    := smallbrain

# name and version
NAME      := smallbrain

ifeq ($(OS), Windows_NT)
	uname_S := Windows
	FLAGS  = -pthread -lstdc++ -static
	SUFFIX  := .exe
else
	FLAGS  = -lpthread -lstdc++
	SUFFIX  :=
	uname_S := $(shell uname -s)
endif

ifeq ($(uname_S), Darwin)
	NATIVE = -mcpu=apple-a14	
	FLAGS = 
else
ifeq ($(uname_S), Linux)
	CXXFLAGS = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
else
ifeq ($(OS), Windows_NT)
	CXXFLAGS = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
endif
endif
endif

ifeq ($(build), release-native)
	NATIVE   = -march=native
	ARCH     = -x86-64-native
endif

ifeq ($(build), release-x86-64)
	INSTRUCTIONS = -msse -msse2 -mpopcnt
	ARCH         = -x86-64
endif

ifeq ($(build), release-x86-64-modern)
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt
	ARCH         = -x86-64-modern
endif

ifeq ($(build), release-x86-64-avx2)
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt -msse4 -mavx -mavx2
	ARCH         = -x86-64-avx2
endif

ifeq ($(build), release-x86-64-bmi2)
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt -msse4.1 -mavx -mavx2 -mbmi -mbmi2 
	ARCH         = -x86-64-bmi2
endif

ifeq ($(build), debug)
	CXXFLAGS = -g3 -fno-omit-frame-pointer
	NATIVE   = -msse -msse3 -mpopcnt
endif

#Include github sha
ifdef sha
	CXXFLAGS += -DSHA=\"$(sha)\"
endif

SRCS := $(wildcard *.cpp) ./syzygy/Fathom/src/tbprobe.c
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
EXE  := $(NAME)$(ARCH)$(SUFFIX)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $(EXE) -flto $^ $(FLAGS)
%.o: %.cpp
	$(CC) -c $(CXXFLAGS) -funroll-loops $(NATIVE) $(INSTRUCTIONS) -O3 -std=c++17 -DEVALFILE=\"$(EVALFILE)\" -Wall -flto $< $(FLAGS)
clean:
	rm -rf *.o $(EXE)

.PHONY: all clean