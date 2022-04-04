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

# Definitions

CXX       = clang++
EXE       = mayhem
FILES     = main.cpp

BFLAGS    = -std=c++17 -flto -O3 -march=native
WFLAGS    = -Wall -Wextra -Wshadow -Wcast-qual -pedantic -DNDEBUG
NFLAGS    = -DUSE_AVX2 -mavx2
CXXFLAGS += $(BFLAGS) $(WFLAGS) $(NFLAGS)

# Targets

all:
	$(CXX) $(CXXFLAGS) -o $(EXE) $(FILES)

# For old CPUs
oldcpu:
	g++ $(BFLAGS) $(WFLAGS) -DUSE_SSE2 -msse2 -o $(EXE) $(FILES)

clean:
	rm -f $(EXE)

# If this file exists
.PHONY: all oldcpu clean
