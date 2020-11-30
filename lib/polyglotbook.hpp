/*
  Copyright (C) 2020 Toni Helminen (Mayhem author / Modifications)
*/

/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2014 Marco Costalba, Joona Kiiski, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <fstream>
#include <string>
#include <algorithm>
#include <iostream>
#include <ctime>

class PolyglotBook : private std::ifstream {

public:

  PolyglotBook();
 ~PolyglotBook();
  int probe(const bool);
  void setup(std::int8_t*, const std::uint64_t, const std::uint8_t, const std::int8_t, const bool);
  bool open_book(const std::string&);

private:

  template<typename T> PolyglotBook& operator>>(T&);

  // A Polyglot book is a series of "entries" of 16 bytes. All integers are
  // stored in big-endian format, with the highest byte first (regardless of
  // size). The entries are ordered according to the key in ascending order.
  struct Entry {
    std::uint64_t key;
    std::uint16_t move;
    std::uint16_t count;
    std::uint32_t learn;
  };

  // Board for building a hash key
  struct PolyBoard {
    std::uint64_t both;
    std::int8_t *pieces, epsq; 
    std::uint8_t castle, wtm;
  } polyboard;

  int ctz(const std::uint64_t);
  std::uint64_t clear_bit(const std::uint64_t);
  std::uint64_t polyglot_key();
  bool open(const std::string&);
  std::size_t find_first(const std::uint64_t);
  bool on_board(const int, const int);
  bool ep_legal();

};
