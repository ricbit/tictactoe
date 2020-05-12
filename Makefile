tictactoe : tictactoe.cc
	g++ -std=c++2a $^ -o $@ -O3 -Wall -g

cppcheck :
	cppcheck --enable=style tictactoe.cc
