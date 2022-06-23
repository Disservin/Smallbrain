_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
EVALFILE   = $(_ROOT)/default.net
CC        := g++
TARGET    := smallbrain

# name and version
NAME      := smallbrain
MINOR     := 0
MAJOR     := 4

ifeq ($(OS), Windows_NT)
	uname_S := Windows
	CXFLAGS  = -pthread -lstdc++
	SUFFIX  := .exe
else
	CXFLAGS  = -lpthread -lstdc++
	SUFFIX  :=
	uname_S := $(shell uname -s)
endif
ifeq ($(uname_S), Darwin)
	NATIVE = -mcpu=apple-a14
else
	NATIVE = -march=native
	CFLAGS = -static -static-libgcc -static-libstdc++
endif

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
EXE := $(NAME)_$(MAJOR).$(MINOR)$(SUFFIX)

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXE) $^ $(CXFLAGS)
%.o: %.cpp
	$(CC) -c $(CFLAGS) $(NATIVE) -DEVALFILE=\"$(EVALFILE)\" -flto -O3 -std=c++20 -Wall $< $(CXFLAGS)
clean:
	rm -rf $(TARGET) *.o
	
.PHONY: all clean