_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
EVALFILE   = $(_ROOT)/default.nnue
CC        := g++
TARGET    := smallbrain

# name and version
NAME      := smallbrain
MINOR     := 0
MAJOR     := 5

ifeq ($(OS), Windows_NT)
	uname_S := Windows
	CXFLAGS  = -pthread -lstdc++
	SUFFIX  := .exe
	CO =  -O3 -std=c++17
else
	CXFLAGS  = -lpthread -lstdc++
	SUFFIX  :=
	uname_S := $(shell uname -s)
	CO =  -O3 -std=c++17
endif
ifeq ($(uname_S), Darwin)
	NATIVE = -mcpu=apple-a14	
	CXFLAGS = 
else
ifeq ($(uname_S), Linux)
	NATIVE = -mavx2
	CFLAGS = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
else
ifeq ($(OS), Windows_NT)
	NATIVE = -mavx2
	CFLAGS = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
endif
endif
endif

ifeq ($(build), release)
	CFLAGS = -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	CXFLAGS  = -pthread -lstdc++ -static
endif

ifeq ($(build), debug)
	CFLAGS = -g3 -ggdb3 -pg -fno-omit-frame-pointer
	CO = -O0 -std=c++17
endif

ifeq ($(ARCH),)
	NATIVE=x86-64-modern
endif

ifeq ($(ARCH),x86-64)
	NATIVE += -msse
endif

ifeq ($(ARCH),x86-64-modern)
	NATIVE += -msse -msse3 -mpopcnt
endif

ifeq ($(ARCH),x86-64-bmi2)
	NATIVE += -msse -msse3 -mpopcnt -msse4 -mbmi2
endif


SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
EXE := $(NAME)$(MAJOR).$(MINOR)$(SUFFIX)

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXE) $^ $(CXFLAGS)
%.o: %.cpp
	$(CC) -c $(CFLAGS) $(NATIVE) $(CO) -DEVALFILE=\"$(EVALFILE)\" -flto -Wall $< $(CXFLAGS)
clean:
	rm -rf $(TARGET) *.o
	rm *.exe
	rm $(EXE)
	
.PHONY: all clean
