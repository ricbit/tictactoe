# Set GOOGLE_TEST in your .bashrc as /home/ricbit/src/googletest or whatever.
TEST_BASE=${GOOGLE_TEST}/googletest
HEADERS = boarddata.hh semantic.hh strategies.hh minimax.hh state.hh elevator.hh \
          solutiontree.hh

all : tictactoe heatmap test minimax

tictactoe : tictactoe.cc ${HEADERS}
	g++-10 -std=c++2a tictactoe.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

minimax : minimax.cc ${HEADERS}
	g++-10 -std=c++2a minimax.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

minimaxc : minimax.cc ${HEADERS}
	clang-10 -std=c++2a minimax.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

phasediag : phasediag.cc ${HEADERS}
	g++-10 -std=c++2a phasediag.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

draw : phasediag
	./phasediag > phasediag.txt
	echo "plot 'phasediag.txt' using 1:2 title 'Branch factor' with lines, 125-x" \
	| gnuplot -persist

heatmap : heatmap.cc ${HEADERS}
	g++-10 -std=c++2a heatmap.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

heatmapc : heatmap.cc ${HEADERS}
	clang++-10 -std=c++2a heatmap.cc -o $@ -O3 -Wall -g -march=native -ltbb -lpthread

clang : tictactoe.cc ${HEADERS}
	clang++-10 -std=c++2a tictactoe.cc -o $@ -O3 -Wall -g -march=native -ltbb

test : test.cc ${HEADERS}
	g++-10 -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ -O3 -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread
	./test

singletest : test.cc ${HEADERS}
	g++-10 -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ -O3 -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread
	./singletest --gtest_filter=StateTest.Defens* > sym.txt

testc : test.cc ${HEADERS}
	clang++-10 -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ -O3 -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread
	./test

asm :
	g++ -std=c++2a tictactoe.cc -o $@ -O1 -Wall -g -S -masm=intel

spaces :
	for i in `ls *.hh *.cc *.py Makefile`; do sed -i "s/\s\+$$//g" $$i ; done

clean :
	rm -f test asm tictactoe

cppcheck :
	cppcheck --enable=style,warning tictactoe.cc heatmap.cc minimax.cc test.cc

solution.txt : minimax
	./minimax

html : solution.txt dumper.py
	python3 dumper.py > solution.html

flask : app.py solution.txt
	flask run

callminimax : minimax
	valgrind --tool=callgrind --dump-instr=yes  ./minimax
	kcachegrind `ls callgrind* -t | head -n 1`
