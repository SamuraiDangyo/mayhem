# Mayhem. Linux UCI Chess960 engine. Written in C++20 language
# Copyright (C) 2020-2022 Toni Helminen
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Definitions

CXX    = clang++
EXE    = mayhem
BFLAGS = -std=c++20 -O3 -march=native -DNDEBUG
WFLAGS = -Wall -Wextra -Wshadow -Wcast-qual -pedantic
NFLAGS = -DUSE_AVX2 -mavx2

# Targets

all:
	$(CXX) $(BFLAGS) $(WFLAGS) $(NFLAGS) $(CXXFLAGS) -o $(EXE) main.cpp

clean:
	rm -f $(EXE)

help:
	@echo '+-+-+-+-+ Compiling help +-+-+-+-+'
	@echo '-DWINDOWS, [CXX, CXXFLAGS, EXE, [BWN]FLAGS]= # Flags'
	@echo 'make -j # Simple build'
	@echo 'make -j NFLAGS="-DUSE_SSE2 -msse2" # Old CPU build'

.PHONY: all clean help
