# Mayhem. Linux UCI Chess960 engine. Written in C++17 language
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

# Examples

# make -j CXX="g++"
# make -j CXX="g++" NFLAGS="-DUSE_SSE2 -msse2" EXE="mayhem-sse2"

# Definitions

CXX    = clang++
EXE    = mayhem
BFLAGS = -std=c++17 -flto -O3 -march=native -DNDEBUG
WFLAGS = -Wall -Wextra -Wshadow -Wcast-qual -pedantic
NFLAGS = -DUSE_AVX2 -mavx2

# Targets

all:
	$(CXX) $(BFLAGS) $(WFLAGS) $(NFLAGS) $(CXXFLAGS) -o $(EXE) main.cpp

clean:
	rm -f $(EXE)

.PHONY: all clean
