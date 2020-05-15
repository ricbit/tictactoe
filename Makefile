tictactoe : tictactoe.cc semantic.hh
	g++ -std=c++2a tictactoe.cc -o $@ -O3 -Wall -g -march=native

asm :
	g++ -std=c++2a tictactoe.cc -o $@ -O1 -Wall -g -S -masm=intel

cppcheck :
	cppcheck --enable=style tictactoe.cc
