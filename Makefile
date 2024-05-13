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

CXX    = clang++
EXE    = mayhem
BIN    = /usr/bin
BFLAGS = -std=c++20 -O3 -flto -march=native -DNDEBUG -DMAYHEMBOOK -DMAYHEMNNUE
WFLAGS = -Wall -Wextra -Wshadow -pedantic
NFLAGS = -DUSE_AVX2 -mavx2 -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2

# Targets

all:
	$(CXX) $(BFLAGS) $(WFLAGS) $(NFLAGS) $(CXXFLAGS) -o $(EXE) main.cpp

install:
	sudo cp $(EXE) $(BIN)
	sudo chmod 555 $(BIN)/$(EXE)
	sudo cp final-book.bin $(BIN)
	sudo cp nn-cb80fb9393af.nnue $(BIN)

strip:
	strip $(EXE)

clean:
	rm -f $(EXE)

help:
	@echo "#+# Mayhem #+#"
	@echo ""
	@echo "To compile Mayhem, type:"
	@echo ""
	@echo "> make [target] [OPT=option ...] [-FLAG ...]"
	@echo ""
	@echo "Supported targets:"
	@echo ""
	@echo "all      # Build"
	@echo "install  # Installation"
	@echo "strip    # Strip executable"
	@echo "clean    # Clean up"
	@echo "help     # This help"
	@echo ""
	@echo "Examples:"
	@echo ""
	@echo "> make -j                 # Just build"
	@echo "> make all strip install  # Install"

.PHONY: all install clean help
