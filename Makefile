# Definitions

CXX=clang++
CXXFLAGS=-std=c++14 -Ofast -march=native -mpopcnt -fomit-frame-pointer -Wall -pedantic -Wextra -DNDEBUG -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
FILES=*.cpp
EXE=mayhem

# Targets

all:
	$(CXX) $(CXXFLAGS) $(FILES) -o $(EXE)

release: clean
	x86_64-w64-mingw32-g++ $(CXXFLAGS) -static -DWINDOWS $(FILES) -o $(EXE)-$(VER)-x86-windows-modern-64bit.exe
	$(CXX) $(CXXFLAGS) -static $(FILES) -o $(EXE)-$(VER)-x86-unix-modern-64bit

strip:
	strip ./$(EXE)

clean:
	rm -f $(EXE) $(EXE)-* *.pgn game.* log.*

test: all
	cutechess-cli -engine cmd=./$(EXE) dir=. proto=uci -engine cmd=crafty proto=xboard -each option.MoveOverhead=10 tc=60/40 -rounds 10 -resign movecount=4 score=500 -draw movenumber=40 movecount=12 score=30 -pgnout games.pgn

xboard: all
	xboard -fUCI -fcp ./$(EXE)

.PHONY: all release strip clean test xboard
