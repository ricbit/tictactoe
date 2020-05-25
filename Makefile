# Set GOOGLE_TEST in your .bashrc as /home/ricbit/src/googletest or whatever.
TEST_BASE=${GOOGLE_TEST}/googletest
HEADERS = boarddata.hh semantic.hh tictactoe.hh state.hh

all : tictactoe heatmap test

tictactoe : tictactoe.cc ${HEADERS}
	g++-10 -std=c++2a tictactoe.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

heatmap : heatmap.cc ${HEADERS}
	g++-10 -std=c++2a heatmap.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

heatmapc : heatmap.cc ${HEADERS}
	clang++-10 -std=c++2a heatmap.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

clang : tictactoe.cc ${HEADERS}
	clang++-10 -std=c++2a tictactoe.cc -o $@ -O3 -Wall -g -march=native -ltbb

test : test.cc ${HEADERS}
	g++-10 -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ -O3 -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread
	./test

asm :
	g++ -std=c++2a tictactoe.cc -o $@ -O1 -Wall -g -S -masm=intel

spaces :
	for i in `ls *.hh *.cc Makefile`; do sed -i "s/\s\+$$//g" $$i ; done

clean :
	rm -f test asm tictactoe

cppcheck :
	cppcheck --enable=style tictactoe.cc
