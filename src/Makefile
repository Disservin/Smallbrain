_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
EVALFILE   = $(_ROOT)/default.nnue
CC        := g++
TARGET    := smallbrain
NATIVE     = -march=native

# name and version
NAME      := smallbrain
MINOR     := 0
MAJOR     := 6

ifeq ($(OS), Windows_NT)
	uname_S := Windows
	FLAGS  = -pthread -lstdc++
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
	CXXFLAGS = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
else
ifeq ($(OS), Windows_NT)
	CXXFLAGS = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
endif
endif
endif

ifeq ($(build), release-native)
	CXXFLAGS = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE   = -march=native
	FLAGS    = -pthread -lstdc++ -static
	ARCH     = -x86-64-native
endif

ifeq ($(build), release-x86-64)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=x86-64
	INSTRUCTIONS = -msse -msse2 -mpopcnt
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64
endif

ifeq ($(build), release-x86-64-sse3)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=x86-64
	INSTRUCTIONS = -m64 -msse -msse2 -mpopcnt -msse3 -mssse3 -mlzcnt
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-sse3
endif

ifeq ($(build), release-x86-64-modern)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=x86-64
	INSTRUCTIONS = -m64 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mcx16 -mbmi -mavx -mlzcnt -mpclmul
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-modern
endif

ifeq ($(build), release-x86-64-avx2)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=x86-64
	INSTRUCTIONS = -m64 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mcx16 -mbmi -mavx -mlzcnt -mpclmul -mavx2
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-avx2
endif

ifeq ($(build), release-x86-64-bmi2)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=x86-64
	INSTRUCTIONS = -m64 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mcx16 -mbmi -mavx -mlzcnt -mpclmul -mavx2 -mbmi2
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-bmi2
endif

ifeq ($(build), release-x86-64-zen2)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=znver2
	INSTRUCTIONS = 
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-zen2
endif

ifeq ($(build), release-x86-64-zen3)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=znver3
	INSTRUCTIONS = 
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-zen3
endif

ifeq ($(build), release-x86-64-haswell)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=haswell
	INSTRUCTIONS = 
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-haswell
endif

ifeq ($(build), release-x86-64-skylake)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=skylake
	INSTRUCTIONS = 
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-skylake
endif

ifeq ($(build), release-x86-64-sandybridge)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=sandybridge
	INSTRUCTIONS = 
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-sandybridge
endif

ifeq ($(build), release-x86-64-alderlake)
	CXXFLAGS     = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	NATIVE       = -march=alderlake
	INSTRUCTIONS = 
	FLAGS        = -pthread -lstdc++ -static
	ARCH         = -x86-64-alderlake
endif

ifeq ($(build), debug)
	CXXFLAGS = -g3 -fno-omit-frame-pointer
	NATIVE   = -msse -msse3 -mpopcnt
endif

SRCS := $(wildcard *.cpp) ./syzygy/Fathom/src/tbprobe.c
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
EXE  := $(NAME)$(MAJOR).$(MINOR)$(ARCH)$(SUFFIX)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CXXFLAGS) -o $(EXE) $^ $(FLAGS)
%.o: %.cpp
	$(CC) -c $(CXXFLAGS) -funroll-loops $(NATIVE) $(INSTRUCTIONS) -O3 -std=c++17 -DEVALFILE=\"$(EVALFILE)\" -flto -Wall $< $(FLAGS)
clean:
	rm -rf $(TARGET) *.o
	rm $(NAME)$(MAJOR).$(MINOR)$(SUFFIX)



.PHONY: all clean
