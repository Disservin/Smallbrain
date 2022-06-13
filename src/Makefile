_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
EVALFILE   = $(_ROOT)/default.net
CC := g++
TARGET := smallbrain

ifeq ($(OS), Windows_NT)
	uname_S := Windows
	CXFLAGS = -pthread
else
	CXFLAGS = -lpthread
	uname_S := $(shell uname -s)
endif
ifeq ($(uname_S), Darwin)
	NATIVE = -mcpu=apple-a14
else
	NATIVE = -march=native
	CFLAGS = -static -static-libgcc -static-libstdc++ 
endif

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CXFLAGS)
%.o: %.cpp
	$(CC) -c $(CFLAGS) $(NATIVE) -DEVALFILE=\"$(EVALFILE)\" -flto -O3 -std=c++20 -Wall $< $(CXFLAGS)
clean:
	rm -rf $(TARGET) *.o
	
.PHONY: all clean