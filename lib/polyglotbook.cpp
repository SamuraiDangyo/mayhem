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

/*
  The code in this file is based on the opening book code in PolyGlot
  by Fabien Letouzey. PolyGlot is available under the GNU General
  Public License, and can be downloaded from http://wbec-ridderkerk.nl
*/

#include "polyglotbook.hpp"

//using namespace std;

polyglotbook::PolyglotBook::PolyglotBook() : polyboard{} {

  std::srand(static_cast<unsigned int>(std::time(nullptr)));

}

polyglotbook::PolyglotBook::~PolyglotBook() {

  if (is_open())
    close();

}


/// operator>>() reads sizeof(T) chars from the file's binary byte stream and
/// converts them into a number of type T. A Polyglot book stores numbers in
/// big-endian format.

template<typename T>
polyglotbook::PolyglotBook& polyglotbook::PolyglotBook::operator>>(T& n) {

  n = 0;
  for (std::size_t i = 0; i < sizeof(T); ++i)
    n = T((n << 8) + std::ifstream::get());

  return *this;

}

template<>
polyglotbook::PolyglotBook& polyglotbook::PolyglotBook::operator>>(Entry& e) {

  return *this >> e.key >> e.move >> e.count >> e.learn;

}

inline int polyglotbook::PolyglotBook::ctz(const std::uint64_t bb) {

  return __builtin_ctzll(bb);

}

inline std::uint64_t polyglotbook::PolyglotBook::clear_bit(const std::uint64_t bb) {

  return bb & (bb - 0x1ULL);

}

std::uint64_t polyglotbook::PolyglotBook::polyglot_key() {

  std::uint64_t key = 0;

  //
  // Board
  //
  for (auto both = polyboard.both; both; both = clear_bit(both)) {
    const auto sq = ctz(both);
    switch (polyboard.pieces[sq]) {
    case -1: key ^= polyglotbook::kPG.Zobrist.psq[0][sq];  break;
    case +1: key ^= polyglotbook::kPG.Zobrist.psq[1][sq];  break;
    case -2: key ^= polyglotbook::kPG.Zobrist.psq[2][sq];  break;
    case +2: key ^= polyglotbook::kPG.Zobrist.psq[3][sq];  break;
    case -3: key ^= polyglotbook::kPG.Zobrist.psq[4][sq];  break;
    case +3: key ^= polyglotbook::kPG.Zobrist.psq[5][sq];  break;
    case -4: key ^= polyglotbook::kPG.Zobrist.psq[6][sq];  break;
    case +4: key ^= polyglotbook::kPG.Zobrist.psq[7][sq];  break;
    case -5: key ^= polyglotbook::kPG.Zobrist.psq[8][sq];  break;
    case +5: key ^= polyglotbook::kPG.Zobrist.psq[9][sq];  break;
    case -6: key ^= polyglotbook::kPG.Zobrist.psq[10][sq]; break;
    case +6: key ^= polyglotbook::kPG.Zobrist.psq[11][sq]; break;
    }
  }

  //
  // Castling rights
  //
  if (polyboard.castle & 0x1) key ^= polyglotbook::kPG.Zobrist.castling[0];
  if (polyboard.castle & 0x2) key ^= polyglotbook::kPG.Zobrist.castling[1];
  if (polyboard.castle & 0x4) key ^= polyglotbook::kPG.Zobrist.castling[2];
  if (polyboard.castle & 0x8) key ^= polyglotbook::kPG.Zobrist.castling[3];

  //
  // En passant
  //
  if (ep_legal()) key ^= polyglotbook::kPG.Zobrist.enpassant[polyboard.epsq % 8];

  //
  // Turn
  //
  if (polyboard.wtm) key ^= polyglotbook::kPG.Zobrist.turn;

  //
  // The key is ready
  //
  return key;

}

/// open() tries to open a book file with the given name after closing any
/// existing one.

