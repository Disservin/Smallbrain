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

smallbrain: uci.o search.o board.o threadmanager.o evaluation.o helper.o psqt.o benchmark.o timemanager.o
	g++ uci.o search.o board.o threadmanager.o evaluation.o helper.o psqt.o benchmark.o timemanager.o $(CFLAGS) -o smallbrain -lpthread 

uci.o: uci.cpp uci.h
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall uci.cpp 

search.o: search.cpp search.h
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall search.cpp

board.o: board.cpp board.h sliders2.hpp
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall board.cpp 

threadmanager.o: threadmanager.cpp threadmanager.h
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall threadmanager.cpp 

evaluation.o: evaluation.cpp evaluation.h
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall evaluation.cpp 

helper.o: helper.cpp helper.h
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall helper.cpp 

psqt.o: psqt.cpp psqt.h
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall psqt.cpp 

benchmark.o: benchmark.cpp benchmark.h
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall benchmark.cpp

timemanager.o: timemanager.cpp timemanager.h
	g++ -c -flto -O3 $(TARGET) -std=c++20 -static -Wall timemanager.cpp
clean:
	rm *.o *.exe