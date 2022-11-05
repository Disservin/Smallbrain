_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
EVALFILE   = $(_ROOT)/default.nnue
CC        := g++
TARGET    := smallbrain
CXXFLAGS  :=
NATIVE     = -march=native
FLAGS     := -lstdc++ 
SUFFIX    :=

# name and version
NAME      := smallbrain

KERNEL = $(shell uname -s)
ifeq ($(KERNEL),Linux)
	OS = $(shell uname -o)
	CXXFLAGS += -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	FLAGS  = -lpthread -static
else
	uname_S := $(shell uname -s)
endif

ifeq ($(OS), Windows_NT)
	FLAGS   += -static
	SUFFIX  := .exe
	CXXFLAGS += -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
else
ifeq ($(COMP), mingw)
	FLAGS   += -static
	SUFFIX  := .exe
	CXXFLAGS += -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
else
ifeq ($(uname_S), Darwin)
	NATIVE = -mcpu=apple-a14	
	FLAGS = 
else
endif
endif
endif

ifdef build
	NATIVE =
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

ifeq ($(build), release-x86-64-avx512)
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt -msse4.1 -mavx -mavx2 -mbmi -mbmi2 -mavx512f -mavx512bw
	ARCH         = -x86-64-avx512
endif

ifeq ($(build), release-x86-64-vnni256)
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt -msse4.1 -mavx -mavx2 -mbmi -mbmi2 -mavx512f -mavx512bw -mavx512vnni -mavx512dq -mavx512vl -mprefer-vector-width=256
	ARCH         = -x86-64-vnni256
endif

ifeq ($(build), release-x86-64-vnni512)
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt -msse4.1 -mavx -mavx2 -mbmi -mbmi2 -mavx512vnni -mavx512dq -mavx512vl
	ARCH         = -x86-64-vnni512
endif

ifeq ($(build), debug)
	CXXFLAGS = -g3 -fno-omit-frame-pointer
	NATIVE   = -msse -msse3 -mpopcnt
endif

# Include sha for versioning 
SHA = $(shell git rev-parse --short HEAD 2>/dev/null)
ifneq ($(SHA), )
	CXXFLAGS += -DSHA=\"$(SHA)\"
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