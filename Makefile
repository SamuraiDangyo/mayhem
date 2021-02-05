#
# Mayhem. Linux UCI Chess960 engine. Written in C++14 language
# Copyright (C) 2020-2021 Toni Helminen <GPLv3>
#

# Definitions

CXX=clang++
CXXFLAGS=-std=c++14 -O3 -march=native -mpopcnt -Wall -Wextra -pedantic -DNDEBUG -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
FILES=main.cpp lib/nnue.cpp lib/polyglotbook.cpp
EXE=mayhem

# Targets

all:
	$(CXX) $(CXXFLAGS) $(FILES) -o $(EXE)

release:
	x86_64-w64-mingw32-g++ $(CXXFLAGS) -static -DWINDOWS $(FILES) -o $(EXE)-$(VER)-x86-windows-modern-64bit.exe
	$(CXX) $(CXXFLAGS) -static $(FILES) -o $(EXE)-$(VER)-x86-unix-modern-64bit

valgrind:
	g++ -Wall -O1 -mpopcnt -ggdb3 $(FILES)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind.txt ./a.out --bench

gprof:
	g++ -Wall -O1 -mpopcnt -g -pg -ggdb3 $(FILES)
	./a.out --bench
	gprof > gprof.txt

test:
	cutechess-cli -engine cmd=./$(EXE) dir=. proto=uci -engine cmd=fruit proto=uci -each tc=60+1 -rounds 100 -pgnout games.pgn

xboard:
	xboard -fUCI -fcp ./$(EXE)

clean:
	rm -f $(EXE) $(EXE)-* *.pgn game.* log.* *.out *.txt *.debug

.PHONY: all release valgrind gprof test xboard clean
