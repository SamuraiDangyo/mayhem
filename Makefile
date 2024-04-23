# Mayhem. Linux UCI Chess960 engine. Written in C++20 language
# Copyright (C) 2020-2024 Toni Helminen
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

CXX        = clang++
EXE        = mayhem
BIN_FOLDER = /usr/bin/
BFLAGS     = -std=c++20 -O3 -flto=auto -march=native -DNDEBUG -DMAYHEMBOOK -DMAYHEMNNUE
WFLAGS     = -Wall -Wextra -Wshadow -pedantic
NFLAGS     = -DUSE_AVX2 -mavx2

# Targets

all:
	$(CXX) $(BFLAGS) $(WFLAGS) $(NFLAGS) $(CXXFLAGS) -o $(EXE) main.cpp

install:
	chmod 555 mayhem
	sudo cp mayhem $(BIN_FOLDER)
	sudo cp final-book.bin $(BIN_FOLDER)
	sudo cp nn-cb80fb9393af.nnue $(BIN_FOLDER)

clean:
	rm -f $(EXE)

help:
	@echo '+-+ Mayhem compilation +-+'
	@echo ''
	@echo '-DWINDOWS             # Windows build flag'
	@echo '-DMAYHEMBOOK          # Use PolyGlot book'
	@echo '-DMAYHEMNNUE          # Use NNUE evaluation'
	@echo '-DUSE_AVX2 -mavx2     # Use avx2'
	@echo '-DUSE_SSE41 -msse4.1  # Use sse4.1'
	@echo '-DUSE_SSSE3 -mssse3   # Use sse3'
	@echo ''
	@echo 'make all              # Simple build target'
	@echo 'make install          # Install Mayhem on your system'
	@echo 'make clean            # Clean up'
	@echo 'make help             # This help'
	@echo ''
	@echo 'CXX=                  # Compiler option'
	@echo 'EXE=                  # Exe name'
	@echo 'BIN_FOLDER=           # Where to install Mayhem'
	@echo 'BFLAGS=               # Build flags'
	@echo 'WFLAGS=               # Warning flags'
	@echo 'NFLAGS=               # NN flags'
	@echo 'CXXFLAGS=             # Extra flags'
	@echo ''
	@echo '> make -j                         # > Simple build'
	@echo '> make all install clean          # > Install'
	@echo '> make NFLAGS="-DUSE_SSE2 -msse2" # > Old CPU build'

.PHONY: all install clean help
