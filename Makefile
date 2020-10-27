# Definitions

CC=clang++
CFLAGS=-std=c++14 -Ofast -fomit-frame-pointer -march=native -Wall -pedantic -Wextra -DNDEBUG -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
EXE=mayhem
FILES=*.cpp

# Targets

all:
	$(CC) $(CFLAGS) $(FILES) -o $(EXE)

release: clean
	x86_64-w64-mingw32-g++ $(CFLAGS) -DWINDOWS $(FILES) -o mayhem-0.45-x86-windows-modern-64bit
	clang++ $(CFLAGS) $(FILES) -o mayhem-0.45-x86-unix-modern-64bit

strip:
	strip ./$(EXE)

clean:
	rm -f $(EXE) $(EXE)-*

play: all
	cutechess-cli -variant fischerandom -engine cmd=./$(EXE) dir=. proto=uci -engine cmd=sapeli dir=. proto=uci -each tc=60 -rounds 100

xboard: all
	xboard -fUCI -fcp ./$(EXE)

.PHONY: all release strip clean play xboard
