#
# Mayhem. Linux UCI Chess960 engine. Written in C++17 language
# Copyright (C) 2020-2021 Toni Helminen <GPLv3>
#

# Definitions

CXX=clang++
NNUEFLAGS=-DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
CXXFLAGS=-std=c++17 -O3 -march=native -mpopcnt -Wall -Wextra -pedantic -DNDEBUG $(NNUEFLAGS)
RELFLAGS=-std=c++17 -O3 -mpopcnt -Wall -Wextra -pedantic -DNDEBUG -static $(NNUEFLAGS)
FILES=main.cpp lib/nnue.cpp lib/polyglotbook.cpp
EXE=mayhem

# Targets

all:
	$(CXX) $(CXXFLAGS) -o $(EXE) $(FILES)

release:
	x86_64-w64-mingw32-g++ $(RELFLAGS) -DWINDOWS -o $(EXE)-$(VER)-x86-windows-slow-64bit.exe $(FILES)
	clang++ $(RELFLAGS) -o $(EXE)-$(VER)-x86-unix-slow-64bit $(FILES)

valgrind:
	g++ -Wall -O1 -mpopcnt -ggdb3 $(FILES)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose \
		--log-file=valgrind.txt ./a.out --bench

gprof:
	g++ -Wall -O1 -mpopcnt -g -pg -ggdb3 $(FILES)
	./a.out --bench
	gprof > gprof.txt

xboard:
	xboard -fUCI -fcp ./$(EXE)

clean:
	rm -f $(EXE) $(EXE)-* *.pgn game.* log.* *.out *.txt *.debug

.PHONY: all release valgrind gprof xboard clean
