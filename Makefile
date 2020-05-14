tictactoe : tictactoe.cc semantic.hh
	g++ -std=c++2a tictactoe.cc -o $@ -O3 -Wall -g

cppcheck :
	cppcheck --enable=style tictactoe.cc
