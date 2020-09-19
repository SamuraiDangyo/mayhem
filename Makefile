#
# Mayhem is just (an experimental engine) Sapeli 1.90 written in C++14
# GNU General Public License version 3; for details see LICENSE
#

# Definitions

CC=g++
CFLAGS=-std=c++14 -O2 -march=native -Wall -pedantic -Wextra -DNDEBUG -pthread
EXE=mayhem
INSTALLDIR=/usr/bin/
NAME=mayhem.cpp

# Targets

all:
	$(CC) $(CFLAGS) $(NAME) -o $(EXE)

strip:
	strip ./$(EXE)

clean:
	rm -f $(EXE)*

install: all
	if [ -d $(INSTALLDIR) ]; then sudo cp -f $(EXE) $(INSTALLDIR); fi

help: all
	./$(EXE) --help

play: all
	cutechess-cli -variant fischerandom -engine cmd=./$(EXE) dir=. proto=uci -engine cmd=sapeli dir=. proto=uci -each tc=1 -rounds 100000

xboard: all
	xboard -fUCI -fcp ./$(EXE)

goodmoves: all
	python3.8 good-moves-gen.py --gen

endgame: all
	python3.8 endgame-gen.py --gen

uniques:
	uniq good-moves.nn good-moves-uniques.nn
	uniq white-wins.nn white-wins-uniques.nn
	uniq black-wins.nn black-wins-uniques.nn

.PHONY: all strip clean install help play xboard goodmoves endgame uniques
