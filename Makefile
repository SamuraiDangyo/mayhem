#
# Mayhem. Linux UCI Chess960 engine. Written in C++17 language
# Copyright (C) 2020-2021 Toni Helminen <GPLv3>
#

#
# Definitions
#

CXX      = clang++
EXE      = mayhem
FILES    = lib/polyglotbook.cpp lib/nnue.cpp main.cpp

# For Windows add: -DWINDOWS
# No avx2? Use: -DUSE_SSE -msse

BFLAGS   = -std=c++17 -O3 -flto -lm -march=native -mpopcnt
WFLAGS   = -Wall -Wextra -pedantic -DNDEBUG
NFLAGS   = -DUSE_AVX2 -mavx2
CXXFLAGS = $(BFLAGS) $(WFLAGS) $(NFLAGS)

#
# Targets
#

all:
	$(CXX) $(CXXFLAGS) -o $(EXE) $(FILES)

clean:
	rm -f $(EXE)

.PHONY: all clean
