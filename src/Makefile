smallbrain: uci.o search.o board.o threadmanager.o evaluation.o helper.o psqt.o
	g++ uci.o search.o board.o threadmanager.o evaluation.o helper.o psqt.o -o smallbrain -lpthread 

uci.o: uci.cpp uci.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -Wall uci.cpp -lpthread 

search.o: search.cpp search.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -Wall search.cpp -lpthread 

board.o: board.cpp board.h sliders.hpp
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -Wall board.cpp -lpthread 

threadmanager.o: threadmanager.cpp threadmanager.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -Wall threadmanager.cpp -lpthread 

evaluation.o: evaluation.cpp evaluation.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -Wall evaluation.cpp -lpthread 

helper.o: helper.cpp helper.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -Wall helper.cpp -lpthread 

psqt.o: psqt.cpp psqt.h
	g++ -c -flto -O3 -march=native -std=c++20 -static-libgcc -static-libstdc++ -static -Wall psqt.cpp -lpthread 
clean:
	rm *.o *.exe