bool polyglotbook::PolyglotBook::open(const std::string &file) {

  if (is_open()) // Cannot close an already closed file
      close();

  std::ifstream::open(file, std::ifstream::in | std::ifstream::binary);

  const auto opened = is_open();
  std::ifstream::clear(); // Reset any error flag to allow a retry ifstream::open()

  return opened;

}

/// probe() tries to find a book move for the given position. If no move is
/// found, it returns MOVE_NONE. If pickBest is true, then it always returns
/// the highest-rated move, otherwise it randomly chooses one based on the
/// move score.

bool polyglotbook::PolyglotBook::open_book(const std::string &file) {

  return open(file);

}

bool polyglotbook::PolyglotBook::on_board(const int x, const int y) {

  return x >= 0 && x <= 7 && y >= 0 && y <= 7;

}

bool polyglotbook::PolyglotBook::ep_legal() {

  if (polyboard.epsq == -1)
    return false;

  const int x = polyboard.epsq % 8, y = polyboard.epsq / 8;

  return polyboard.wtm ? (on_board(x - 1, y) && polyboard.pieces[8 * y + x - 1] == -1) || (on_board(x + 1, y) && polyboard.pieces[8 * y + x + 1] == -1)
                       : (on_board(x - 1, y) && polyboard.pieces[8 * y + x - 1] == +1) || (on_board(x + 1, y) && polyboard.pieces[8 * y + x + 1] == +1);

}

polyglotbook::PolyglotBook& polyglotbook::PolyglotBook::setup(std::int8_t *pieces, const std::uint64_t both, const std::uint8_t castle, const std::int8_t epsq, const bool wtm) {

  polyboard.pieces = pieces;
  polyboard.both   = both;
  polyboard.castle = castle;
  polyboard.epsq   = epsq;
  polyboard.wtm    = wtm;

  return *this;

}

int polyglotbook::PolyglotBook::probe(const bool pickBest) {

  if (!is_open())
    return 0;

  Entry e;
  std::uint16_t best = 0;
  unsigned sum = 0;
  int move = 0;
  const auto key = polyglot_key();

  seekg(find_first(key) * sizeof(Entry), std::ios_base::beg);

  while (*this >> e, e.key == key && good()) {
      best = std::max(best, e.count);
      sum += e.count;

      // Choose book move according to its score. If a move has a very high
      // score it has a higher probability of being choosen than a move with
      // a lower score. Note that first entry is always chosen.
      if (   (!pickBest && sum && (std::rand() % sum) < e.count)
          || (pickBest && e.count == best)) {
          move = e.move;
      }
  }

  return move;

  // A PolyGlot book move is encoded as follows:
  //
  // bit  0- 5: destination square (from 0 to 63)
  // bit  6-11: origin square (from 0 to 63)
  // bit 12-14: promotion piece (from KNIGHT == 1 to QUEEN == 4)
  //
  // Castling moves follow the "king captures rook" representation. If a book
  // move is a promotion, we have to convert it to our representation and in
  // all other cases, we can directly compare with a Move after having masked
  // out the special Move flags (bit 14-15) that are not supported by PolyGlot.

}


/// find_first() takes a book key as input, and does a binary search through
/// the book file for the given key. Returns the index of the leftmost book
/// entry with the same key as the input.

std::size_t polyglotbook::PolyglotBook::find_first(const std::uint64_t key) {

  seekg(0, std::ios::end); // Move pointer to end, so tellg() gets file's size

  std::size_t low = 0, high = (std::size_t) tellg() / sizeof(Entry) - 1;

  //assert(low <= high);

  while (low < high && good()) {

    const std::size_t mid = (low + high) / 2;
    Entry e;

    //assert(mid >= low && mid < high);

    seekg(mid * sizeof(Entry), ios_base::beg);
    *this >> e;

    if (key <= e.key)
        high = mid;
    else
        low = mid + 1;
  }

  //assert(low == high);

  return low;

}
