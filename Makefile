# Set GOOGLE_TEST in your .bashrc as /home/ricbit/src/googletest or whatever.
TEST_BASE=${GOOGLE_TEST}/googletest

tictactoe : tictactoe.cc semantic.hh tictactoe.hh
	g++ -std=c++2a tictactoe.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

clang : tictactoe.cc semantic.hh tictactoe.hh
	clang++ -std=c++2a tictactoe.cc -o $@ -O3 -Wall -g -march=native -ltbb

test : semantic.hh tictactoe.hh test.cc
	g++ -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ -O3 -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread
	./test

asm :
	g++ -std=c++2a tictactoe.cc -o $@ -O1 -Wall -g -S -masm=intel

clean :
	rm -f test asm tictactoe

cppcheck :
	cppcheck --enable=style tictactoe.cc
