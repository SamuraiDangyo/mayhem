/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2014 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2020-2021 Toni Helminen (Mayhem author / Modifications)

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

namespace polyglotbook {

PolyglotBook::PolyglotBook() : polyboard{} {

  std::srand(static_cast<unsigned int>(std::time(nullptr)));

}

PolyglotBook::~PolyglotBook() {

  if (this->is_open())
    this->close();

}

/// operator>>() reads sizeof(T) chars from the file's binary byte stream and
/// converts them into a number of type T. A Polyglot book stores numbers in
/// big-endian format.

template<typename T>
PolyglotBook& PolyglotBook::operator>>(T& n) {

  n = 0;
  for (std::size_t i = 0; i < sizeof(T); ++i)
    n = T((n << 8) + std::ifstream::get());

  return *this;

}

template<>
PolyglotBook& PolyglotBook::operator>>(Entry& e) {

  return *this >> e.key >> e.move >> e.count >> e.learn;

}

inline int PolyglotBook::ctz(const std::uint64_t bb) {

  return __builtin_ctzll(bb);

}

inline int PolyglotBook::ctz_pop(std::uint64_t *bb) {

  const auto ret = this->ctz(*bb);
  *bb = *bb & (*bb - 0x1ULL);
  return ret;

}

std::uint64_t PolyglotBook::polyglot_key() {

  std::uint64_t key = 0;

  //
  // Board
  //
  for (auto both{this->polyboard.both}; both; )
    switch (const auto sq = this->ctz_pop(&both); this->polyboard.pieces[sq]) {
      case +1: key ^= kZobrist.PG.psq[1][sq];  break;
      case +2: key ^= kZobrist.PG.psq[3][sq];  break;
      case +3: key ^= kZobrist.PG.psq[5][sq];  break;
      case +4: key ^= kZobrist.PG.psq[7][sq];  break;
      case +5: key ^= kZobrist.PG.psq[9][sq];  break;
      case +6: key ^= kZobrist.PG.psq[11][sq]; break;
      case -1: key ^= kZobrist.PG.psq[0][sq];  break;
      case -2: key ^= kZobrist.PG.psq[2][sq];  break;
      case -3: key ^= kZobrist.PG.psq[4][sq];  break;
      case -4: key ^= kZobrist.PG.psq[6][sq];  break;
      case -5: key ^= kZobrist.PG.psq[8][sq];  break;
      case -6: key ^= kZobrist.PG.psq[10][sq]; break;
    }

  //
  // Castling rights
  //
  if (this->polyboard.castling & 0x1) key ^= kZobrist.PG.castling[0];
  if (this->polyboard.castling & 0x2) key ^= kZobrist.PG.castling[1];
  if (this->polyboard.castling & 0x4) key ^= kZobrist.PG.castling[2];
  if (this->polyboard.castling & 0x8) key ^= kZobrist.PG.castling[3];

  //
  // En passant
  //
  if (this->is_ep_legal()) key ^= kZobrist.PG.enpassant[this->polyboard.epsq % 8];

  //
  // Turn
  //
  if (this->polyboard.wtm) key ^= kZobrist.PG.turn;

  //
  // The key is ready
  //
  return key;

}

/// open() tries to open a book file with the given name after closing any
/// existing one.

bool PolyglotBook::open(const std::string &file) {

  if (this->is_open()) // Cannot close an already closed file
      this->close();

  std::ifstream::open(file, std::ifstream::in | std::ifstream::binary);
  const auto opened = this->is_open();
  std::ifstream::clear(); // Reset any error flag to allow a retry ifstream::open()

  return opened;

}

/// probe() tries to find a book move for the given position. If no move is
/// found, it returns MOVE_NONE. If pickBest is true, then it always returns
/// the highest-rated move, otherwise it randomly chooses one based on the
/// move score.

bool PolyglotBook::open_book(const std::string &file) {

  return this->open(file);

}

bool PolyglotBook::on_board(const int x) {

  return x >= 0 && x <= 7;

}

bool PolyglotBook::is_ep_legal() {

  // -1 means no en passant possible
  if (this->polyboard.epsq < 0 || this->polyboard.epsq > 63)
    return false;

  const auto x = this->polyboard.epsq % 8;
  const auto y = this->polyboard.epsq / 8;

  return this->polyboard.wtm ?
      (this->on_board(x - 1) && this->polyboard.pieces[8 * y + x - 1] == -1) ||
      (this->on_board(x + 1) && this->polyboard.pieces[8 * y + x + 1] == -1)
        :
      (this->on_board(x - 1) && this->polyboard.pieces[8 * y + x - 1] == +1) ||
      (this->on_board(x + 1) && this->polyboard.pieces[8 * y + x + 1] == +1);

}

PolyglotBook& PolyglotBook::setup(std::int8_t *pieces, const std::uint64_t both,
    const std::uint8_t castling, const std::int8_t epsq, const bool wtm) {

  this->polyboard.pieces   = pieces;
  this->polyboard.both     = both;
  this->polyboard.castling = castling;
  this->polyboard.epsq     = epsq;
  this->polyboard.wtm      = wtm;

  return *this;

}

int PolyglotBook::probe(const bool pick_best) {

  if (!this->is_open())
    return 0;

  Entry e            = {};
  std::uint16_t best = 0;
  unsigned sum       = 0;
  int move           = 0;
  const auto key     = this->polyglot_key();

  this->seekg(this->find_first(key) * sizeof(Entry), std::ios_base::beg);

  while (*this >> e, e.key == key && this->good()) {
      best = std::max(best, e.count);
      sum += e.count;

      // Choose book move according to its score. If a move has a very high
      // score it has a higher probability of being choosen than a move with
      // a lower score. Note that first entry is always chosen.
      if (  (!pick_best && sum && (std::rand() % sum) < e.count) ||
            ( pick_best && e.count == best))
        move = e.move;
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

std::size_t PolyglotBook::find_first(const std::uint64_t key) {

  this->seekg(0, std::ios::end); // Move pointer to end, so tellg() gets file's size

  std::size_t low = 0, high = static_cast<std::size_t>(this->tellg()) / sizeof(Entry) - 1;
  Entry e{};

  while (low < high && this->good()) {

    const std::size_t mid = (low + high) / 2;

    this->seekg(mid * sizeof(Entry), std::ios_base::beg);
    *this >> e;

    if (key <= e.key)
      high = mid;
    else
      low = mid + 1;
  }

  return low;

}

} // namespace polyglotbook
