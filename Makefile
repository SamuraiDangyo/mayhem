#
# Mayhem. Linux UCI Chess960 engine. Written in C++17 language
# Copyright (C) 2020-2021 Toni Helminen <GPLv3>
#

# 
# Definitions
#

CXX      = clang++
EXE      = mayhem
FILES    = main.cpp lib/nnue.cpp lib/polyglotbook.cpp

# For Windows add: -DWINDOWS 

BFLAGS   = -std=c++17 -O3 -flto -march=native -mpopcnt
WFLAGS   = -Wall -Wextra -pedantic -DNDEBUG
NFLAGS   = -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
CXXFLAGS = $(BFLAGS) $(WFLAGS) $(NFLAGS)

#
# Targets
# 

all:
	$(CXX) $(CXXFLAGS) -o $(EXE) $(FILES)

clean:
	rm -f $(EXE)

.PHONY: all clean
