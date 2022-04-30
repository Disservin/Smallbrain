smallbrain: uci.o search.o board.o threadmanager.o evaluation.o helper.o 
	g++ uci.o search.o board.o threadmanager.o evaluation.o helper.o -o smallbrain 

uci.o: uci.cpp uci.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -lpthread -Wall uci.cpp

search.o: search.cpp search.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -lpthread -Wall search.cpp 

board.o: board.cpp board.h sliders.hpp
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -lpthread -Wall board.cpp

threadmanager.o: threadmanager.cpp threadmanager.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -lpthread -Wall threadmanager.cpp

evaluation.o: evaluation.cpp evaluation.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -lpthread -Wall evaluation.cpp

helper.o: helper.cpp helper.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -lpthread -Wall helper.cpp

clean:
	rm *.o *.exe