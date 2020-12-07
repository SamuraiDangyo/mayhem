# Definitions

CXX=clang++
CXXFLAGS=-std=c++14 -O3 -march=native -mpopcnt -Wall -Wextra -Wshadow -pedantic -DNDEBUG -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
FILES=*.cpp lib/*.cpp
EXE=mayhem

# Targets

all:
	$(CXX) $(CXXFLAGS) $(FILES) -o $(EXE)

release:
	x86_64-w64-mingw32-g++ $(CXXFLAGS) -static -DWINDOWS $(FILES) -o $(EXE)-$(VER)-x86-windows-modern-64bit.exe
	$(CXX) $(CXXFLAGS) -static $(FILES) -o $(EXE)-$(VER)-x86-unix-modern-64bit

test:
	cutechess-cli -engine cmd=./$(EXE) dir=. proto=uci -engine cmd=fruit proto=uci -repeat -each tc=60+.1 -rounds 100 -resign movecount=8 score=500 -draw movenumber=40 movecount=10 score=30 -pgnout games.pgn

valgrind:
	g++ -Wall -O1 -mpopcnt -ggdb3 $(FILES)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./a.out --bench

xboard:
	xboard -fUCI -fcp ./$(EXE)

clean:
	rm -f $(EXE) $(EXE)-* *.pgn game.* log.* *.out *.txt

.PHONY: all release test valgrind xboard clean
