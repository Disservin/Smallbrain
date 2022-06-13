_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
EVALFILE   = $(_ROOT)/default.net

ifeq ($(OS), Windows_NT)
    uname_S := Windows
else
	uname_S := $(shell uname -s)
endif
ifeq ($(uname_S), Darwin)
	TARGET = -mcpu=apple-a14
else
	TARGET = -march=native
endif

ifeq ($(build), release)
	CFLAGS = -static-libgcc -static-libstdc++ -static
endif

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

smallbrain: $(TARGET)
$(TARGET): $(OBJS)
	g++ -o smallbrain $@ $^

%.o: %.cpp
	g++ -c -DEVALFILE=\"$(EVALFILE)\" -flto -O3 $(TARGET) $(CFLAGS) -std=c++20 -static -Wall $< 

clean:
	rm *.o *.exe