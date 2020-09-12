# Set GOOGLE_TEST in your .bashrc as /home/ricbit/src/googletest or whatever.
TEST_BASE=${GOOGLE_TEST}/googletest
HEADERS = boarddata.hh semantic.hh strategies.hh minimax.hh state.hh elevator.hh \
          solutiontree.hh boardnode.hh traversal.hh node.hh
OPT = -O3
OPTTEST = -O0

all : tictactoe heatmap test minimax

tictactoe : tictactoe.cc ${HEADERS}
	g++-10 -std=c++2a tictactoe.cc -o $@ ${OPT} -Wall -g -march=native -ltbb -lpthread

minimax : minimax.cc ${HEADERS}
	g++-10 -std=c++2a minimax.cc -o $@ ${OPT} -Wall -g -march=native -ltbb -lpthread

minimaxc : minimax.cc ${HEADERS}
	clang++-10 -std=c++2a minimax.cc -o $@ ${OPT} -Wall -g -march=native -ltbb -lpthread

phasediag : phasediag.cc ${HEADERS}
	g++-10 -std=c++2a phasediag.cc -o $@ ${OPT} -Wall -g -march=native -ltbb -lpthread

draw : phasediag
	./phasediag > phasediag.txt
	echo "plot 'phasediag.txt' using 1:2 title 'Branch factor' with lines, 125-x" \
	| gnuplot -persist

heatmap : heatmap.cc ${HEADERS}
	g++-10 -std=c++2a heatmap.cc -o $@ ${OPT} -Wall -g -march=native -ltbb -lpthread

heatmapc : heatmap.cc ${HEADERS}
	clang++-10 -std=c++2a heatmap.cc -o $@ ${OPT} -Wall -g -march=native -ltbb -lpthread

clang : tictactoe.cc ${HEADERS}
	clang++-10 -std=c++2a tictactoe.cc -o $@ ${OPT} -Wall -g -march=native -ltbb

test : test.cc ${HEADERS}
	g++-10 -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ ${OPTTEST} -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread
	./test

testcompile : test.cc ${HEADERS}
	g++-10 -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ ${OPTTEST} -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread

singletest : test.cc ${HEADERS}
	g++-10 -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ ${OPTTEST} -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread
	./singletest --gtest_filter=MiniMaxTest.Check32PNS* > sym.txt

testc : test.cc ${HEADERS}
	clang++-10 -std=c++2a -I${TEST_BASE}/include/ -L${TEST_BASE}/build/lib test.cc -o $@ ${OPTTEST} -Wall -g -march=native -ltbb -lgtest -lgtest_main -lpthread
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
	./minimax solution.txt

html : solution.txt dumper.py
	python3 dumper.py > solution.html

flask : app.py solution.txt
	flask run

callminimax : minimax
	valgrind --tool=callgrind --dump-instr=yes  ./minimax
	kcachegrind `ls callgrind* -t | head -n 1`

cpd :
	/home/ricbit/src/pmd-bin-6.25.0/bin/run.sh cpd --exclude test.cc --files . --language cpp --minimum-tokens 10

branch :
	tail solution.txt -n+2 | awk '{print NF - 8}' | sort -g | uniq -c

