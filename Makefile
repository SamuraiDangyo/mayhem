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
BIN_FOLDER = /usr/bin
BFLAGS     = -std=c++20 -O3 -flto -march=native -DNDEBUG -DMAYHEMBOOK -DMAYHEMNNUE
WFLAGS     = -Wall -Wextra -Wshadow -pedantic
NFLAGS     = -DUSE_AVX2 -mavx2 -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2

# Targets

all:
	$(CXX) $(BFLAGS) $(WFLAGS) $(NFLAGS) $(CXXFLAGS) -o $(EXE) main.cpp

install:
	sudo cp mayhem $(BIN_FOLDER)
	sudo chmod 555 $(BIN_FOLDER)/mayhem
	sudo cp final-book.bin $(BIN_FOLDER)
	sudo cp nn-cb80fb9393af.nnue $(BIN_FOLDER)

strip:
	strip $(EXE)

clean:
	rm -f $(EXE)

help:
	@echo "+-+ Mayhem Compilation +-+"
	@echo ""
	@echo "To compile Mayhem, type:"
	@echo ""
	@echo "> make [target] [OPT=option ...] [-FLAG ...]"
	@echo ""
	@echo "Supported targets:"
	@echo ""
	@echo "all           # Build"
	@echo "install       # Installation"
	@echo "strip         # Strip executable"
	@echo "clean         # Clean up"
	@echo "help          # This help"
	@echo ""
	@echo "Supported flags:"
	@echo ""
	@echo "-DWINDOWS      # Windows build flag"
	@echo "-DMAYHEMBOOK   # Use PolyGlot book"
	@echo "-DMAYHEMNNUE   # Use NNUE evaluation"
	@echo ""
	@echo "Supported options:"
	@echo ""
	@echo "CXX=           # Compiler option"
	@echo "EXE=           # Executable name"
	@echo "BIN_FOLDER=    # Where to install Mayhem"
	@echo "BFLAGS=        # Build opts"
	@echo "WFLAGS=        # Warning opts"
	@echo "NFLAGS=        # NN opts"
	@echo "CXXFLAGS=      # Extra opts"
	@echo ""
	@echo "Simple examples:"
	@echo ""
	@echo "> make -j                      # Just build"
	@echo "> make all strip install clean # Install"

.PHONY: all install clean help
