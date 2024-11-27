/*
Mayhem. Linux UCI Chess960 engine. Written in C++20 language
Copyright (C) 2020-2024 Toni Helminen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// Header guard

#pragma once

// Headers

#include <bit>
#include <algorithm>
#include <memory>
#include <iostream>
#include <sstream>
#include <vector>
#include <array>
#include <string>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <numeric>
#include <utility>

extern "C" {
#ifdef WINDOWS
#include <conio.h>
#else
#include <unistd.h>
#endif
} // extern "C"

#include "nnue.hpp"
#include "polyglotbook.hpp"
#include "eucalyptus.hpp"

// Namespace

namespace mayhem {

// Macros

#define VERSION            "Mayhem 8.5" // Version
#define MAX_MOVES          256      // Max chess moves
#define MAX_SEARCH_DEPTH   64       // Max search depth (Stack frame problems ...)
#define MAX_Q_SEARCH_DEPTH 16       // Max Qsearch depth
#define INF                1048576  // System max number
#define DEF_HASH_MB        256      // MiB
#define NOISE              2        // Noise for opening moves
#define MOVEOVERHEAD       100      // ms
#define REPS_DRAW          3        // 3rd repetition is a draw
#define FIFTY              100      // 100 moves w/o progress is a draw (256 max)
#define R50_ARR            (FIFTY + 2) // Checkmate overrules 50 move rep so extra space here
#define SHUFFLE            30       // Allow shuffling then scale
#define BOOK_MS            100      // At least 100ms+ for the book lookup
#define PERFT_DEPTH        6        // Perft at depth 6
#define BENCH_DEPTH        14       // Bench at depth 14
#define BENCH_SPEED        10000    // Bench for 10s
#define BOOK_BEST          false    // Nondeterministic opening play
#define READ_CLOCK         0x1FFULL // Read clock every 512 ticks (white / 2 x both)
#define STARTPOS           "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" // UCI startpos
#define WEEK               (7 * 24 * 60 * 60 * 1000) // ms
#define MAX_PIECES         (2 * (8 * 1 + 2 * 3 + 2 * 3 + 2 * 5 + 1 * 9 + 1 * 0)) // Max pieces on board (Kings always exist)
#define EVAL_FILE          "nn-cb80fb9393af.nnue" // Default NNUE evaluation file
#define BOOK_FILE          "final-book.bin"       // Default Polyglot book file
#define FRC_PENALTY        100      // Penalty for bishop blocked on corner by own pawn
#define TEMPO_BONUS        25       // Bonus for the side to move
#define BISHOP_PAIR_BONUS  20       // Both colored bishops bonus
#define CHECKS_BONUS       17       // Bonus for checks

// Use NNUE evaluation (From make) ?
#ifdef MAYHEMNNUE
#define USE_NNUE true
#else
#define USE_NNUE false
#endif

// Use Polyglot book ?
#ifdef MAYHEMBOOK
#define USE_BOOK true
#else
#define USE_BOOK false
#endif

// Constants

// Tactical fens to pressure search
const std::vector<std::string> kBench = {
  "r2q2k1/pQ2bppp/4p3/8/3r1B2/6P1/P3PP1P/1R3RK1 w - - 0 1 ; bm f4b8",
  "q5k1/5pp1/8/1pb1P3/2p4p/2P2r1P/1P3PQ1/1N3R1K b - - 0 1 ; bm f3h3",
  "3r2k1/1pp4p/p1n1q1p1/2Q5/1P2B3/P3P1Pb/3N1R1P/6K1 b - - 1 1 ; bm e6e4",
  "8/8/8/4p3/4k3/8/8/4K3 w - - 0 1 ; bm e1e2",
  "2n3k1/P5P1/6K1/8/8/8/8/8 w - - 0 1 ; bm a7a8b",
  "7K/P1p1p1p1/2P1P1Pk/6pP/3p2P1/1P6/3P4/8 w - - 0 1 ; bm a7a8n",
  "5K2/k7/4P1p1/8/8/8/4b3/8 w - - 0 1 ; bm f8e7",
  "8/k7/P2b2P1/KP1Pn2P/4R3/8/6np/8 w - - 0 1 ; bm b5b6",
  "7k/5K2/7P/6pP/8/8/8/8 w - - 0 1 ; bm f7f6",
  "8/6PK/5k2/8/8/8/8/6r1 w - - 0 1 ; bm g7g8n",
  "r7/2k1Pp1p/p1n2p2/P1b1r3/2p5/2P3P1/5P1P/1R1Q2K1 w - - 0 1 ; bm b1b7",
  "3r2k1/pp5p/6p1/2Ppq3/4Nr2/4B2b/PP2P2K/R1Q1R2B b - - 0 1 ; bm f4f2",
  "2r3k1/q5pp/4p3/2rp1p2/1p1B1P2/1P1QP3/P1R3PP/6K1 w - - 0 1 ; bm d3b5",
  "8/8/8/8/8/k5R1/2rn4/K7 b - - 55 94 ; bm d2b3",
  "8/k7/p1p5/2P1p3/1P2B3/P3P3/3K2p1/6n1 b - - 0 1 ; bm g1f3",
  "1q3r1k/r4ppp/5n2/8/3Q1N2/p6R/PPP5/1K5R w - - 0 1 ; bm d4f6",
  "8/r2n1p2/1r1N1Pk1/3pP1p1/1p4P1/qPp2K2/P1R4R/8 w - - 0 1 ; bm h2h6",
  "rnb2rk1/pp1nqppp/4p3/3pP3/3p3P/2NB3N/PPP2PP1/R2QK2R w KQ - 0 1 ; bm d3h7",
  "3r4/k1P5/P7/1K6/8/8/8/8 w - - 0 1 ; bm c7d8n",
  "8/4R2n/4K1pk/6p1/7P/8/8/8 w - - 0 1 ; bm e7h7",
  "R7/P4k2/8/8/8/8/r7/6K1 w - - 0 1 ; bm a8h8",
  "2kr3r/pp1q1ppp/5n2/1Nb5/2Pp1B2/7Q/P4PPP/1R3RK1 w - - 0 1 ; bm b5a7",
  "2R5/2R4p/5p1k/6n1/8/1P2QPPq/r7/6K1 w - - 0 1 ; bm c7h7",
  "5r1k/1b4p1/p6p/4Pp1q/2pNnP2/7N/PPQ3PP/5R1K b - - 0 1 ; bm h5h3",
  "6k1/3r4/2R5/P5P1/1P4p1/8/4rB2/6K1 b - - 0 1 ; bm g4g3",
  "5n2/pRrk2p1/P4p1p/4p3/3N4/5P2/6PP/6K1 w - - 0 1 ; bm d4b5",
  "8/6pp/4p3/1p1n4/1NbkN1P1/P4P1P/1PR3K1/r7 w - - 0 1 ; bm c2c4",
  "2r5/2rk2pp/1pn1pb2/pN1p4/P2P4/1N2B3/nPR1KPPP/3R4 b - - 0 1 ; bm c6d4",
  "nrq4r/2k1p3/1p1pPnp1/pRpP1p2/P1P2P2/2P1BB2/1R2Q1P1/6K1 w - - 0 1 ; bm e3c5",
  "3r2k1/5p2/6p1/4b3/1P2P3/1R2P2p/P1K1N3/8 b - - 0 1 ; bm d8d1",
  "1k1r4/pp1r1pp1/4n1p1/2R5/2Pp1qP1/3P2QP/P4PB1/1R4K1 w - - 0 1 ; bm g2b7",
  "2r1k3/6pr/p1nBP3/1p3p1p/2q5/2P5/P1R4P/K2Q2R1 w - - 0 1 ; bm g1g7",
  "2b4k/p1b2p2/2p2q2/3p1PNp/3P2R1/3B4/P1Q2PKP/4r3 w - - 0 1 ; bm c2c6",
  "5bk1/1rQ4p/5pp1/2pP4/3n1PP1/7P/1q3BB1/4R1K1 w - - 0 1 ; bm d5d6",
  "rnbqkb1r/pppp1ppp/8/4P3/6n1/7P/PPPNPPP1/R1BQKBNR b KQkq - 0 1 ; bm g4e3"
};

// [Attacker][Captured] / [PNBRQK][pnbrqk]
constexpr int kMvv[6][6] = {
  { 10, 15, 15, 20, 25, 99 }, { 9, 14, 14, 19, 24, 99 }, { 9, 14, 14, 19, 24, 99 },
  {  8, 13, 13, 18, 23, 99 }, { 7, 12, 12, 17, 22, 99 }, { 6, 11, 11, 16, 21, 99 }
};

// Evaluation phases      ( P  N  B  R  Q  K )
constexpr int kPiece[6] = { 1, 3, 3, 5, 9, 0 }; // Must match MAX_PIECES !

// ( MG  EG ) -> ( P  N  B  R  Q  K )
constexpr int kPestoMaterial[2][6] = {
  { 82, 337, 365, 477, 1025, 0 },
  { 94, 281, 297, 512,  936, 0 }
};

// [Piece][Phase][Square]
constexpr int kPestoPsqt[6][2][64] = {
{{ -55, -54, -53, -52, -52, -53, -54, -55, // Pawn (MG)
   -35,  -1, -20, -23, -15,  24,  38, -22,
   -26,  -4,  -4, -10,   3,   3,  33, -12,
   -27,  -2,  -5,  12,  17,   6,  10, -25,
   -14,  13,   6,  21,  23,  12,  17, -23,
    -6,   7,  26,  31,  65,  56,  25, -20,
    98, 134,  61,  95,  68, 126,  34, -11,
     0,   0,   0,   0,   0,   0,   0,   0 },
 { -55, -54, -53, -52, -52, -53, -54, -55, // Pawn (EG)
    13,   8,   8,  10,  13,   0,   2,  -7,
     4,   7,  -6,   1,   0,  -5,  -1,  -8,
    13,   9,  -3,  -7,  -7,  -8,   3,  -1,
    32,  24,  13,   5,  -2,   4,  17,  17,
    94, 100,  85,  67,  56,  53,  82,  84,
   178, 173, 158, 134, 147, 132, 165, 187,
     0,   0,   0,   0,   0,   0,   0,   0 }},
{{-105, -21, -58, -33, -17, -28, -19, -23, // Knight (MG)
   -29, -53, -12,  -3,  -1,  18, -14, -19,
   -23,  -9,  12,  10,  19,  17,  25, -16,
   -13,   4,  16,  13,  28,  19,  21,  -8,
    -9,  17,  19,  53,  37,  69,  18,  22,
   -47,  60,  37,  65,  84, 129,  73,  44,
   -73, -41,  72,  36,  23,  62,   7, -17,
  -167, -89, -34, -49,  61, -97, -15,-107 },
 { -29, -51, -23, -15, -22, -18, -50, -64, // Knight (EG)
   -42, -20, -10,  -5,  -2, -20, -23, -44,
   -23,  -3,  -1,  15,  10,  -3, -20, -22,
   -18,  -6,  16,  25,  16,  17,   4, -18,
   -17,   3,  22,  22,  22,  11,   8, -18,
   -24, -20,  10,   9,  -1,  -9, -19, -41,
   -25,  -8, -25,  -2,  -9, -25, -24, -52,
   -58, -38, -13, -28, -31, -27, -63, -99 }},
{{ -33,  -3, -14, -21, -13, -12, -39, -21, // Bishop (MG)
     4,  15,  16,   0,   7,  21,  33,   1,
     0,  15,  15,  15,  14,  27,  18,  10,
    -6,  13,  13,  26,  34,  12,  10,   4,
    -4,   5,  19,  50,  37,  37,   7,  -2,
   -16,  37,  43,  40,  35,  50,  37,  -2,
   -26,  16, -18, -13,  30,  59,  18, -47,
   -29,   4, -82, -37, -25, -42,   7,  -8 },
 { -23,  -9, -23,  -5,  -9, -16,  -5, -17, // Bishop (EG)
   -14, -18,  -7,  -1,   4,  -9, -15, -27,
   -12,  -3,   8,  10,  13,   3,  -7, -15,
    -6,   3,  13,  19,   7,  10,  -3,  -9,
    -3,   9,  12,   9,  14,  10,   3,   2,
     2,  -8,   0,  -1,  -2,   6,   0,   4,
    -8,  -4,   7, -12,  -3, -13,  -4, -14,
   -14, -21, -11,  -8,  -7,  -9, -17, -24 }},
{{ -19, -13,   1,  17,  16,   7, -37, -26, // Rook (MG)
   -44, -16, -20,  -9,  -1,  11,  -6, -71,
   -45, -25, -16, -17,   3,   0,  -5, -33,
   -36, -26, -12,  -1,   9,  -7,   6, -23,
   -24, -11,   7,  26,  24,  35,  -8, -20,
    -5,  19,  26,  36,  17,  45,  61,  16,
    27,  32,  58,  62,  80,  67,  26,  44,
    32,  42,  32,  51,  63,   9,  31,  43 },
  { -9,   2,   3,  -1,  -5, -13,   4, -20, // Rook (EG)
    -6,  -6,   0,   2,  -9,  -9, -11,  -3,
    -4,   0,  -5,  -1,  -7, -12,  -8, -16,
     3,   5,   8,   4,  -5,  -6,  -8, -11,
     4,   3,  13,   1,   2,   1,  -1,   2,
     7,   7,   7,   5,   4,  -3,  -5,  -3,
    11,  13,  13,  11,  -3,   3,   8,   3,
    13,  10,  18,  15,  12,  12,   8,   5 }},
{{  -1, -18,  -9,  10, -15, -25, -31, -50, // Queen (MG)
   -35,  -8,  11,   2,   8,  15,  -3,   1,
   -14,   2, -11,  -2,  -5,   2,  14,   5,
    -9, -26,  -9, -10,  -2,  -4,   3,  -3,
   -27, -27, -16, -16,  -1,  17,  -2,   1,
   -13, -17,   7,   8,  29,  56,  47,  57,
   -24, -39,  -5,   1, -16,  57,  28,  54,
   -28,   0,  29,  12,  59,  44,  43,  45 },
 { -33, -28, -22, -43,  -5, -32, -20, -41, // Queen (EG)
   -22, -23, -30, -16, -16, -23, -36, -32,
   -16, -27,  15,   6,   9,  17,  10,   5,
   -18,  28,  19,  47,  31,  34,  39,  23,
     3,  22,  24,  45,  57,  40,  57,  36,
   -20,   6,   9,  49,  47,  35,  19,   9,
   -17,  20,  32,  41,  58,  25,  30,   0,
    -9,  22,  22,  27,  27,  19,  10,  20 }},
{{ -15,  36,  12, -54,   8, -28,  24,  14, // King (MG)
     1,   7,  -8, -64, -43, -16,   9,   8,
   -14, -14, -22, -46, -44, -30, -15, -27,
   -49,  -1, -27, -39, -46, -44, -33, -51,
   -17, -20, -12, -27, -30, -25, -14, -36,
    -9,  24,   2, -16, -20,   6,  22, -22,
    29,  -1, -20,  -7,  -8,  -4, -38, -29,
   -65,  23,  16, -15, -56, -34,   2,  13 },
 { -53, -34, -21, -11, -28, -14, -24, -43, // King (EG)
   -27, -11,   4,  13,  14,   4,  -5, -17,
   -19,  -3,  11,  21,  23,  16,   7,  -9,
   -18,  -4,  21,  24,  27,  23,   9, -11,
    -8,  22,  24,  27,  26,  33,  26,   3,
    10,  17,  23,  15,  20,  45,  44,  13,
   -12,  17,  14,  17,  17,  38,  23,  11,
   -74, -35, -18, -18, -11,  15,   4, -17 }}
};

constexpr std::uint64_t kRookMagics[3][64] = {
  { 0x548001400080106cULL, 0x900184000110820ULL,  0x428004200a81080ULL,  0x140088082000c40ULL, // Magics
    0x1480020800011400ULL, 0x100008804085201ULL,  0x2a40220001048140ULL, 0x50000810000482aULL,
    0x250020100020a004ULL, 0x3101880100900a00ULL, 0x200a040a00082002ULL, 0x1004300044032084ULL,
    0x2100408001013ULL,    0x21f00440122083ULL,   0xa204280406023040ULL, 0x2241801020800041ULL,
    0xe10100800208004ULL,  0x2010401410080ULL,    0x181482000208805ULL,  0x4080101000021c00ULL,
    0xa250210012080022ULL, 0x4210641044000827ULL, 0x8081a02300d4010ULL,  0x8008012000410001ULL,
    0x28c0822120108100ULL, 0x500160020aa005ULL,   0xc11050088c1000ULL,   0x48c00101000a288ULL,
    0x494a184408028200ULL, 0x20880100240006ULL,   0x10b4010200081ULL,    0x40a200260000490cULL,
    0x22384003800050ULL,   0x7102001a008010ULL,   0x80020c8010900c0ULL,  0x100204082a001060ULL,
    0x8000118188800428ULL, 0x58e0020009140244ULL, 0x100145040040188dULL, 0x44120220400980ULL,
    0x114001007a00800ULL,  0x80a0100516304000ULL, 0x7200301488001000ULL, 0x1000151040808018ULL,
    0x3000a200010e0020ULL, 0x1000849180802810ULL, 0x829100210208080ULL,  0x1004050021528004ULL,
    0x61482000c41820b0ULL, 0x241001018a401a4ULL,  0x45020c009cc04040ULL, 0x308210c020081200ULL,
    0xa000215040040ULL,    0x10a6024001928700ULL, 0x42c204800c804408ULL, 0x30441a28614200ULL,
    0x40100229080420aULL,  0x9801084000201103ULL, 0x8408622090484202ULL, 0x4022001048a0e2ULL,
    0x280120020049902ULL,  0x1200412602009402ULL, 0x914900048020884ULL,  0x104824281002402ULL },
  { 0x101010101017eULL,    0x202020202027cULL,    0x404040404047aULL,    0x8080808080876ULL, // Masks
    0x1010101010106eULL,   0x2020202020205eULL,   0x4040404040403eULL,   0x8080808080807eULL,
    0x1010101017e00ULL,    0x2020202027c00ULL,    0x4040404047a00ULL,    0x8080808087600ULL,
    0x10101010106e00ULL,   0x20202020205e00ULL,   0x40404040403e00ULL,   0x80808080807e00ULL,
    0x10101017e0100ULL,    0x20202027c0200ULL,    0x40404047a0400ULL,    0x8080808760800ULL,
    0x101010106e1000ULL,   0x202020205e2000ULL,   0x404040403e4000ULL,   0x808080807e8000ULL,
    0x101017e010100ULL,    0x202027c020200ULL,    0x404047a040400ULL,    0x8080876080800ULL,
    0x1010106e101000ULL,   0x2020205e202000ULL,   0x4040403e404000ULL,   0x8080807e808000ULL,
    0x1017e01010100ULL,    0x2027c02020200ULL,    0x4047a04040400ULL,    0x8087608080800ULL,
    0x10106e10101000ULL,   0x20205e20202000ULL,   0x40403e40404000ULL,   0x80807e80808000ULL,
    0x17e0101010100ULL,    0x27c0202020200ULL,    0x47a0404040400ULL,    0x8760808080800ULL,
    0x106e1010101000ULL,   0x205e2020202000ULL,   0x403e4040404000ULL,   0x807e8080808000ULL,
    0x7e010101010100ULL,   0x7c020202020200ULL,   0x7a040404040400ULL,   0x76080808080800ULL,
    0x6e101010101000ULL,   0x5e202020202000ULL,   0x3e404040404000ULL,   0x7e808080808000ULL,
    0x7e01010101010100ULL, 0x7c02020202020200ULL, 0x7a04040404040400ULL, 0x7608080808080800ULL,
    0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL },
  { 0x101010101017eULL,    0x202020202027cULL,    0x404040404047aULL,    0x8080808080876ULL, // Moves
    0x1010101010106eULL,   0x2020202020205eULL,   0x4040404040403eULL,   0x8080808080807eULL,
    0x1010101017e00ULL,    0x2020202027c00ULL,    0x4040404047a00ULL,    0x8080808087600ULL,
    0x10101010106e00ULL,   0x20202020205e00ULL,   0x40404040403e00ULL,   0x80808080807e00ULL,
    0x10101017e0100ULL,    0x20202027c0200ULL,    0x40404047a0400ULL,    0x8080808760800ULL,
    0x101010106e1000ULL,   0x202020205e2000ULL,   0x404040403e4000ULL,   0x808080807e8000ULL,
    0x101017e010100ULL,    0x202027c020200ULL,    0x404047a040400ULL,    0x8080876080800ULL,
    0x1010106e101000ULL,   0x2020205e202000ULL,   0x4040403e404000ULL,   0x8080807e808000ULL,
    0x1017e01010100ULL,    0x2027c02020200ULL,    0x4047a04040400ULL,    0x8087608080800ULL,
    0x10106e10101000ULL,   0x20205e20202000ULL,   0x40403e40404000ULL,   0x80807e80808000ULL,
    0x17e0101010100ULL,    0x27c0202020200ULL,    0x47a0404040400ULL,    0x8760808080800ULL,
    0x106e1010101000ULL,   0x205e2020202000ULL,   0x403e4040404000ULL,   0x807e8080808000ULL,
    0x7e010101010100ULL,   0x7c020202020200ULL,   0x7a040404040400ULL,   0x76080808080800ULL,
    0x6e101010101000ULL,   0x5e202020202000ULL,   0x3e404040404000ULL,   0x7e808080808000ULL,
    0x7e01010101010100ULL, 0x7c02020202020200ULL, 0x7a04040404040400ULL, 0x7608080808080800ULL,
    0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL }
};

constexpr std::uint64_t kBishopMagics[3][64] = {
  { 0x2890208600480830ULL, 0x324148050f087ULL,    0x1402488a86402004ULL, 0xc2210a1100044bULL, // Magics
    0x88450040b021110cULL, 0xc0407240011ULL,      0xd0246940cc101681ULL, 0x1022840c2e410060ULL,
    0x4a1804309028d00bULL, 0x821880304a2c0ULL,    0x134088090100280ULL,  0x8102183814c0208ULL,
    0x518598604083202ULL,  0x67104040408690ULL,   0x1010040020d000ULL,   0x600001028911902ULL,
    0x8810183800c504c4ULL, 0x2628200121054640ULL, 0x28003000102006ULL,   0x4100c204842244ULL,
    0x1221c50102421430ULL, 0x80109046e0844002ULL, 0xc128600019010400ULL, 0x812218030404c38ULL,
    0x1224152461091c00ULL, 0x1c820008124000aULL,  0xa004868015010400ULL, 0x34c080004202040ULL,
    0x200100312100c001ULL, 0x4030048118314100ULL, 0x410000090018ULL,     0x142c010480801ULL,
    0x8080841c1d004262ULL, 0x81440f004060406ULL,  0x400a090008202ULL,    0x2204020084280080ULL,
    0xb820060400008028ULL, 0x110041840112010ULL,  0x8002080a1c84400ULL,  0x212100111040204aULL,
    0x9412118200481012ULL, 0x804105002001444cULL, 0x103001280823000ULL,  0x40088e028080300ULL,
    0x51020d8080246601ULL, 0x4a0a100e0804502aULL, 0x5042028328010ULL,    0xe000808180020200ULL,
    0x1002020620608101ULL, 0x1108300804090c00ULL, 0x180404848840841ULL,  0x100180040ac80040ULL,
    0x20840000c1424001ULL, 0x82c00400108800ULL,   0x28c0493811082aULL,   0x214980910400080cULL,
    0x8d1a0210b0c000ULL,   0x164c500ca0410cULL,   0xc6040804283004ULL,   0x14808001a040400ULL,
    0x180450800222a011ULL, 0x600014600490202ULL,  0x21040100d903ULL,     0x10404821000420ULL },
  { 0x40201008040200ULL,   0x402010080400ULL,     0x4020100a00ULL,       0x40221400ULL, // Masks
    0x2442800ULL,          0x204085000ULL,        0x20408102000ULL,      0x2040810204000ULL,
    0x20100804020000ULL,   0x40201008040000ULL,   0x4020100a0000ULL,     0x4022140000ULL,
    0x244280000ULL,        0x20408500000ULL,      0x2040810200000ULL,    0x4081020400000ULL,
    0x10080402000200ULL,   0x20100804000400ULL,   0x4020100a000a00ULL,   0x402214001400ULL,
    0x24428002800ULL,      0x2040850005000ULL,    0x4081020002000ULL,    0x8102040004000ULL,
    0x8040200020400ULL,    0x10080400040800ULL,   0x20100a000a1000ULL,   0x40221400142200ULL,
    0x2442800284400ULL,    0x4085000500800ULL,    0x8102000201000ULL,    0x10204000402000ULL,
    0x4020002040800ULL,    0x8040004081000ULL,    0x100a000a102000ULL,   0x22140014224000ULL,
    0x44280028440200ULL,   0x8500050080400ULL,    0x10200020100800ULL,   0x20400040201000ULL,
    0x2000204081000ULL,    0x4000408102000ULL,    0xa000a10204000ULL,    0x14001422400000ULL,
    0x28002844020000ULL,   0x50005008040200ULL,   0x20002010080400ULL,   0x40004020100800ULL,
    0x20408102000ULL,      0x40810204000ULL,      0xa1020400000ULL,      0x142240000000ULL,
    0x284402000000ULL,     0x500804020000ULL,     0x201008040200ULL,     0x402010080400ULL,
    0x2040810204000ULL,    0x4081020400000ULL,    0xa102040000000ULL,    0x14224000000000ULL,
    0x28440200000000ULL,   0x50080402000000ULL,   0x20100804020000ULL,   0x40201008040200ULL },
  { 0x40201008040200ULL,   0x402010080400ULL,     0x4020100a00ULL,       0x40221400ULL, // Moves
    0x2442800ULL,          0x204085000ULL,        0x20408102000ULL,      0x2040810204000ULL,
    0x20100804020000ULL,   0x40201008040000ULL,   0x4020100a0000ULL,     0x4022140000ULL,
    0x244280000ULL,        0x20408500000ULL,      0x2040810200000ULL,    0x4081020400000ULL,
    0x10080402000200ULL,   0x20100804000400ULL,   0x4020100a000a00ULL,   0x402214001400ULL,
    0x24428002800ULL,      0x2040850005000ULL,    0x4081020002000ULL,    0x8102040004000ULL,
    0x8040200020400ULL,    0x10080400040800ULL,   0x20100a000a1000ULL,   0x40221400142200ULL,
    0x2442800284400ULL,    0x4085000500800ULL,    0x8102000201000ULL,    0x10204000402000ULL,
    0x4020002040800ULL,    0x8040004081000ULL,    0x100a000a102000ULL,   0x22140014224000ULL,
    0x44280028440200ULL,   0x8500050080400ULL,    0x10200020100800ULL,   0x20400040201000ULL,
    0x2000204081000ULL,    0x4000408102000ULL,    0xa000a10204000ULL,    0x14001422400000ULL,
    0x28002844020000ULL,   0x50005008040200ULL,   0x20002010080400ULL,   0x40004020100800ULL,
    0x20408102000ULL,      0x40810204000ULL,      0xa1020400000ULL,      0x142240000000ULL,
    0x284402000000ULL,     0x500804020000ULL,     0x201008040200ULL,     0x402010080400ULL,
    0x2040810204000ULL,    0x4081020400000ULL,    0xa102040000000ULL,    0x14224000000000ULL,
    0x28440200000000ULL,   0x50080402000000ULL,   0x20100804020000ULL,   0x40201008040200ULL }
};

// Enums

enum class MoveType { kKiller, kGood };

// Structs

struct Board { // 172B
  std::uint64_t white[6]{};   // White bitboards
  std::uint64_t black[6]{};   // Black bitboards
  std::int32_t  score{0};     // Sorting score
  std::int8_t   pieces[64]{}; // Pieces white and black
  std::int8_t   epsq{-1};     // En passant square
  std::uint8_t  index{0};     // Sorting index
  std::uint8_t  from{0};      // From square
  std::uint8_t  to{0};        // To square
  std::uint8_t  type{0};      // Move type ( 0:Normal 1:OOw 2:OOOw 3:OOb 4:OOOb 5:=n 6:=b 7:=r 8:=q )
  std::uint8_t  castle{0};    // Castling rights ( 0x1:K 0x2:Q 0x4:k 0x8:q )
  std::uint8_t  fifty{0};     // Rule 50 counter ( 256 max )

  bool is_underpromo() const;
  bool is_queen_promo() const;
  bool is_castling() const;
  const std::string movename() const;
  const std::string to_fen() const;
  const std::string to_s() const;
};

struct HashEntry { // 10B
  std::uint32_t killer_hash{0}; // Killer move hash
  std::uint32_t good_hash{0};   // Good move hash
  std::uint8_t  killer{0};      // Killer move index
  std::uint8_t  good{0};        // Good move index

  template <MoveType> void update(const std::uint64_t, const std::uint8_t);
  void put_hash_value_to_moves(const std::uint64_t, Board*) const;
};

// Variables

std::uint64_t g_black = 0, g_white = 0, g_both = 0, g_empty = 0, g_good = 0, g_stop_search_time = 0,
  g_nodes = 0, g_pawn_sq = 0, g_pawn_1_moves_w[64]{}, g_pawn_1_moves_b[64]{}, g_pawn_2_moves_w[64]{},
  g_pawn_2_moves_b[64]{}, g_knight_moves[64]{}, g_king_moves[64]{}, g_pawn_checks_w[64]{}, g_pawn_checks_b[64]{},
  g_castle_w[2]{}, g_castle_b[2]{}, g_castle_empty_w[2]{}, g_castle_empty_b[2]{}, g_bishop_magic_moves[64][512]{},
  g_rook_magic_moves[64][4096]{}, g_zobrist_ep[64]{}, g_zobrist_castle[16]{}, g_zobrist_wtm[2]{},
  g_r50_positions[R50_ARR]{}, g_zobrist_board[13][64]{};

int g_move_overhead = MOVEOVERHEAD, g_level = 100, g_root_n = 0, g_king_w = 0, g_king_b = 0, g_moves_n = 0,
  g_max_depth = MAX_SEARCH_DEPTH, g_q_depth = 0, g_depth = 0, g_best_score = 0, g_noise = NOISE, g_last_eval = 0,
  g_fullmoves = 1, g_rook_w[2]{}, g_rook_b[2]{}, g_nnue_pieces[64]{}, g_nnue_squares[64]{};

bool g_chess960 = false, g_wtm = false, g_underpromos = true, g_nullmove_active = false,
  g_stop_search = false, g_is_pv = false, g_book_exist = false, g_nnue_exist = false,
  g_classical = true, g_game_on = true, g_analyzing = false;

Board g_board_empty{}, *g_board = &g_board_empty, *g_moves = nullptr, *g_board_orig = nullptr,
  g_boards[MAX_SEARCH_DEPTH + MAX_Q_SEARCH_DEPTH][MAX_MOVES]{};

std::uint32_t g_hash_entries = 0, g_tokens_nth = 0;
std::vector<std::string> g_tokens(300); // 300 plys init
polyglotbook::PolyglotBook g_book{};
std::unique_ptr<HashEntry[]> g_hash{};

// Prototypes

int SearchW(int, const int, const int, const int);
int SearchB(const int, int, const int, const int);
int QSearchB(const int, int, const int, const int);
int Evaluate(const bool);
bool ChecksW();
bool ChecksB();
std::uint64_t GetRookMagicMoves(const int, const std::uint64_t);
std::uint64_t GetBishopMagicMoves(const int, const std::uint64_t);

// Utils

// White bitboards
inline std::uint64_t White() {
  return g_board->white[0] | g_board->white[1] | g_board->white[2] |
         g_board->white[3] | g_board->white[4] | g_board->white[5];
}

// Black bitboards
inline std::uint64_t Black() {
  return g_board->black[0] | g_board->black[1] | g_board->black[2] |
         g_board->black[3] | g_board->black[4] | g_board->black[5];
}

// Both colors
inline std::uint64_t Both() {
  return White() | Black();
}

// Set bit in 1 -> 64
inline std::uint64_t Bit(const int nth) {
  return 0x1ULL << nth;
}

// Count rightmost zeros AND then pop BitBoard
inline int CtzrPop(std::uint64_t *b) {
  const int ret = std::countr_zero(*b); // 1011000 -> 3
  *b &= *b - 0x1ULL;
  return ret;
}

// X axle of board
inline int MakeX(const int sq) {
  return int(sq % 8);
}

// Y axle of board
inline int MakeY(const int sq) {
  return int(sq / 8);
}

// Nodes Per Second
std::uint64_t Nps(const std::uint64_t nodes, const std::uint64_t ms) {
  return static_cast<std::uint64_t>(1000 * nodes) / std::max<std::uint64_t>(1, ms);
}

// Is (x, y) on board ? Slow, but only for init
bool IsOnBoard(const int x, const int y) {
  return x >= 0 && x <= 7 && y >= 0 && y <= 7;
}

// X-coord to char
char MakeFile2Char(const int f) {
  switch (f) {
    case 0:  return 'a';
    case 1:  return 'b';
    case 2:  return 'c';
    case 3:  return 'd';
    case 4:  return 'e';
    case 5:  return 'f';
    case 6:  return 'g';
    default: return 'h';
  }
}

// Y-coord to char
char MakeRank2Char(const int r) {
  switch (r) {
    case 0:  return '1';
    case 1:  return '2';
    case 2:  return '3';
    case 3:  return '4';
    case 4:  return '5';
    case 5:  return '6';
    case 6:  return '7';
    default: return '8';
  }
}

// Convert int coords to string
const std::string MakeMove2Str(const int from, const int to) {
  return std::string{MakeFile2Char(MakeX(from)), MakeRank2Char(MakeY(from)),
                     MakeFile2Char(MakeX(to)),   MakeRank2Char(MakeY(to))};
}

extern "C" {
bool IsInputAvailable() { // See if std::cin has smt
#ifdef WINDOWS
  return _kbhit();
#else
  fd_set fd{};
  struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
  FD_ZERO(&fd);
  FD_SET(STDIN_FILENO, &fd);
  select(STDIN_FILENO + 1, &fd, nullptr, nullptr, &tv);
  return FD_ISSET(STDIN_FILENO, &fd) > 0;
#endif
}
} // extern "C"

// ms since 1.1.1970
std::uint64_t Now(const std::uint64_t ms = 0) {
  return ms +
    std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()) .count();
}

std::uint64_t Mixer(const std::uint64_t num) {
  return ((num) << 7) ^ ((num) >> 5);
}

// Deterministic Random()
std::uint64_t Random64() {
  static std::uint64_t a = 0X12311227ULL, b = 0X1931311ULL, c = 0X13138141ULL;
  a ^= b + c;
  b ^= b * c + 0x1717711ULL;
  c  = (3 * c) + 0x1ULL;
  return Mixer(a) ^ Mixer(b) ^ Mixer(c);
}

// 8x deterministic random for zobrist
std::uint64_t Random8x64() {
  std::uint64_t ret = 0;
  for (std::size_t i = 0; i < 8; i += 1) ret ^= Random64() << (8 * i);
  return ret;
}

// Nondeterministic Rand()
int Random(const int min, const int max) {
  static std::uint64_t seed = 0x202c7ULL + static_cast<std::uint64_t>(std::time(nullptr));
  if (min == max) return min;
  if (min > max) return Random(max, min);
  seed = (seed << 5) ^ (seed + 1) ^ (seed >> 3);
  return min + static_cast<int>(seed % static_cast<std::uint64_t>(std::abs(max - min) + 1));
}

// Split string by given str
template <class T>
void SplitString(const std::string &str, T &cont, const std::string &delims = " ") {
  std::size_t cur = str.find_first_of(delims), prev = 0;
  while (cur != std::string::npos) {
    cont.push_back(str.substr(prev, cur - prev));
    prev = cur + 1;
    cur  = str.find_first_of(delims, prev);
  }
  cont.push_back(str.substr(prev, cur - prev));
}

// Flips the fen and bm. Fully legal FEN expected.
// 8/5P1p/6kr/7p/7P/5K2/8/8 w - - 0 1 ; bm f7f8b
// -->
// 8/8/5k2/7p/7P/6KR/5p1P/8 b - - 0 1 ; bm f2f1b
const std::string FlipFen(const std::string &fen) {
  std::string s{};
  const std::string num      = "12345678";
  const std::string num_flip = "87654321";
  const std::string small    = "pnbrqkwacdefgh";
  const std::string upper    = "PNBRQKWACDEFGH";
  bool only_number           = false;
  int empty                  = 0;
  for (std::size_t i = 0; i < fen.size(); i += 1) {
    if (fen[i] == ' ') {
      empty += 1;
      if (empty == 1) {
        std::vector<std::string> pieces{};
        SplitString< std::vector< std::string> >(s, pieces, "/");
        if (pieces.size() != 8) throw std::runtime_error("info string ( #1 ) Bad fen: " + fen);
        s = "";
        for (std::size_t k = 0; k < 8; k += 1) {
          s += pieces[7 - k];
          if (k <= 6) s += '/';
        }
      }
    }
    if (fen[i] == ';') only_number = true;
    if (small.find(fen[i]) != std::string::npos && !only_number) {
      s += empty == 1 ? (fen[i] == 'w' ? 'b' : 'w') : static_cast<char>(std::toupper(fen[i]));
    } else if (upper.find(fen[i]) != std::string::npos && !only_number) {
      s += static_cast<char>(std::tolower(fen[i]));
    } else {
      const std::size_t j = num.find(fen[i]);
      s += (j != std::string::npos && (empty == 3 || empty == 8)) ? num_flip[j] : fen[i];
    }
  }
  return s;
}

// Read input from std::cin
void ReadInput() {
  std::string line{};
  std::getline(std::cin, line);
  g_tokens_nth = 0;
  g_tokens.clear();
  SplitString< std::vector<std::string> >(line, g_tokens);
}

// PolyGlot Book lib

void SetBook(const std::string &book_file = BOOK_FILE) {
  g_book_exist = USE_BOOK && (book_file.length() <= 1 ? false : g_book.open_book(book_file));
}

// NNUE lib

void SetNNUE(const std::string &eval_file = EVAL_FILE) {
  g_classical = USE_NNUE && (!(g_nnue_exist = eval_file.length() <= 1 ? false : nnue::nnue_init(eval_file.c_str())));
}

// Hashtable

void SetHashtable(int hash_mb = DEF_HASH_MB) {
  hash_mb = std::clamp(hash_mb, 1, 1048576); // Limits 1MB -> 1TB
  g_hash_entries = static_cast<std::uint32_t>((1 << 20) * hash_mb) / (sizeof(HashEntry)); // Hash(B) / Block(B)
  g_hash.reset(new HashEntry[g_hash_entries]); // Claim space
}

// Hash

std::uint64_t Hash(const bool wtm) {
  std::uint64_t ret = g_zobrist_ep[g_board->epsq + 1] ^
                      g_zobrist_wtm[wtm] ^
                      g_zobrist_castle[g_board->castle];
  for (auto both = Both(); both; ) {
    const auto sq = CtzrPop(&both);
    ret ^= g_zobrist_board[g_board->pieces[sq] + 6][sq];
  }
  return ret;
}

// HashEntry

// Update hashtable sorting algorithm
template <MoveType type>
void HashEntry::update(const std::uint64_t hash, const std::uint8_t index) {
  if constexpr (type == MoveType::kKiller) {
    this->killer_hash = static_cast<std::uint32_t>(hash >> 32);
    this->killer      = index + 1;
  } else { // == MoveType::kGood !
    this->good_hash = static_cast<std::uint32_t>(hash >> 32);
    this->good      = index + 1;
  }
}

// Best moves put first for maximum cutoffs
void HashEntry::put_hash_value_to_moves(const std::uint64_t hash, Board *moves) const {
  if (this->killer && (this->killer_hash == static_cast<std::uint32_t>(hash >> 32)))
    moves[this->killer - 1].score += 10000;
  if (this->good && (this->good_hash == static_cast<std::uint32_t>(hash >> 32)))
    moves[this->good - 1].score += 7000;
}

// Board

bool Board::is_queen_promo() const {
  return this->type == 8; // e7e8q
}

bool Board::is_castling() const {
  switch (this->type) {
    case 1:  return true; // O-Ow
    case 2:  return true; // O-O-Ow
    case 3:  return true; // O-Ob
    case 4:  return true; // O-O-Ob
    default: return false;
  }
}

bool Board::is_underpromo() const {
  switch (this->type) {
    case 5:  return true; // e7e8n
    case 6:  return true; // e7e8n
    case 7:  return true; // e7e8r
    default: return false;
  }
}

const std::string Board::movename() const {
  switch (this->type) {
    case 1:  return MakeMove2Str(g_king_w, g_chess960 ? g_rook_w[0] : 6);      // O-Ow
    case 2:  return MakeMove2Str(g_king_w, g_chess960 ? g_rook_w[1] : 2);      // O-O-Ow
    case 3:  return MakeMove2Str(g_king_b, g_chess960 ? g_rook_b[0] : 56 + 6); // O-Ob
    case 4:  return MakeMove2Str(g_king_b, g_chess960 ? g_rook_b[1] : 56 + 2); // O-O-Ob
    case 5:  return MakeMove2Str(this->from, this->to) + 'n'; // e7e8n
    case 6:  return MakeMove2Str(this->from, this->to) + 'b'; // e7e8b
    case 7:  return MakeMove2Str(this->from, this->to) + 'r'; // e7e8r
    case 8:  return MakeMove2Str(this->from, this->to) + 'q'; // e7e8q
    default: return MakeMove2Str(this->from, this->to); // Normal
  }
}

char GetPiece(const int piece) {
  switch (piece) {
    case +1: return 'P';
    case +2: return 'N';
    case +3: return 'B';
    case +4: return 'R';
    case +5: return 'Q';
    case +6: return 'K';
    case -1: return 'p';
    case -2: return 'n';
    case -3: return 'b';
    case -4: return 'r';
    case -5: return 'q';
    case -6: return 'k';
    default: return '.';
  }
}

char GetCastleFile(const int file) {
  switch (file) {
    case 0:  return 'A';
    case 1:  return 'B';
    case 2:  return 'C';
    case 3:  return 'D';
    case 4:  return 'E';
    case 5:  return 'F';
    case 6:  return 'G';
    case 7:  return 'H';
    case 56: return 'a';
    case 57: return 'b';
    case 58: return 'c';
    case 59: return 'd';
    case 60: return 'e';
    case 61: return 'f';
    case 62: return 'g';
    case 63: return 'h';
    default: return '.';
  }
}

// Board presentation in FEN ( Forsyth–Edwards Notation )
const std::string Board::to_fen() const {
  std::stringstream s{};
  for (auto r = 7; r >= 0; r -= 1) {
    auto empty = 0;
    for (auto f = 0; f <= 7; f += 1)
      if (const auto p = GetPiece(this->pieces[8 * r + f]); p == '.') {
        empty += 1;
      } else {
        if (empty) {
          s << empty;
          empty = 0;
        }
        s << p;
      }
    if (empty)  s << empty;
    if (r != 0) s << "/";
  }
  s << (g_wtm ? " w " : " b ");
  if (this->castle & 0x1) s << GetCastleFile(g_rook_w[0]);
  if (this->castle & 0x2) s << GetCastleFile(g_rook_w[1]);
  if (this->castle & 0x4) s << GetCastleFile(g_rook_b[0]);
  if (this->castle & 0x8) s << GetCastleFile(g_rook_b[1]);
  s << (this->castle ? " " : "- ");
  if (this->epsq == -1)
    s << "-";
  else
    s << MakeFile2Char(MakeX(this->epsq)) << MakeRank2Char(MakeY(this->epsq));
  s << " " << static_cast<int>(this->fifty) << " " << static_cast<int>(std::max(1, g_fullmoves));
  return s.str();
}

// String presentation of board
const std::string Board::to_s() const {
  std::stringstream s{};
  s << " +---+---+---+---+---+---+---+---+\n";
  for (auto r = 7; r >= 0; r -= 1) {
    for (auto f = 0; f <= 7; f += 1)
      s << " | " << GetPiece(this->pieces[8 * r + f]);
    s << " | " << (1 + r) << "\n +---+---+---+---+---+---+---+---+\n";
  }
  s << "   a   b   c   d   e   f   g   h\n\n" <<
    "> " << this->to_fen() << '\n' <<
    "> Eval: " << std::showpos << Evaluate(g_wtm) << std::noshowpos << " | " <<
    "NNUE: " << (g_nnue_exist ? "OK" : "FAIL") << " | " <<
    "Book: " << (g_book_exist ? "OK" : "FAIL");
  return s.str();
}

// Tokenizer

bool TokenIsOk(const std::uint32_t nth = 0) {
  return g_tokens_nth + nth < g_tokens.size(); // O(1)
}

const std::string TokenGetNth(const std::uint32_t nth = 0) {
  return TokenIsOk(nth) ? g_tokens[g_tokens_nth + nth] : "";
}

void TokenPop(const std::uint32_t nth = 1) {
  g_tokens_nth += nth;
}

bool TokenPeek(const std::string &token, const std::uint32_t nth = 0) {
  return TokenIsOk(nth) && token == g_tokens[g_tokens_nth + nth];
}

int TokenGetNumber(const std::uint32_t nth = 0) {
  return TokenIsOk(nth) ? std::stoi(g_tokens[g_tokens_nth + nth]) : 0;
}

// If true then pop n
bool Token(const std::string &token, const std::uint32_t pop_n = 1) {
  if (!TokenPeek(token)) return false;
  TokenPop(pop_n);
  return true;
}

// Fen handling

std::uint64_t FenFill(int from, const int to) { // from / to -> Always good
  auto ret = Bit(from); // Build filled bitboard
  if (from == to) return ret;
  const auto diff = from > to ? -1 : +1;
  do { from += diff; ret |= Bit(from); } while (from != to);
  return ret;
}

// White: O-O
void FenBuildCastlingBitboard_O_O_W() {
  if (!(g_board->castle & 0x1)) return;
  g_castle_w[0]       = FenFill(g_king_w, 6);
  g_castle_empty_w[0] = (g_castle_w[0] | FenFill(g_rook_w[0], 5)) ^ (Bit(g_king_w) | Bit(g_rook_w[0]));
}

// White: O-O-O
void FenBuildCastlingBitboard_O_O_O_W() {
  if (!(g_board->castle & 0x2)) return;
  g_castle_w[1]       = FenFill(g_king_w, 2);
  g_castle_empty_w[1] = (g_castle_w[1] | FenFill(g_rook_w[1], 3)) ^ (Bit(g_king_w) | Bit(g_rook_w[1]));
}

// Black: O-O
void FenBuildCastlingBitboard_O_O_B() {
  if (!(g_board->castle & 0x4)) return;
  g_castle_b[0]       = FenFill(g_king_b, 56 + 6);
  g_castle_empty_b[0] = (g_castle_b[0] | FenFill(g_rook_b[0], 56 + 5)) ^ (Bit(g_king_b) | Bit(g_rook_b[0]));
}

// Black: O-O-O
void FenBuildCastlingBitboard_O_O_O_B() {
  if (!(g_board->castle & 0x8)) return;
  g_castle_b[1]       = FenFill(g_king_b, 56 + 2);
  g_castle_empty_b[1] = (g_castle_b[1] | FenFill(g_rook_b[1], 56 + 3)) ^ (Bit(g_king_b) | Bit(g_rook_b[1]));
}

void FenCheckCastlingBitboards() {
  for (const std::size_t i : {0, 1}) {
    g_castle_empty_w[i] &= 0xFFULL;
    g_castle_empty_b[i] &= 0xFF00000000000000ULL;
    g_castle_w[i]       &= 0xFFULL;
    g_castle_b[i]       &= 0xFF00000000000000ULL;
  }
}

void FenBuildCastlingBitboards() {
  FenBuildCastlingBitboard_O_O_W();
  FenBuildCastlingBitboard_O_O_O_W();
  FenBuildCastlingBitboard_O_O_B();
  FenBuildCastlingBitboard_O_O_O_B();
  FenCheckCastlingBitboards();
}

void FenFindKings(const int sq, const int piece) {
  switch (piece) {
    case +6: g_king_w = sq; break; // K
    case -6: g_king_b = sq; break; // k
  }
}

void FenPutPieceOnBoard(const int sq, const int piece) {
  g_board->pieces[sq] = piece;
}

void FenCreatePieceBitboards(const int sq, const int piece) {
  if      (piece > 0) g_board->white[+piece - 1] |= Bit(sq);
  else if (piece < 0) g_board->black[-piece - 1] |= Bit(sq);
}

void FenPutPiece(const int sq, const int piece) {
  FenFindKings(sq, piece);
  FenPutPieceOnBoard(sq, piece);
  FenCreatePieceBitboards(sq, piece);
}

int FenMakePiece2Num(const char p) {
  switch (p) {
    case 'P': return +1;
    case 'N': return +2;
    case 'B': return +3;
    case 'R': return +4;
    case 'Q': return +5;
    case 'K': return +6;
    case 'p': return -1;
    case 'n': return -2;
    case 'b': return -3;
    case 'r': return -4;
    case 'q': return -5;
    case 'k': return -6;
    default:  return  0;
  }
}

int FenMakeChar2Num(const char e) {
  switch (e) {
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    default:  return 0;
  }
}

int FenMakeFile2Num(const char f) {
  switch (f) {
    case 'a': return 0;
    case 'b': return 1;
    case 'c': return 2;
    case 'd': return 3;
    case 'e': return 4;
    case 'f': return 5;
    case 'g': return 6;
    case 'h': return 7;
    default:  return 0;
  }
}

void FenBoard(const std::string &board) {
  int sq = 56;
  for (std::size_t i = 0; i < board.length() && sq >= 0; i += 1) // O(n)
    if (const auto c = board[i]; c == '/') {
      sq -= 16;
    } else if (std::isdigit(c)) {
      sq += FenMakeChar2Num(c);
    } else {
      FenPutPiece(sq, FenMakePiece2Num(c));
      sq += 1;
    }
}

void FenAddCastle(int *rooks, const int sq, const int castle) {
  *rooks           = sq;
  g_board->castle |= castle;
}

void FenAddChess960CastlingW(const char file) {
  if (file < 'A' || file > 'H') return;
  if (const auto tmp = file - 'A'; tmp > g_king_w)
    FenAddCastle(g_rook_w + 0, tmp, 0x1);
  else if (tmp < g_king_w)
    FenAddCastle(g_rook_w + 1, tmp, 0x2);
}

void FenAddChess960CastlingB(const char file) {
  if (file < 'a' || file > 'h') return;
  if (const auto tmp = (file - 'a') + 56; tmp > g_king_b)
    FenAddCastle(g_rook_b + 0, tmp, 0x4);
  else if (tmp < g_king_b)
    FenAddCastle(g_rook_b + 1, tmp, 0x8);
}

void FenAddChess960Castling(const char file) {
  FenAddChess960CastlingW(file);
  FenAddChess960CastlingB(file);
}

void FenKQkq(const std::string &KQkq) {
  for (std::size_t i = 0; i < KQkq.length(); i += 1)
    switch (const auto f = KQkq[i]) {
      case 'K': FenAddCastle(g_rook_w + 0, 7,      0x1); break;
      case 'Q': FenAddCastle(g_rook_w + 1, 0,      0x2); break;
      case 'k': FenAddCastle(g_rook_b + 0, 56 + 7, 0x4); break;
      case 'q': FenAddCastle(g_rook_b + 1, 56 + 0, 0x8); break;
      default:  FenAddChess960Castling(f);               break;
    }
}

void FenEp(const std::string &ep) {
  if (ep.length() != 2) return;
  g_board->epsq = 8 * FenMakeFile2Num(ep[1]) + FenMakeChar2Num(ep[0]) - 1;
}

void FenRule50(const std::string &fifty) {
  if (fifty.length() == 0 || fifty[0] == '-') return;
  g_board->fifty = static_cast<std::uint8_t>(std::min(std::stoi(fifty), FIFTY));
}

void FenFullMoves(const std::string &fullmoves) {
  if (fullmoves.length() == 0) return;
  g_fullmoves = std::max(std::stoi(fullmoves), 1);
}

void FenCheckStr(const std::string &fen, const std::vector<std::string> &tokens) {
  static const std::size_t len = std::string("8/8/8/8/8/8/8/8 w - - 0 1").length();
  if (fen.length() < len                       ||
      tokens.size() < 6                        ||
      tokens[0].find('K') == std::string::npos ||
      tokens[0].find('k') == std::string::npos)
    throw std::runtime_error("info string ( #2 ) Bad fen: " + fen);
}

void FenWtm(const std::string &wtm) {
  g_wtm = wtm == "w";
}

// Fully legal FEN expected
void FenGen(std::string fen) {
  std::vector<std::string> tokens{};
  SplitString< std::vector<std::string> >(fen, tokens);

  FenCheckStr(fen, tokens);
  FenBoard(tokens[0]);
  FenWtm(tokens[1]);
  FenKQkq(tokens[2]);
  FenEp(tokens[3]);
  FenRule50(tokens[4]);
  FenFullMoves(tokens[5]);
  FenBuildCastlingBitboards();
}

// Reset board
void FenReset() {
  g_board_empty = {};
  g_board       = &g_board_empty;
  g_wtm         = true;
  g_king_w      = 0;
  g_king_b      = 0;
  g_fullmoves   = 1;

  for (const std::size_t i : {0, 1}) {
    g_castle_w[i]       = 0;
    g_castle_empty_w[i] = 0;
    g_castle_b[i]       = 0;
    g_castle_empty_b[i] = 0;
    g_rook_w[i]         = 0;
    g_rook_b[i]         = 0;
  }

  for (std::size_t i = 0; i < 6; i += 1) {
    g_board->white[i] = 0;
    g_board->black[i] = 0;
  }
}

void SetFen(const std::string &fen = STARTPOS) {
  FenReset();
  FenGen(fen);
}

// Checks

bool ChecksHereW(const int sq) {
  const auto both = Both();
  return (g_pawn_checks_b[sq]           &  g_board->white[0]) |
         (g_knight_moves[sq]            &  g_board->white[1]) |
         (GetBishopMagicMoves(sq, both) & (g_board->white[2] | g_board->white[4])) |
         (GetRookMagicMoves(sq, both)   & (g_board->white[3] | g_board->white[4])) |
         (g_king_moves[sq]              &  g_board->white[5]);
}

bool ChecksHereB(const int sq) {
  const auto both = Both();
  return (g_pawn_checks_w[sq]           &  g_board->black[0]) |
         (g_knight_moves[sq]            &  g_board->black[1]) |
         (GetBishopMagicMoves(sq, both) & (g_board->black[2] | g_board->black[4])) |
         (GetRookMagicMoves(sq, both)   & (g_board->black[3] | g_board->black[4])) |
         (g_king_moves[sq]              &  g_board->black[5]);
}

bool ChecksCastleW(std::uint64_t squares) {
  while (squares)
    if (ChecksHereW(CtzrPop(&squares)))
      return true;
  return false;
}

bool ChecksCastleB(std::uint64_t squares) {
  while (squares)
    if (ChecksHereB(CtzrPop(&squares)))
      return true;
  return false;
}

bool ChecksW() {
  return ChecksHereW(std::countr_zero(g_board->black[5]));
}

bool ChecksB() {
  return ChecksHereB(std::countr_zero(g_board->white[5]));
}

// Sorting

// Sort only one node at a time ( Avoid the costly n! of operations ! )
// Swap every node for simplicity ( See: lazy-sorting-algorithm paper )
void LazySort(const int ply, const int nth, const int total_moves) {
  for (auto i = nth + 1; i < total_moves; i += 1)
    if (g_boards[ply][i].score > g_boards[ply][nth].score)
      std::swap(g_boards[ply][nth], g_boards[ply][i]);
}

// 1. Evaluate all root moves
void EvalRootMoves() {
  for (auto i = 0; i < g_root_n; i += 1) {
    g_board         = g_boards[0] + i; // Pointer to this board
    g_board->score += (g_board->is_queen_promo() ? 1000  : 0) +
                      (g_board->is_castling()    ? 100   : 0) +
                      (g_board->is_underpromo()  ? -5000 : 0) +
                      (Random(-g_noise, +g_noise)) +       // Add noise -> Make unpredictable
                      (g_wtm ? +1 : -1) * Evaluate(g_wtm); // Full eval
  }
}

// 2. Then sort root moves
struct RootCompFunctor {
  bool operator()(const Board &a, const Board &b) const {
    return a.score > b.score;
  }
};

// 9 -> 0
void SortRootMoves() {
  std::sort(g_boards[0] + 0, g_boards[0] + g_root_n, RootCompFunctor());
}

void SortRoot(const int index) {
  if (!index) return;
  const auto tmp = g_boards[0][index];
  for (auto i = index; i > 0; i -= 1)
    g_boards[0][i] = g_boards[0][i - 1];
  g_boards[0][0] = tmp;
}

void SwapMoveInRootList(const int index) {
  if (!index) return;
  std::swap(g_boards[0][0], g_boards[0][index]);
}

// Move generator

std::uint64_t GetBishopMagicIndex(const int sq, const std::uint64_t mask) {
  return ((mask & kBishopMagics[1][sq]) * kBishopMagics[0][sq]) >> 55;
}

std::uint64_t GetRookMagicIndex(const int sq, const std::uint64_t mask) {
  return ((mask & kRookMagics[1][sq]) * kRookMagics[0][sq]) >> 52;
}

std::uint64_t GetBishopMagicMoves(const int sq, const std::uint64_t mask) {
  return g_bishop_magic_moves[sq][GetBishopMagicIndex(sq, mask)];
}

std::uint64_t GetRookMagicMoves(const int sq, const std::uint64_t mask) {
  return g_rook_magic_moves[sq][GetRookMagicIndex(sq, mask)];
}

void HandleCastlingW(const int mtype, const int from, const int to) {
  g_moves[g_moves_n] = *g_board; // Copy board
  g_board            = &g_moves[g_moves_n]; // Set pointer
  g_board->score     = 0;
  g_board->epsq      = -1;
  g_board->from      = from;
  g_board->to        = to;
  g_board->type      = mtype;
  g_board->castle   &= 0x4 | 0x8;
  g_board->fifty     = 0;
}

void HandleCastlingB(const int mtype, const int from, const int to) {
  g_moves[g_moves_n] = *g_board;
  g_board            = &g_moves[g_moves_n];
  g_board->score     = 0;
  g_board->epsq      = -1;
  g_board->from      = from;
  g_board->to        = to;
  g_board->type      = mtype;
  g_board->castle   &= 0x1 | 0x2;
  g_board->fifty     = 0;
}

void AddCastleOOW() {
  if (ChecksCastleB(g_castle_w[0])) return;

  HandleCastlingW(1, g_king_w, 6);
  g_board->pieces[g_rook_w[0]] = 0;
  g_board->pieces[g_king_w]    = 0;
  g_board->pieces[5]           = +4;
  g_board->pieces[6]           = +6;
  g_board->white[3]            = (g_board->white[3] ^ Bit(g_rook_w[0])) | Bit(5);
  g_board->white[5]            = (g_board->white[5] ^ Bit(g_king_w))    | Bit(6);

  if (ChecksB()) return;
  g_board->index = g_moves_n;
  g_moves_n     += 1;
}

void AddCastleOOB() {
  if (ChecksCastleW(g_castle_b[0])) return;

  HandleCastlingB(3, g_king_b, 56 + 6);
  g_board->pieces[g_rook_b[0]] = 0;
  g_board->pieces[g_king_b]    = 0;
  g_board->pieces[56 + 5]      = -4;
  g_board->pieces[56 + 6]      = -6;
  g_board->black[3]            = (g_board->black[3] ^ Bit(g_rook_b[0])) | Bit(56 + 5);
  g_board->black[5]            = (g_board->black[5] ^ Bit(g_king_b))    | Bit(56 + 6);

  if (ChecksW()) return;
  g_board->index = g_moves_n;
  g_moves_n     += 1;
}

void AddCastleOOOW() {
  if (ChecksCastleB(g_castle_w[1])) return;

  HandleCastlingW(2, g_king_w, 2);
  g_board->pieces[g_rook_w[1]] = 0;
  g_board->pieces[g_king_w]    = 0;
  g_board->pieces[3]           = +4;
  g_board->pieces[2]           = +6;
  g_board->white[3]            = (g_board->white[3] ^ Bit(g_rook_w[1])) | Bit(3);
  g_board->white[5]            = (g_board->white[5] ^ Bit(g_king_w))    | Bit(2);

  if (ChecksB()) return;
  g_board->index = g_moves_n;
  g_moves_n     += 1;
}

void AddCastleOOOB() {
  if (ChecksCastleW(g_castle_b[1])) return;

  HandleCastlingB(4, g_king_b, 56 + 2);
  g_board->pieces[g_rook_b[1]] = 0;
  g_board->pieces[g_king_b]    = 0;
  g_board->pieces[56 + 3]      = -4;
  g_board->pieces[56 + 2]      = -6;
  g_board->black[3]            = (g_board->black[3] ^ Bit(g_rook_b[1])) | Bit(56 + 3);
  g_board->black[5]            = (g_board->black[5] ^ Bit(g_king_b))    | Bit(56 + 2);

  if (ChecksW()) return;
  g_board->index = g_moves_n;
  g_moves_n     += 1;
}

void AddOOW() {
  if (!(g_board->castle & 0x1) || (g_castle_empty_w[0] & g_both)) return;
  AddCastleOOW();
  g_board = g_board_orig;
}

void AddOOOW() {
  if (!(g_board->castle & 0x2) || (g_castle_empty_w[1] & g_both)) return;
  AddCastleOOOW();
  g_board = g_board_orig;
}

void MgenCastlingMovesW() {
  AddOOW();
  AddOOOW();
}

void AddOOB() {
  if (!(g_board->castle & 0x4) || (g_castle_empty_b[0] & g_both)) return;
  AddCastleOOB();
  g_board = g_board_orig;
}

void AddOOOB() {
  if (!(g_board->castle & 0x8) || (g_castle_empty_b[1] & g_both)) return;
  AddCastleOOOB();
  g_board = g_board_orig;
}

void MgenCastlingMovesB() {
  AddOOB();
  AddOOOB();
}

bool CheckHasKingMovedW() {
  if (g_board->pieces[g_king_w] == +6) return false;
  g_board->castle &= 0x4 | 0x8;
  return true;
}

void CheckHasLeftRookMovedW() {
  if (g_board->pieces[g_rook_w[0]] == +4) return;
  g_board->castle &= 0x2 | 0x4 | 0x8;
}

void CheckHasRightRookMovedW() {
  if (g_board->pieces[g_rook_w[1]] == +4) return;
  g_board->castle &= 0x1 | 0x4 | 0x8;
}

void CheckCastlingRightsW() {
  if (CheckHasKingMovedW()) return;
  CheckHasLeftRookMovedW();
  CheckHasRightRookMovedW();
}

bool CheckHasKingMovedB() {
  if (g_board->pieces[g_king_b] == -6) return false;
  g_board->castle &= 0x1 | 0x2;
  return true;
}

void CheckHasLeftRookMovedB() {
  if (g_board->pieces[g_rook_b[0]] == -4) return;
  g_board->castle &= 0x1 | 0x2 | 0x8;
}

void CheckHasRightRookMovedB() {
  if (g_board->pieces[g_rook_b[1]] == -4) return;
  g_board->castle &= 0x1 | 0x2 | 0x4;
}

void CheckCastlingRightsB() {
  if (CheckHasKingMovedB()) return;
  CheckHasLeftRookMovedB();
  CheckHasRightRookMovedB();
}

void HandleCastlingRights() {
  if (!g_board->castle) return;
  CheckCastlingRightsW();
  CheckCastlingRightsB();
}

void ModifyPawnStuffW(const int from, const int to) {
  if (g_board->pieces[to] != +1) return;

  g_board->fifty = 0;
  if (to == g_board_orig->epsq) {
    g_board->score          = 10; // PxP
    g_board->pieces[to - 8] = 0;
    g_board->black[0]      ^= Bit(to - 8);
  } else if (MakeY(from) == 1 && MakeY(to) == 3) { // e2e4 ...
    g_board->epsq = to - 8;
  } else if (MakeY(to) == 6) { // Bonus for 7th ranks
    g_board->score = 91;
  }
}

void ModifyPawnStuffB(const int from, const int to) {
  if (g_board->pieces[to] != -1) return;

  g_board->fifty = 0;
  if (to == g_board_orig->epsq) {
    g_board->score          = 10;
    g_board->pieces[to + 8] = 0;
    g_board->white[0]      ^= Bit(to + 8);
  } else if (MakeY(from) == 6 && MakeY(to) == 4) {
    g_board->epsq = to + 8;
  } else if (MakeY(to) == 1) {
    g_board->score = 91;
  }
}

void AddPromotionW(const int from, const int to, const int piece) {
  const auto eat = g_board->pieces[to];

  g_moves[g_moves_n]         = *g_board;
  g_board                    = &g_moves[g_moves_n];
  g_board->from              = from;
  g_board->to                = to;
  g_board->score             = piece == +5 ? 115 : 0; // Bonus for =q only
  g_board->type              = 3 + piece;
  g_board->epsq              = -1;
  g_board->fifty             = 0;
  g_board->pieces[to]        = piece;
  g_board->pieces[from]      = 0;
  g_board->white[0]         ^= Bit(from);
  g_board->white[piece - 1] |= Bit(to);

  if (eat <= -1)  g_board->black[-eat - 1] ^= Bit(to);

  if (ChecksB()) return;
  HandleCastlingRights();
  g_board->index = g_moves_n++;
}

void AddPromotionB(const int from, const int to, const int piece) {
  const auto eat = g_board->pieces[to];

  g_moves[g_moves_n]          = *g_board;
  g_board                     = &g_moves[g_moves_n];
  g_board->from               = from;
  g_board->to                 = to;
  g_board->score              = piece == -5 ? 115 : 0;
  g_board->type               = 3 + (-piece);
  g_board->epsq               = -1;
  g_board->fifty              = 0;
  g_board->pieces[from]       = 0;
  g_board->pieces[to]         = piece;
  g_board->black[0]          ^= Bit(from);
  g_board->black[-piece - 1] |= Bit(to);

  if (eat >= +1)  g_board->white[eat - 1] ^= Bit(to);

  if (ChecksW()) return;
  HandleCastlingRights();
  g_board->index = g_moves_n;
  g_moves_n     += 1;
}

void AddPromotionW2(const int from, const int to, const int piece) {
  AddPromotionW(from, to, piece);
  g_board = g_board_orig;
}

void AddPromotionB2(const int from, const int to, const int piece) {
  AddPromotionB(from, to, piece);
  g_board = g_board_orig;
}

void AddPromotionStuff_NBRQ_W(const int from, const int to) {
  for (const auto p : {+5, +2, +4, +3})
    AddPromotionW2(from, to, p);
}

void AddPromotionStuff_NQ_W(const int from, const int to) {
  for (const auto p : {+5, +2})
    AddPromotionW2(from, to, p);
}

void AddPromotionStuffW(const int from, const int to) {
  if (g_underpromos)
    AddPromotionStuff_NBRQ_W(from, to);
  else
    AddPromotionStuff_NQ_W(from, to);
}

void AddPromotionStuff_NBRQ_B(const int from, const int to) {
  for (const auto p : {-5, -2, -4, -3})
    AddPromotionB2(from, to, p);
}

void AddPromotionStuff_NQ_B(const int from, const int to) {
  for (const auto p : {-5, -2})
    AddPromotionB2(from, to, p);
}

void AddPromotionStuffB(const int from, const int to) {
  if (g_underpromos)
    AddPromotionStuff_NBRQ_B(from, to);
  else
    AddPromotionStuff_NQ_B(from, to);
}

void CheckNormalCapturesW(const int me, const int eat, const int to) {
  if (eat > -1) return;
  g_board->black[-eat - 1] ^= Bit(to);
  g_board->score            = kMvv[me - 1][-eat - 1];
  g_board->fifty            = 0;
}

void CheckNormalCapturesB(const int me, const int eat, const int to) {
  if (eat < +1) return;
  g_board->white[eat - 1] ^= Bit(to);
  g_board->score           = kMvv[-me - 1][eat - 1];
  g_board->fifty           = 0;
}

// If not under checks -> Handle castling rights -> Add move
void AddMoveIfOkW() {
  if (ChecksB()) return;
  HandleCastlingRights();
  g_board->index = g_moves_n;
  g_moves_n     += 1;
}

void AddMoveIfOkB() {
  if (ChecksW()) return;
  HandleCastlingRights();
  g_board->index = g_moves_n;
  g_moves_n     += 1;
}

void AddNormalStuffW(const int from, const int to) {
  const auto me  = g_board->pieces[from];
  const auto eat = g_board->pieces[to];

  g_moves[g_moves_n]     = *g_board;
  g_board                = &g_moves[g_moves_n];
  g_board->from          = from;
  g_board->to            = to;
  g_board->score         = 0;
  g_board->type          = 0;
  g_board->epsq          = -1;
  g_board->pieces[from]  = 0;
  g_board->pieces[to]    = me;
  g_board->white[me - 1] = (g_board->white[me - 1] ^ Bit(from)) | Bit(to);
  g_board->fifty        += 1; // Rule50 counter increased after non-decisive move

  CheckNormalCapturesW(me, eat, to);
  ModifyPawnStuffW(from, to);
  AddMoveIfOkW();
  g_board = g_board_orig; // Back to the old board
}

void AddNormalStuffB(const int from, const int to) {
  const auto me  = g_board->pieces[from];
  const auto eat = g_board->pieces[to];

  g_moves[g_moves_n]      = *g_board;
  g_board                 = &g_moves[g_moves_n];
  g_board->from           = from;
  g_board->to             = to;
  g_board->score          = 0;
  g_board->type           = 0;
  g_board->epsq           = -1;
  g_board->pieces[to]     = me;
  g_board->pieces[from]   = 0;
  g_board->black[-me - 1] = (g_board->black[-me - 1] ^ Bit(from)) | Bit(to);
  g_board->fifty         += 1;

  CheckNormalCapturesB(me, eat, to);
  ModifyPawnStuffB(from, to);
  AddMoveIfOkB();
  g_board = g_board_orig;
}

void AddW(const int from, const int to) {
  if (g_board->pieces[from] == +1 && MakeY(from) == 6)
    AddPromotionStuffW(from, to);
  else
    AddNormalStuffW(from, to);
}

void AddB(const int from, const int to) {
  if (g_board->pieces[from] == -1 && MakeY(from) == 1)
    AddPromotionStuffB(from, to);
  else
    AddNormalStuffB(from, to);
}

void AddMovesW(const int from, std::uint64_t moves) {
  while (moves) AddW(from, CtzrPop(&moves));
}

void AddMovesB(const int from, std::uint64_t moves) {
  while (moves) AddB(from, CtzrPop(&moves));
}

void MgenPawnsW() {
  for (auto p = g_board->white[0]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesW(sq, g_pawn_checks_w[sq] & g_pawn_sq);
    if (MakeY(sq) == 1) {
      if (g_pawn_1_moves_w[sq] & g_empty)
        AddMovesW(sq, g_pawn_2_moves_w[sq] & g_empty);
    } else {
      AddMovesW(sq, g_pawn_1_moves_w[sq] & g_empty);
    }
  }
}

void MgenPawnsB() {
  for (auto p = g_board->black[0]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesB(sq, g_pawn_checks_b[sq] & g_pawn_sq);
    if (MakeY(sq) == 6) {
      if (g_pawn_1_moves_b[sq] & g_empty)
        AddMovesB(sq, g_pawn_2_moves_b[sq] & g_empty);
    } else {
      AddMovesB(sq, g_pawn_1_moves_b[sq] & g_empty);
    }
  }
}

void MgenPawnsOnlyCapturesW() {
  for (auto p = g_board->white[0]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesW(sq, MakeY(sq) == 6 ? g_pawn_1_moves_w[sq] & (~g_both) : g_pawn_checks_w[sq] & g_pawn_sq);
  }
}

void MgenPawnsOnlyCapturesB() {
  for (auto p = g_board->black[0]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesB(sq, MakeY(sq) == 1 ? g_pawn_1_moves_b[sq] & (~g_both) : g_pawn_checks_b[sq] & g_pawn_sq);
  }
}

void MgenKnightsW() {
  for (auto p = g_board->white[1]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesW(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenKnightsB() {
  for (auto p = g_board->black[1]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesB(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenBishopsPlusQueensW() {
  for (auto p = g_board->white[2] | g_board->white[4]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesW(sq, GetBishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenBishopsPlusQueensB() {
  for (auto p = g_board->black[2] | g_board->black[4]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesB(sq, GetBishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensW() {
  for (auto p = g_board->white[3] | g_board->white[4]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesW(sq, GetRookMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensB() {
  for (auto p = g_board->black[3] | g_board->black[4]; p; ) {
    const auto sq = CtzrPop(&p);
    AddMovesB(sq, GetRookMagicMoves(sq, g_both) & g_good);
  }
}

void MgenKingW() {
  const auto sq = std::countr_zero(g_board->white[5]);
  AddMovesW(sq, g_king_moves[sq] & g_good);
}

void MgenKingB() {
  const auto sq = std::countr_zero(g_board->black[5]);
  AddMovesB(sq, g_king_moves[sq] & g_good);
}

void MgenSetupBoth() {
  g_white = White();
  g_black = Black();
  g_both  = g_white | g_black;
  g_empty = ~g_both;
}

void MgenSetupW() {
  MgenSetupBoth();
  g_pawn_sq = g_black | (g_board->epsq > 0 ? Bit(g_board->epsq) & 0x0000FF0000000000ULL : 0);
}

void MgenSetupB() {
  MgenSetupBoth();
  g_pawn_sq = g_white | (g_board->epsq > 0 ? Bit(g_board->epsq) & 0x0000000000FF0000ULL : 0);
}

void MgenAllW() {
  MgenSetupW();
  g_good = ~g_white;
  MgenPawnsW();
  MgenKnightsW();
  MgenBishopsPlusQueensW();
  MgenRooksPlusQueensW();
  MgenKingW();
  MgenCastlingMovesW();
}

void MgenAllB() {
  MgenSetupB();
  g_good = ~g_black;
  MgenPawnsB();
  MgenKnightsB();
  MgenBishopsPlusQueensB();
  MgenRooksPlusQueensB();
  MgenKingB();
  MgenCastlingMovesB();
}

void MgenAllCapturesW() {
  MgenSetupW();
  g_good = g_black;
  MgenPawnsOnlyCapturesW();
  MgenKnightsW();
  MgenBishopsPlusQueensW();
  MgenRooksPlusQueensW();
  MgenKingW();
}

void MgenAllCapturesB() {
  MgenSetupB();
  g_good = g_white;
  MgenPawnsOnlyCapturesB();
  MgenKnightsB();
  MgenBishopsPlusQueensB();
  MgenRooksPlusQueensB();
  MgenKingB();
}

void MgenReset(Board *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
}

// Generate everything
int MgenW(Board *moves) {
  MgenReset(moves);
  MgenAllW();
  return g_moves_n;
}

int MgenB(Board *moves) {
  MgenReset(moves);
  MgenAllB();
  return g_moves_n;
}

// Generate only captures
int MgenCapturesW(Board *moves) {
  MgenReset(moves);
  MgenAllCapturesW();
  return g_moves_n;
}

int MgenCapturesB(Board *moves) {
  MgenReset(moves);
  MgenAllCapturesB();
  return g_moves_n;
}

// All moves if under checks or just captures
int MgenTacticalW(Board *moves) {
  return ChecksB() ? MgenW(moves) : MgenCapturesW(moves);
}

int MgenTacticalB(Board *moves) {
  return ChecksW() ? MgenB(moves) : MgenCapturesB(moves);
}

// Generate only root moves
void MgenRoot() {
  g_root_n = g_wtm ? MgenW(g_boards[0]) : MgenB(g_boards[0]);
}

// Evaluation

// Mirror horizontal
inline int FlipY(const int sq) {
  return sq ^ 56;
}

// Probe Eucalyptus KPK bitbases -> true: draw / false: not draw
bool ProbeKPK(const bool wtm) {
  return g_board->white[0] ?
    eucalyptus::IsDraw(      std::countr_zero(g_board->white[5]),
      std::countr_zero(g_board->white[0]),              std::countr_zero(g_board->black[5]),   wtm) :
    eucalyptus::IsDraw(FlipY(std::countr_zero(g_board->black[5])),
      FlipY(std::countr_zero(g_board->black[0])), FlipY(std::countr_zero(g_board->white[5])), !wtm);
}

// Detect trivial draws really fast
bool IsEasyDraw(const bool wtm) {
  if (g_board->white[3] || g_board->white[4] || g_board->black[3] || g_board->black[4]) return false; // R/Q/r/q -> No draw

  const auto nnbb  = g_board->white[1] | g_board->white[2] | g_board->black[1] | g_board->black[2];
  const auto pawns = g_board->white[0] | g_board->black[0];
  if (nnbb) return pawns ? false : std::popcount(nnbb) <= 1; // Total 1 N/B + no pawns -> Draw

  const auto pawns_n = std::popcount(pawns); // No N/B/R/Q/n/b/r/q -> Pawns ?
  return pawns_n == 1 ? ProbeKPK(wtm) : (pawns_n == 0); // Check KPK ? / Bare kings ? -> Draw
}

int FixFRC() {
  // No bishop in corner -> Speedup
  static const std::uint64_t corners = Bit(0) | Bit(7) | Bit(56) | Bit(63);
  if (!((g_board->white[2] | g_board->black[2]) & corners)) return 0;

  int s = 0;
  if (g_board->pieces[0]  == +3 && g_board->pieces[9]  == +1) s -= FRC_PENALTY;
  if (g_board->pieces[7]  == +3 && g_board->pieces[14] == +1) s -= FRC_PENALTY;
  if (g_board->pieces[56] == -3 && g_board->pieces[49] == -1) s += FRC_PENALTY;
  if (g_board->pieces[63] == -3 && g_board->pieces[54] == -1) s += FRC_PENALTY;
  return s;
}

// HCE

inline int Square(const int x) {
  return x * x;
}

int CloseBonus(const int a, const int b) {
  return Square(7 - std::abs(MakeX(a) - MakeX(b))) + Square(7 - std::abs(MakeY(a) - MakeY(b)));
}

int CloseAnyCornerBonus(const int sq) {
  return std::max({CloseBonus(sq, 0),  CloseBonus(sq, 7),
                   CloseBonus(sq, 56), CloseBonus(sq, 63)});
}

struct Evaluation {
  const std::uint64_t white{0}, black{0}, both{0};
  const bool wtm{true};
  int w_pieces[5]{}, b_pieces[5]{}, white_total{1}, black_total{1},
      both_total{0}, piece_sum{0}, wk{0}, bk{0}, score{0},
      mg{0}, eg{0}, scale_factor{1};

  void check_blind_bishop_w() {
    const auto wpx   = MakeX(std::countr_zero(g_board->white[0]));
    const auto color = g_board->white[2] & 0x55aa55aa55aa55aaULL;
    if ((color && wpx == 7) || (!color && wpx == 0))
      this->scale_factor = 4;
  }

  void check_blind_bishop_b() {
    const auto bpx   = MakeX(std::countr_zero(g_board->black[0]));
    const auto color = g_board->black[2] & 0x55aa55aa55aa55aaULL;
    if ((!color && bpx == 7) || (color && bpx == 0))
      this->scale_factor = 4;
  }

  Evaluation* pesto_w(const int p, const int sq) {
    this->mg += kPestoPsqt[p][0][sq] + kPestoMaterial[0][p];
    this->eg += kPestoPsqt[p][1][sq] + kPestoMaterial[1][p];
    return this;
  }

  Evaluation* pesto_b(const int p, const int sq) {
    this->mg -= kPestoPsqt[p][0][FlipY(sq)] + kPestoMaterial[0][p];
    this->eg -= kPestoPsqt[p][1][FlipY(sq)] + kPestoMaterial[1][p];
    return this;
  }

  // Squares not having own pieces are reachable
  std::uint64_t reachable_w() const {
    return ~this->white;
  }

  std::uint64_t reachable_b() const {
    return ~this->black;
  }

  Evaluation* mobility_w(const int k, const std::uint64_t m) {
    this->score += k * std::popcount(m);
    return this;
  }

  Evaluation* mobility_b(const int k, const std::uint64_t m) {
    this->score -= k * std::popcount(m);
    return this;
  }

  Evaluation* eval_score_w(const int piece, const int k, const int sq, const std::uint64_t m) {
    return this->pesto_w(piece, sq)
               ->mobility_w(k, m & this->reachable_w());
  }

  Evaluation* eval_score_b(const int piece, const int k, const int sq, const std::uint64_t m) {
    return this->pesto_b(piece, sq)
               ->mobility_b(k, m & this->reachable_b());
  }

  void eval_w(const int p) {
    this->piece_sum   += kPiece[p];
    this->white_total += 1;
    this->w_pieces[p] += 1;
  }

  void eval_b(const int p) {
    this->piece_sum   += kPiece[p];
    this->black_total += 1;
    this->b_pieces[p] += 1;
  }

  void pawn_w(const int sq)   {
    this->pesto_w(0, sq)
        ->eval_w(0);
  }

  void pawn_b(const int sq) {
    this->pesto_b(0, sq)
        ->eval_b(0);
  }

  void knight_w(const int sq) {
    this->eval_score_w(1, 2, sq, g_knight_moves[sq])
        ->eval_w(1);
  }

  void knight_b(const int sq) {
    this->eval_score_b(1, 2, sq, g_knight_moves[sq])
        ->eval_b(1);
  }

  void bishop_w(const int sq) {
    this->eval_score_w(2, 3, sq, GetBishopMagicMoves(sq, this->both))
        ->eval_w(2);
  }

  void bishop_b(const int sq) {
    this->eval_score_b(2, 3, sq, GetBishopMagicMoves(sq, this->both))
        ->eval_b(2);
  }

  void rook_w(const int sq) {
    this->eval_score_w(3, 3, sq, GetRookMagicMoves(sq, this->both))
        ->eval_w(3);
  }

  void rook_b(const int sq) {
    this->eval_score_b(3, 3, sq, GetRookMagicMoves(sq, this->both))
        ->eval_b(3);
  }

  void queen_w(const int sq) {
    this->eval_score_w(4, 2, sq, GetBishopMagicMoves(sq, this->both) | GetRookMagicMoves(sq, this->both))
        ->eval_w(4);
  }

  void queen_b(const int sq) {
    this->eval_score_b(4, 2, sq, GetBishopMagicMoves(sq, this->both) | GetRookMagicMoves(sq, this->both))
        ->eval_b(4);
  }

  void king_w(const int sq) {
    this->eval_score_w(5, 1, sq, g_king_moves[sq])
        ->wk = sq;
  }

  void king_b(const int sq) {
    this->eval_score_b(5, 1, sq, g_king_moves[sq])
        ->bk = sq;
  }

  void eval_piece(const int sq) {
    switch (g_board->pieces[sq]) {
      case +1: this->pawn_w(sq);   break;
      case +2: this->knight_w(sq); break;
      case +3: this->bishop_w(sq); break;
      case +4: this->rook_w(sq);   break;
      case +5: this->queen_w(sq);  break;
      case +6: this->king_w(sq);   break;
      case -1: this->pawn_b(sq);   break;
      case -2: this->knight_b(sq); break;
      case -3: this->bishop_b(sq); break;
      case -4: this->rook_b(sq);   break;
      case -5: this->queen_b(sq);  break;
      case -6: this->king_b(sq);   break;
    }
  }

  Evaluation* evaluate_pieces() {
    for (auto b = this->both; b; )
      this->eval_piece(CtzrPop(&b));
    this->both_total = this->white_total + this->black_total;
    return this;
  }

  void bonus_knbk_w() {
    this->score += (2 * CloseBonus(this->wk, this->bk)) +
                   (10 * ((g_board->white[2] & 0xaa55aa55aa55aa55ULL) ?
                      std::max(CloseBonus(0, this->bk), CloseBonus(63, this->bk)) :
                      std::max(CloseBonus(7, this->bk), CloseBonus(56, this->bk))));
  }

  void bonus_knbk_b() {
    this->score -= (2 * CloseBonus(this->wk, this->bk)) +
                   (10 * ((g_board->black[2] & 0xaa55aa55aa55aa55ULL) ?
                      std::max(CloseBonus(0, this->wk), CloseBonus(63, this->wk)) :
                      std::max(CloseBonus(7, this->wk), CloseBonus(56, this->wk))));
  }

  Evaluation* bonus_tempo() {
    this->score += this->wtm ? +TEMPO_BONUS : -TEMPO_BONUS;
    return this;
  }

  Evaluation* bonus_checks() {
    if (     ChecksW()) this->score += CHECKS_BONUS;
    else if (ChecksB()) this->score -= CHECKS_BONUS;
    return this;
  }

  // Force enemy king in the corner and get closer
  void bonus_mating_w() {
    this->score += 6 * CloseAnyCornerBonus(this->bk) + 4 * CloseBonus(this->wk, this->bk);
  }

  void bonus_mating_b() {
    this->score -= 6 * CloseAnyCornerBonus(this->wk) + 4 * CloseBonus(this->bk, this->wk);
  }

  int wp(const std::size_t x) {
    return this->w_pieces[x];
  }

  int bp(const std::size_t x) {
    return this->b_pieces[x];
  }

  Evaluation* bonus_bishop_pair() {
    if (this->wp(2) >= 2) this->score += BISHOP_PAIR_BONUS;
    if (this->bp(2) >= 2) this->score -= BISHOP_PAIR_BONUS;
    return this;
  }

  // 1. KQvK(PNBR) -> White Checkmate
  // 2. K(PNBR)vKQ -> Black Checkmate
  // 3. KRvK(NB)   -> Drawish (Try to checkmate)
  // 4. K(NB)vKR   -> Drawish (Try to checkmate)
  void bonus_special_4men() {
    if (this->wp(4) && (!this->bp(4))) {
      this->bonus_mating_w();
    } else if (this->bp(4) && (!this->wp(4))) {
      this->bonus_mating_b();
    } else if (this->wp(3) && (this->bp(1) || this->bp(2))) {
      this->scale_factor = 4;
      this->bonus_mating_w();
    } else if (this->bp(3) && (this->wp(1) || this->wp(2))) {
      this->scale_factor = 4;
      this->bonus_mating_b();
    }
  }

  // 1. KRRvKR / KR(NB)vK(NB)               -> White Checkmate
  // 2. KRvKRR / K(NB)vKR(NB)               -> Black Checkmate
  // 3. K(RQ)(PNB)vK(RQ) / K(RQ)vK(RQ)(PNB) -> Drawish
  void bonus_special_5men() {
    if (     (this->wp(3) == 2 && this->bp(3)) ||
             (this->wp(3) && (this->wp(2) || this->wp(1)) && (this->bp(2) || this->bp(1))))
      this->bonus_mating_w();
    else if ((this->bp(3) == 2 && this->wp(3)) ||
             (this->bp(3) && (this->bp(2) || this->bp(1)) && (this->wp(2) || this->wp(1))))
      this->bonus_mating_b();
    else if (((this->wp(3) && this->bp(3)) || (this->wp(4) && this->bp(4))) &&
        ((this->wp(0) || this->wp(1) || this->wp(2)) || (this->bp(0) || this->bp(1) || this->bp(2))))
      this->scale_factor = 4;
  }

  // 1. Special mating pattern (KNBvK)
  // 2. Don't force king to corner    -> Try to promote
  // 3. Can't force mate w/ 2 knights -> Drawish
  void white_is_mating() {
    if (this->white_total == 3) {
      if (this->wp(2) && this->wp(1)) { this->bonus_knbk_w();         return; }
      if (this->wp(2) && this->wp(0)) { this->check_blind_bishop_w(); return; }
      if (this->wp(1) == 2)     { this->scale_factor = 4; }
    }
    this->bonus_mating_w();
  }

  void black_is_mating() {
    if (this->black_total == 3) {
      if (this->bp(2) && this->bp(1)) { this->bonus_knbk_b();         return; }
      if (this->bp(2) && this->bp(0)) { this->check_blind_bishop_b(); return; }
      if (this->bp(1) == 2)     { this->scale_factor = 4; }
    }
    this->bonus_mating_b();
  }

  // Special EG functions. Avoid always doing "Tabula rasa"
  Evaluation* bonus_endgame() {
    if (     this->black_total == 1) this->white_is_mating();
    else if (this->white_total == 1) this->black_is_mating();
    else if (this->both_total  == 4) this->bonus_special_4men();
    else if (this->both_total  == 5) this->bonus_special_5men();
    return this;
  }

  int calculate_score() const { // 78 phases for HCE
    const float n = static_cast<float>(std::clamp(this->piece_sum, 0, MAX_PIECES)) / static_cast<float>(MAX_PIECES);
    const int   s = static_cast<int>(n * static_cast<float>(this->mg) + (1.0f - n) * static_cast<float>(this->eg));
    return (this->score + s) / this->scale_factor;
  }

  int evaluate() {
    return this->evaluate_pieces()
               ->bonus_tempo()
               ->bonus_checks()
               ->bonus_bishop_pair()
               ->bonus_endgame()
               ->calculate_score();
  }
};

// NNUE Eval

struct NnueEval {
  const bool wtm{true};

  int probe() const {
    std::size_t i = 2;
    for (auto both = Both(); both ; )
      switch (const auto sq = CtzrPop(&both); g_board->pieces[sq]) {
        case +1: case +2: case +3: case +4: case +5: // PNBRQ
          g_nnue_pieces[i]    = 7 - g_board->pieces[sq];
          g_nnue_squares[i++] = sq;
          break;
        case -1: case -2: case -3: case -4: case -5: // pnbrq
          g_nnue_pieces[i]    = 13 + g_board->pieces[sq];
          g_nnue_squares[i++] = sq;
          break;
        case +6: // K
          g_nnue_pieces[0]  = 1;
          g_nnue_squares[0] = sq;
          break;
        case -6: // k
          g_nnue_pieces[1]  = 7;
          g_nnue_squares[1] = sq;
          break;
      }

    g_nnue_pieces[i] = g_nnue_squares[i] = 0;

    return this->wtm ? +(nnue::nnue_evaluate(0, g_nnue_pieces, g_nnue_squares) + TEMPO_BONUS) :
                       -(nnue::nnue_evaluate(1, g_nnue_pieces, g_nnue_squares) + TEMPO_BONUS);
  }

  int evaluate() {
    return this->probe() / 4; // NNUE evals are 4x
  }
};

int EvaluateClassical(const bool wtm) {
  return Evaluation { .white = White(), .black = Black(), .both = Both(), .wtm = wtm }
           .evaluate();
}

int EvaluateNNUE(const bool wtm) {
  return NnueEval { .wtm = wtm }
           .evaluate();
}

// Add noise to eval for different playing levels ( -5 -> +5 pawns )
// 0    (Random Mover)
// 1-99 (Levels)
// 100  (Full Strength)
int LevelNoise() {
  return Random(-5 * (100 - g_level), +5 * (100 - g_level));
}

float GetScale() {
  return std::clamp(g_board->fifty < SHUFFLE ?
    1.0f :
    1.0f - ((static_cast<float>(g_board->fifty - SHUFFLE)) / static_cast<float>(FIFTY + 10.0f)), 0.0f, 1.0f);
}

int GetEval(const bool wtm) {
  return FixFRC() + (g_classical ? EvaluateClassical(wtm) : EvaluateNNUE(wtm));
}

int Evaluate(const bool wtm) {
  return LevelNoise() + (IsEasyDraw(wtm) ? 0 : (GetScale() * static_cast<float>(GetEval(wtm))));
}

// Search

void SpeakUci(const int score, const std::uint64_t ms) {
  std::cout <<
    "info depth " << std::min(g_max_depth, g_depth + 1) <<
    " nodes " << g_nodes <<
    " time " << ms <<
    " nps " << Nps(g_nodes, ms) <<
    " score cp " << ((g_wtm ? +1 : -1) * (std::abs(score) == INF ? score / 100 : score)) <<
    " pv " << g_boards[0][0].movename() << std::endl; // flush
}

bool Draw(const bool wtm) {
  // Checkmate overrules the rule 50
  if (g_board->fifty > FIFTY || IsEasyDraw(wtm)) return true;

  // g_r50_positions.pop() must contain hash !
  const auto hash = g_r50_positions[g_board->fifty];
  for (auto i = g_board->fifty - 2, reps = 1; i >= 0; i -= 2)
    if (g_r50_positions[i] == hash && ++reps >= REPS_DRAW)
      return true;

  return false;
}

// Responding to "quit" / "stop" / "isready" signals
bool UserStop() {
  if (!IsInputAvailable()) return false;

  ReadInput();
  if (Token("isready")) {
    std::cout << "readyok" << std::endl;
    return false;
  }

  return Token("quit") ? !(g_game_on = false) : Token("stop");
}

bool CheckTime() {
  static std::uint64_t ticks = 0;
  return ((++ticks) & READ_CLOCK) ? false : ((g_stop_search_time < Now()) || UserStop());
}

// 1. Check against standpat to see whether we are better -> Done
// 2. Iterate deeper
int QSearchW(int alpha, const int beta, const int depth, const int ply) {
  g_nodes += 1; // Increase visited nodes count

  // Search is stopped. Return ASAP
  if (g_stop_search || (g_stop_search = CheckTime())) return 0;

  // Better / terminal node -> Done
  if (((alpha = std::max(alpha, Evaluate(true))) >= beta) || depth <= 0) return alpha;

  const auto moves_n = MgenTacticalW(g_boards[ply]);
  for (auto i = 0; i < moves_n; i += 1) {
    LazySort(ply, i, moves_n); // Very few moves, sort them all
    g_board = g_boards[ply] + i;
    if ((alpha = std::max(alpha, QSearchB(alpha, beta, depth - 1, ply + 1))) >= beta) return alpha;
  }

  return alpha;
}

int QSearchB(const int alpha, int beta, const int depth, const int ply) {
  g_nodes += 1;

  if (g_stop_search) return 0;
  if ((alpha >= (beta = std::min(beta, Evaluate(false)))) || depth <= 0) return beta;

  const auto moves_n = MgenTacticalB(g_boards[ply]);
  for (auto i = 0; i < moves_n; i += 1) {
    LazySort(ply, i, moves_n);
    g_board = g_boards[ply] + i;
    if (alpha >= (beta = std::min(beta, QSearchW(alpha, beta, depth - 1, ply + 1)))) return beta;
  }

  return beta;
}

void SetMoveAndPv(const int ply, const int move_i) {
  g_board = g_boards[ply] + move_i;
  g_is_pv = move_i <= 1 && !g_board->score;
}

int CalcLMR(const int depth, const int move_i) {
  return depth <= 0 || move_i <= 0 ?
    1 :
    std::clamp<int>(0.25 * std::log(depth) * std::log(move_i), 1, 6);
}

// a >= b -> Minimizer won't pick any better move anyway.
//           So searching beyond is a waste of time.
int SearchMovesW(int alpha, const int beta, int depth, const int ply) {
  const auto hash    = g_r50_positions[g_board->fifty];
  const auto checks  = ChecksB();
  const auto moves_n = MgenW(g_boards[ply]);

  // Checkmate or stalemate
  if (!moves_n) return checks ? -INF : 0;
  // Extend interesting path (SRE / CE / PPE)
  if (moves_n == 1 || (depth == 1 && (checks || g_board->type == 8))) depth += 1;

  const auto ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  auto *entry       = &g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)];
  entry->put_hash_value_to_moves(hash, g_boards[ply]);

  // Tiny speedup since not all moves are scored (lots of pointless shuffling ...)
  // So avoid sorting useless moves
  auto sort = true;
  for (auto i = 0; i < moves_n; i += 1) {
    if (sort) {
      LazySort(ply, i, moves_n);
      sort = g_boards[ply][i].score != 0;
    }
    SetMoveAndPv(ply, i);
    if (ok_lmr && i >= 1 && !g_board->score && !ChecksW()) {
      if (SearchB(alpha, beta, depth - 2 - CalcLMR(depth, i), ply + 1) <= alpha) continue;
      SetMoveAndPv(ply, i);
    }
    if (const auto score = SearchB(alpha, beta, depth - 1, ply + 1); score > alpha) { // Improved scope
      if ((alpha = score) >= beta) {
        entry->update<MoveType::kKiller>(hash, g_boards[ply][i].index);
        return alpha;
      }
      entry->update<MoveType::kGood>(hash, g_boards[ply][i].index);
    }
  }

  return alpha;
}

int SearchMovesB(const int alpha, int beta, int depth, const int ply) {
  const auto hash    = g_r50_positions[g_board->fifty];
  const auto checks  = ChecksW();
  const auto moves_n = MgenB(g_boards[ply]);

  if (!moves_n) return checks ? +INF : 0;
  if (moves_n == 1 || (depth == 1 && (checks || g_board->type == 8))) depth += 1;

  const auto ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  auto *entry       = &g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)];
  entry->put_hash_value_to_moves(hash, g_boards[ply]);

  auto sort = true;
  for (auto i = 0; i < moves_n; i += 1) {
    if (sort) {
      LazySort(ply, i, moves_n);
      sort = g_boards[ply][i].score != 0;
    }
    SetMoveAndPv(ply, i);
    if (ok_lmr && i >= 1 && !g_board->score && !ChecksB()) {
      if (SearchW(alpha, beta, depth - 2 - CalcLMR(depth, i), ply + 1) >= beta) continue;
      SetMoveAndPv(ply, i);
    }
    if (const auto score = SearchW(alpha, beta, depth - 1, ply + 1); score < beta) {
      if (alpha >= (beta = score)) {
        entry->update<MoveType::kKiller>(hash, g_boards[ply][i].index);
        return beta;
      }
      entry->update<MoveType::kGood>(hash, g_boards[ply][i].index);
    }
  }

  return beta;
}

// If we do nothing and we are still better -> Done
bool TryNullMoveW(int *alpha, const int beta, const int depth, const int ply) {
  if ((!g_nullmove_active) && // No nullmove on the path ?
      (!g_is_pv) && // Not pv ?
      ( depth >= 3) && // Enough depth ( 2 blunders too much. 3 sweet spot ... ) ?
      ((g_board->white[1] | g_board->white[2] | g_board->white[3] | g_board->white[4]) ||
        (std::popcount(g_board->white[0]) >= 2)) && // Non pawn material or at least 2 pawns ( Zugzwang ... ) ?
      (!ChecksB()) && // Not under checks ?
      ( Evaluate(true) >= beta)) { // Looks good ?
    const auto ep     = g_board->epsq;
    auto *tmp         = g_board;
    g_board->epsq     = -1;
    g_nullmove_active = true;
    const auto score  = SearchB(*alpha, beta, depth - static_cast<int>(depth / 4 + 3), ply);
    g_nullmove_active = false;
    g_board           = tmp;
    g_board->epsq     = ep;
    if (score >= beta) {
      *alpha = score;
      return true;
    }
  }
  return false;
}

bool TryNullMoveB(const int alpha, int *beta, const int depth, const int ply) {
  if ((!g_nullmove_active) &&
      (!g_is_pv) &&
      ( depth >= 3) &&
      ((g_board->black[1] | g_board->black[2] | g_board->black[3] | g_board->black[4]) ||
        (std::popcount(g_board->black[0]) >= 2)) &&
      (!ChecksW()) &&
      ( alpha >= Evaluate(false))) {
    const auto ep     = g_board->epsq;
    auto *tmp         = g_board;
    g_board->epsq     = -1;
    g_nullmove_active = true;
    const auto score  = SearchW(alpha, *beta, depth - static_cast<int>(depth / 4 + 3), ply);
    g_nullmove_active = false;
    g_board           = tmp;
    g_board->epsq     = ep;
    if (alpha >= score) {
      *beta = score;
      return true;
    }
  }
  return false;
}

// Front-end for ab-search
int SearchW(int alpha, const int beta, const int depth, const int ply) {
  g_nodes += 1;

  if (g_stop_search || (g_stop_search = CheckTime())) return 0; // Search is stopped. Return ASAP
  if (depth <= 0 || ply >= MAX_SEARCH_DEPTH) return QSearchW(alpha, beta, g_q_depth, ply);

  const auto fifty = g_board->fifty;
  const auto tmp   = g_r50_positions[fifty];

  if (TryNullMoveW(&alpha, beta, depth, ply)) return alpha;

  g_r50_positions[fifty] = Hash(true);
  alpha                  = Draw(true) ? 0 : SearchMovesW(alpha, beta, depth, ply);
  g_r50_positions[fifty] = tmp;

  return alpha;
}

int SearchB(const int alpha, int beta, const int depth, const int ply) {
  g_nodes += 1;

  if (g_stop_search) return 0;
  if (depth <= 0 || ply >= MAX_SEARCH_DEPTH) return QSearchB(alpha, beta, g_q_depth, ply);

  const auto fifty = g_board->fifty;
  const auto tmp   = g_r50_positions[fifty];

  if (TryNullMoveB(alpha, &beta, depth, ply)) return beta;

  g_r50_positions[fifty] = Hash(false);
  beta                   = Draw(false) ? 0 : SearchMovesB(alpha, beta, depth, ply);
  g_r50_positions[fifty] = tmp;

  return beta;
}

int FindBestW(const int i, const int alpha) {
  if (g_depth >= 1 && i >= 1) { // Null window search for bad moves
    if (const int score = SearchB(alpha, alpha + 1, g_depth, 1); score > alpha) {
      SetMoveAndPv(0, i);
      return SearchB(alpha, +INF, g_depth, 1); // Search w/ full window
    } else {
      return score;
    }
  }
  return SearchB(alpha, +INF, g_depth, 1);
}

// Root search
int SearchRootW() {
  int best_i = 0, alpha = -INF;

  for (auto i = 0; i < g_root_n; i += 1) {
    SetMoveAndPv(0, i); // 1 / 2 moves too good and not tactical -> pv
    const auto score = FindBestW(i, alpha);
    if (g_stop_search) return g_best_score; // Scores are rubbish now
    if (score > alpha) {
      // Skip underpromos unless really good ( 3+ pawns )
      if (g_boards[0][i].is_underpromo() && ((score + (3 * 100)) < alpha)) continue;
      alpha  = score;
      best_i = i;
    }
  }

  SortRoot(best_i);
  return alpha;
}

int FindBestB(const int i, const int beta) {
  if (g_depth >= 1 && i >= 1) {
    if (const int score = SearchW(beta - 1, beta, g_depth, 1); score < beta) {
      SetMoveAndPv(0, i);
      return SearchW(-INF, beta, g_depth, 1);
    } else {
      return score;
    }
  }
  return SearchW(-INF, beta, g_depth, 1);
}

int SearchRootB() {
  int best_i = 0, beta = +INF;

  for (auto i = 0; i < g_root_n; i += 1) {
    SetMoveAndPv(0, i);
    const auto score = FindBestB(i, beta);
    if (g_stop_search) return g_best_score;
    if (score < beta) {
      if (g_boards[0][i].is_underpromo() && ((score - (3 * 100)) > beta)) continue;
      beta   = score;
      best_i = i;
    }
  }

  SortRoot(best_i);
  return beta;
}

// Material detection for classical activation
struct Material {
  const int white_n{0}, black_n{0};

  // KRRvKR / KRvKRR / KRRRvK / KvKRRR ?
  bool is_rook_ending() const {
    return this->white_n + this->black_n == 5 && std::popcount(g_board->white[3] | g_board->black[3]) == 3;
  }

  // Vs king + (PNBRQ) ?
  bool is_easy() const {
    return g_wtm ? this->black_n <= 2 : this->white_n <= 2;
  }

  // 5 pieces or less both side -> Endgame
  bool is_endgame() const {
    return g_wtm ? this->black_n <= 5 : this->white_n <= 5;
  }

  bool is_9_plus_pawns() const {
    return std::popcount(g_board->white[0]) >= 9 || std::popcount(g_board->black[0]) >= 9;
  }

  bool is_3_plus_minors() const {
    for (const std::size_t i : {1, 2, 3, 4}) // 3+ [=n, =b, =r, =q]
      if (std::popcount(g_board->white[i]) >= 3 || std::popcount(g_board->black[i]) >= 3)
        return true;
    return false;
  }

  bool is_pawns_on_1_or_8_ranks() const {
    return 0xFF000000000000FFULL & (g_board->white[0] | g_board->black[0]);
  }

  bool is_lots_of_pieces() const {
    return this->white_n >= 17 || this->black_n >= 17;
  }

  // Non-usual piece setups
  bool is_weird() const {
    return this->is_9_plus_pawns()          ||
           this->is_3_plus_minors()         ||
           this->is_pawns_on_1_or_8_ranks() ||
           this->is_lots_of_pieces();
  }
};

// Activate HCE when ... No NNUE / Easy / Rook ending / Weird piece counts
bool ClassicalActivation(const Material &m) {
  return (!g_nnue_exist) || m.is_easy() || m.is_rook_ending() || m.is_weird();
}

// Play the book move from root list
bool FindBookMove(const int from, const int to, const int type) {
  if (type) { // Castling or promotion
    for (auto i = 0; i < g_root_n; i += 1)
      if (g_boards[0][i].type == type) {
        SwapMoveInRootList(i);
        return true;
      }
  } else {
    for (auto i = 0; i < g_root_n; i += 1)
      if (g_boards[0][i].from == from && g_boards[0][i].to == to) {
        SwapMoveInRootList(i);
        return true;
      }
  }
  return false;
}

int BookSolveType(const int from, const int to, const int move) {
  if (((move >> 12) & 0x7) == 1) return 5; // =n
  if (((move >> 12) & 0x7) == 2) return 6; // =b
  if (((move >> 12) & 0x7) == 3) return 7; // =r
  if (((move >> 12) & 0x7) == 4) return 8; // =q

  if (g_board->pieces[from] == +6 && g_board->pieces[to] == +4) return to > from ? 1 : 2; // OOW / OOOW
  if (g_board->pieces[from] == -6 && g_board->pieces[to] == -4) return to > from ? 3 : 4; // OOB / OOOB

  return 0; // Normal
}

bool ProbePolygotBook() {
  if (const auto move = g_book.setup(g_board->pieces, Both(), g_board->castle, g_board->epsq, g_wtm)
                              .probe(BOOK_BEST)) {
    const auto from = 8 * ((move >> 9) & 0x7) + ((move >> 6) & 0x7);
    const auto to   = 8 * ((move >> 3) & 0x7) + ((move >> 0) & 0x7);
    return FindBookMove(from, to, BookSolveType(from, to, move));
  }

  return false;
}

bool PlayRandomMove() {
  // At level 0 we simply play a random move
  if (g_level == 0) {
    SwapMoveInRootList(Random(0, g_root_n - 1));
    return true;
  }
  return false;
}

bool PlayFastMove(const int ms) {
  if ((g_root_n <= 1)    || // Only move
      (PlayRandomMove()) || // Random mover
      (ms <= 1)          || // Hurry up !
      (g_book_exist && ms > BOOK_MS && ProbePolygotBook())) { // Try book
    SpeakUci(g_last_eval, 0);
    return true;
  }
  return false;
}

void SearchRootMoves(const bool is_eg) {
  auto good        = 0; // Good score in a row for HCE activation
  const auto start = Now();

  for ( ; std::abs(g_best_score) != INF && g_depth < g_max_depth && !g_stop_search; g_depth += 1) {
    g_q_depth = std::min(g_q_depth + 2, MAX_Q_SEARCH_DEPTH);
    g_best_score = g_wtm ? SearchRootW() : SearchRootB();
    // Switch to classical only when the game is decided ( 4+ pawns ) !
    g_classical = g_classical || (is_eg && std::abs(g_best_score) > (4 * 100) && ((++good) >= 7));
    SpeakUci(g_best_score, Now() - start);
  }

  g_last_eval = g_best_score;
  if (!g_q_depth) SpeakUci(g_last_eval, Now() - start); // Nothing searched -> Print smt for UCI
}

// Reset search status
void ResetThink() {
  g_stop_search     = false;
  g_nullmove_active = false;
  g_is_pv           = false;
  g_q_depth         = 0;
  g_best_score      = 0;
  g_nodes           = 0;
  g_depth           = 0;
}

void Think(const int ms) {
  g_stop_search_time = Now(static_cast<std::uint64_t>(ms)); // Start clock early
  ResetThink();
  MgenRoot();
  if (!g_analyzing && PlayFastMove(ms)) return;

  const auto tmp = g_board;
  const Material m{ .white_n = std::popcount(White()),
                    .black_n = std::popcount(Black()) };
  g_classical = ClassicalActivation(m);
  EvalRootMoves();
  SortRootMoves();

  // Only =q and =n are allowed for gameplay
  g_underpromos = g_analyzing; // Can be removed ...
  SearchRootMoves(m.is_endgame());
  g_underpromos = true;
  g_board       = tmp; // Just in case ...
}

// Perft

std::uint64_t Perft(const bool wtm, const int depth, const int ply) {
  if (depth <= 0) return 1;
  const auto moves_n = wtm ? MgenW(g_boards[ply]) : MgenB(g_boards[ply]);
  if (depth == 1) return moves_n; // Bulk counting
  std::uint64_t nodes = 0;
  for (auto i = 0; i < moves_n; i += 1) {
    g_board = g_boards[ply] + i;
    nodes  += Perft(!wtm, depth - 1, ply + 1);
  }
  return nodes;
}

// UCI

void UciMake(const int root_i) {
  if (!g_wtm) g_fullmoves += 1; // Increase fullmoves only after black move
  g_r50_positions[std::min(g_board->fifty, static_cast<std::uint8_t>(R50_ARR - 1))] = Hash(g_wtm); // Set hash
  g_board_empty = g_boards[0][root_i]; // Copy current board
  g_board       = &g_board_empty; // Set pointer ( g_board must always point to smt )
  g_wtm         = !g_wtm; // Flip the board
}

void UciMakeMove() {
  const auto move = TokenGetNth();
  MgenRoot();
  for (auto i = 0; i < g_root_n; i += 1)
    if (move == g_boards[0][i].movename()) {
      UciMake(i);
      return;
    }
  throw std::runtime_error("info string ( #3 ) Bad move: " + move); // No move found -> Quit
}

void UciTakeSpecialFen() {
  TokenPop(); // pop "fen"
  std::stringstream fen{};
  for ( ; TokenIsOk() && !TokenPeek("moves"); TokenPop())
    fen << TokenGetNth() << " ";
  SetFen(fen.str());
}

void UciFen() {
  Token("startpos") ? SetFen() : UciTakeSpecialFen();
}

void UciMoves() {
  for ( ; TokenIsOk(); TokenPop()) UciMakeMove();
}

void UciPosition() {
  UciFen();
  if (Token("moves")) UciMoves();
}

void UciSetChess960() {
  g_chess960 = TokenPeek("true", 3);
}

void UciSetHash() {
  SetHashtable(TokenGetNumber(3));
}

void UciSetLevel() {
  g_level = std::clamp(TokenGetNumber(3), 0, 100);
}

void UciSetMoveOverhead() {
  g_move_overhead = std::clamp(TokenGetNumber(3), 0, 100000);
}

void UciSetEvalFile() {
  SetNNUE(TokenGetNth(3));
}

void UciSetBookFile() {
  SetBook(TokenGetNth(3));
}

void UciSetoption() {
  if (!TokenPeek("name") || !TokenPeek("value", 2)) return;
  if (     TokenPeek("UCI_Chess960", 1)) UciSetChess960();
  else if (TokenPeek("Hash", 1))         UciSetHash();
  else if (TokenPeek("Level", 1))        UciSetLevel();
  else if (TokenPeek("MoveOverhead", 1)) UciSetMoveOverhead();
  else if (TokenPeek("EvalFile", 1))     UciSetEvalFile();
  else if (TokenPeek("BookFile", 1))     UciSetBookFile();
}

void PrintBestMove() {
  std::cout << "bestmove " << (g_root_n <= 0 ? "0000" : g_boards[0][0].movename()) << std::endl;
}

void UciGoInfinite() {
  g_analyzing = true; // Allow: =n / =b / =r / =q
  Think(WEEK);
  g_analyzing = false;
  PrintBestMove();
}

void UciGoMovetime() {
  Think(std::max(0, TokenGetNumber()));
  PrintBestMove();
}

void UciGoDepth() {
  g_max_depth = std::clamp(TokenGetNumber(), 1, MAX_SEARCH_DEPTH);
  Think(WEEK);
  g_max_depth = MAX_SEARCH_DEPTH;
  PrintBestMove();
}

void UciMovetimeCmd() {
  UciGoMovetime();
  TokenPop();
}

void UciDepthCmd() {
  UciGoDepth();
  TokenPop();
}

void UciThink(const int think_time, const int inc, const int mtg) {
  Think(std::min(think_time, think_time / mtg + inc));
  PrintBestMove();
}

// Calculate needed time then think
// Make sure we never lose on time
// Thus small overheadbuffer (100 ms) to prevent time losses
void UciGo() {
  int wtime = 0, btime = 0, winc = 0, binc = 0, mtg = 26;

  for ( ; TokenIsOk(); TokenPop()) {
    if (     Token("wtime"))     { wtime = std::max(0, TokenGetNumber() - g_move_overhead); }
    else if (Token("btime"))     { btime = std::max(0, TokenGetNumber() - g_move_overhead); }
    else if (Token("winc"))      { winc  = std::max(0, TokenGetNumber()); }
    else if (Token("binc"))      { binc  = std::max(0, TokenGetNumber()); }
    else if (Token("movestogo")) { mtg   = std::max(1, TokenGetNumber()); }
    else if (Token("movetime"))  { UciMovetimeCmd(); return; }
    else if (Token("infinite"))  { UciGoInfinite();  return; }
    else if (Token("depth"))     { UciDepthCmd();    return; }
  }

  if (g_wtm)
    UciThink(wtime, winc, mtg);
  else
    UciThink(btime, binc, mtg);
}

void UciUci() {
  std::cout <<
    "id name " << VERSION << '\n' <<
    "id author Toni Helminen\n" <<
    "option name UCI_Chess960 type check default false\n" <<
    "option name Level type spin default 100 min 0 max 100\n" <<
    "option name MoveOverhead type spin default " << MOVEOVERHEAD << " min 0 max 100000\n" <<
    "option name Hash type spin default " << DEF_HASH_MB << " min 1 max 1048576\n" <<
    "option name EvalFile type string default " << EVAL_FILE << '\n' <<
    "option name BookFile type string default " << BOOK_FILE << '\n' <<
    "uciok" << std::endl;
}

// Save state (just in case) if multiple commands in a row
struct Save {
  const bool nnue{false}, book{false};
  const std::string fen{};

  // Save stuff in constructor
  Save() : nnue{g_nnue_exist}, book{g_book_exist}, fen{g_board->to_fen()} { }

  // Restore stuff in destructor
  ~Save() {
    g_nnue_exist = this->nnue;
    g_book_exist = this->book;
    SetFen(this->fen);
  }
};

// Print ASCII art board ( + Used for debug in UCI mode )
void UciPrintBoard() {
  const Save save{};
  const std::string fen = TokenGetNth();
  if (fen.length()) SetFen(fen);
  std::cout << '\n' << g_board->to_s() << std::endl;
}

void PerftUtil(const int depth, const std::string fen) {
  const Save save{};
  std::uint64_t nodes = depth >= 1 ? 0 : 1, total_ms = 0;
  SetFen(fen);
  MgenRoot();
  for (auto i = 0; i < g_root_n; i += 1) {
    g_board           = g_boards[0] + i;
    const auto start  = Now();
    const auto nodes2 = depth >= 0 ? Perft(!g_wtm, depth - 1, 1) : 0;
    const auto ms     = Now() - start;
    std::cout << (i + 1) << ". " << g_boards[0][i].movename() << " -> " <<
                 nodes2 << " (" << ms << " ms)" << std::endl;
    nodes    += nodes2;
    total_ms += ms;
  }
  std::cout << "\n===========================\n\n" <<
    "Nodes:    " << nodes << '\n' <<
    "Time(ms): " << total_ms << '\n' <<
    "NPS:      " << Nps(nodes, total_ms) << std::endl;
}

void Bench(const int depth, const int time) {
  const Save save{};
  SetHashtable(); // Reset hash
  g_max_depth  = depth;
  g_noise      = 0; // Make search deterministic
  g_nnue_exist = false;
  g_book_exist = false; // Disable book + nnue
  std::uint64_t nodes = 0, total_ms = 0;
  int n = 0, correct = 0;
  for (const std::string &fen2 : kBench) {
    for (std::size_t i = 0; i < 2; i += 1) {
      const std::string &fen = i == 0 ? fen2 : FlipFen(fen2);
      std::cout << "[ " << (++n) << "/" << (2 * kBench.size()) << " ; "  << fen << " ]" << std::endl;
      SetFen(fen);
      const std::uint64_t start = Now();
      Think(time);
      total_ms += Now() - start;
      nodes    += g_nodes;
      std::cout << std::endl;
      if (g_boards[0][0].movename() == fen.substr(fen.rfind(" bm ") + 4)) correct += 1;
    }
  }
  g_noise     = NOISE;
  g_max_depth = MAX_SEARCH_DEPTH;
  std::cout << "===========================\n\n" <<
    "Result:   " << correct << " / " << (2 * kBench.size()) << '\n' <<
    "Nodes:    " << nodes << '\n' <<
    "Time(ms): " << total_ms << '\n' <<
    "NPS:      " << Nps(nodes, total_ms) << std::endl;
}

// Show signature of the program
// Result:   70 / 70
// Nodes:    247216819
// Time(ms): 28076
// NPS:      8805272
void UciBench() {
  const std::string depth = TokenGetNth();
  Bench(!depth.length() ? BENCH_DEPTH : (depth == "inf" ? MAX_SEARCH_DEPTH : std::clamp(std::stoi(depth), 0, MAX_SEARCH_DEPTH)),
        WEEK);
}

// Show speed of the program
// Result:   70 / 70
// Nodes:    6294221730
// Time(ms): 578852
// NPS:      10873628
void UciSpeed() {
  const std::string ms = TokenGetNth();
  Bench(MAX_SEARCH_DEPTH,
        !ms.length() ? BENCH_SPEED : std::max(0, std::stoi(ms)));
}

// Calculate perft split numbers
// Nodes:    119060324
// Time(ms): 1779
// NPS:      66925421
void UciPerft() {
  const std::string depth = TokenGetNth(0);
  const std::string fen   = TokenGetNth(1);
  PerftUtil(depth.length() ? std::max(0, std::stoi(depth)) : PERFT_DEPTH,
            fen.length() ? fen : STARTPOS);
}

void UciPrintLogo() {
  std::cout <<
    "___  ___            _ \n"
    "|  \\/  |           | | \n"
    "| .  . | __ _ _   _| |__   ___ _ __ ___ \n"
    "| |\\/| |/ _` | | | | '_ \\ / _ \\ '_ ` _ \\ \n"
    "| |  | | (_| | |_| | | | |  __/ | | | | | \n"
    "\\_|  |_/\\__,_|\\__, |_| |_|\\___|_| |_| |_| \n"
    "               __/ | \n"
    "              |___/ " << std::endl;
}

void UciHelp() {
  std::cout <<
    "Mayhem. Linux UCI Chess960 engine. Written in C++20 language\n\n" <<
    "Supported commands:\n\n" <<
    "help\n  This help\n\n" <<
    "uci\n  Show the engine info\n\n" <<
    "isready\n  Synchronization of the engine. Responded by 'readyok'\n\n" <<
    "ucinewgame\n  Prepare for a new game\n\n" <<
    "stop\n  Stop the search and report the bestmove\n\n" <<
    "quit\n  Exits the engine ASAP\n\n" <<
    "setoption name [str] value [str]\n  Sets a given option ( See 'uci' )\n\n" <<
    "go wtime [int] btime [int] winc [int] binc [int] ...\n" <<
    "    movestogo [int] movetime [int] depth [int] [infinite]\n" <<
    "  Search the current position with the provided settings\n\n" <<
    "position [startpos | fen] [moves]\n" <<
    "  Sets the board position via an optional FEN and optional movelist\n\n" <<
    "logo\n  Print ASCII art logo\n\n" <<
    "p [fen = startpos]\n  Print ASCII art board\n\n" <<
    "perft [depth = 6] [fen = startpos]\n" <<
    "  Calculate perft split numbers\n\n" <<
    "bench [depth = 14]\n"  <<
    "  Show signature of the program\n\n" <<
    "speed [ms = 10000]\n"  <<
    "  Show speed of the program" << std::endl;
}

void UciNewGame() {
  g_last_eval = 0;
}

void UciReadyOk() {
  std::cout << "readyok" << std::endl;
}

void UciUnknownCommand() {
  std::cout << "Unknown command: " << TokenGetNth() << std::endl;
}

bool UciCommands() {
  if (!TokenIsOk()) return true;

  if (     Token("position"))   UciPosition();
  else if (Token("go"))         UciGo();
  else if (Token("isready"))    UciReadyOk();
  else if (Token("ucinewgame")) UciNewGame();
  else if (Token("setoption"))  UciSetoption();
  else if (Token("uci"))        UciUci();
  else if (Token("quit"))       return false;
  // Extra ...
  else if (Token("logo"))       UciPrintLogo();
  else if (Token("help"))       UciHelp();
  else if (Token("bench"))      UciBench();
  else if (Token("speed"))      UciSpeed();
  else if (Token("perft"))      UciPerft();
  else if (Token("p"))          UciPrintBoard();
  else                          UciUnknownCommand();

  return g_game_on;
}

bool Uci() {
  ReadInput();
  return UciCommands();
}

// Init

std::uint64_t PermutateBb(const std::uint64_t moves, const int index) {
  int total = 0, good[64] = {};
  std::uint64_t permutations = 0;

  for (std::size_t i = 0; i < 64; i += 1)
    if (moves & Bit(i))
      good[total++] = i; // post inc

  const auto popn = std::popcount(moves);
  for (auto i = 0; i < popn; i += 1)
    if ((0x1 << i) & index)
      permutations |= Bit(good[i]);

  return permutations & moves;
}

std::uint64_t MakeSliderMagicMoves(const std::vector<int> &slider_vectors, const int sq, const std::uint64_t moves) {
  std::uint64_t possible_moves = 0;
  const auto x_pos = MakeX(sq), y_pos = MakeY(sq);
  for (std::size_t i = 0; i < slider_vectors.size() / 2; i += 1)
    for (std::size_t j = 1; j < 8; j += 1) {
      const auto x = x_pos + j * slider_vectors[2 * i], y = y_pos + j * slider_vectors[2 * i + 1];
      if (!IsOnBoard(x, y)) break;
      const auto tmp  = Bit(8 * y + x);
      possible_moves |= tmp;
      if (tmp & moves) break;
    }
  return possible_moves & (~Bit(sq));
}

void InitBishopMagics() {
  const std::vector<int> bishop_vectors = {+1, +1, -1, -1, +1, -1, -1, +1};
  for (std::size_t i = 0; i < 64; i += 1) {
    const auto magics = kBishopMagics[2][i] & (~Bit(i));
    for (std::size_t j = 0; j < 512; j += 1) {
      const auto allmoves = PermutateBb(magics, j);
      g_bishop_magic_moves[i][GetBishopMagicIndex(i, allmoves)] = MakeSliderMagicMoves(bishop_vectors, i, allmoves);
    }
  }
}

void InitRookMagics() {
  const std::vector<int> rook_vectors = {+1, 0, 0, +1, 0, -1, -1, 0};
  for (std::size_t i = 0; i < 64; i += 1) {
    const auto magics = kRookMagics[2][i] & (~Bit(i));
    for (std::size_t j = 0; j < 4096; j += 1) {
      const auto allmoves = PermutateBb(magics, j);
      g_rook_magic_moves[i][GetRookMagicIndex(i, allmoves)] = MakeSliderMagicMoves(rook_vectors, i, allmoves);
    }
  }
}

std::uint64_t MakeJumpMoves(const int sq, const int dy, const std::vector<int> &jump_vectors) {
  std::uint64_t moves = 0;
  const auto x_pos = MakeX(sq), y_pos = MakeY(sq);
  for (std::size_t i = 0; i < jump_vectors.size() / 2; i += 1)
    if (const auto x = x_pos + jump_vectors[2 * i], y = y_pos + dy * jump_vectors[2 * i + 1]; IsOnBoard(x, y))
      moves |= Bit(8 * y + x);
  return moves;
}

void InitJumpMoves() {
  const std::vector<int> king_vectors       = {+1,  0,  0, +1,  0, -1, -1,  0, +1, +1, -1, -1, +1, -1, -1, +1};
  const std::vector<int> knight_vectors     = {+2, +1, -2, +1, +2, -1, -2, -1, +1, +2, -1, +2, +1, -2, -1, -2};
  const std::vector<int> pawn_check_vectors = {-1, +1, +1, +1};
  const std::vector<int> pawn_1_vectors     = { 0, +1};

  for (std::size_t i = 0; i < 64; i += 1) {
    g_king_moves[i]     = MakeJumpMoves(i, +1, king_vectors);
    g_knight_moves[i]   = MakeJumpMoves(i, +1, knight_vectors);
    g_pawn_checks_w[i]  = MakeJumpMoves(i, +1, pawn_check_vectors);
    g_pawn_checks_b[i]  = MakeJumpMoves(i, -1, pawn_check_vectors);
    g_pawn_1_moves_w[i] = MakeJumpMoves(i, +1, pawn_1_vectors);
    g_pawn_1_moves_b[i] = MakeJumpMoves(i, -1, pawn_1_vectors);
  }

  for (std::size_t i = 0; i < 8; i += 1) {
    g_pawn_2_moves_w[ 8 + i] = MakeJumpMoves( 8 + i, +1, pawn_1_vectors) | MakeJumpMoves( 8 + i, +2, pawn_1_vectors);
    g_pawn_2_moves_b[48 + i] = MakeJumpMoves(48 + i, -1, pawn_1_vectors) | MakeJumpMoves(48 + i, -2, pawn_1_vectors);
  }
}

void InitZobrist() {
  for (std::size_t i = 0; i < 13; i += 1) for (std::size_t j = 0; j < 64; j += 1) g_zobrist_board[i][j] = Random8x64();
  for (std::size_t i = 0; i < 64; i += 1) g_zobrist_ep[i]     = Random8x64();
  for (std::size_t i = 0; i < 16; i += 1) g_zobrist_castle[i] = Random8x64();
  for (std::size_t i = 0; i <  2; i += 1) g_zobrist_wtm[i]    = Random8x64();
}

void PrintVersion() {
  std::cout << VERSION << " by Toni Helminen" << std::endl;
}

// Mayhem initialization (required)
void Init() {
  InitBishopMagics();
  InitRookMagics();
  InitJumpMoves();
  InitZobrist();
  SetHashtable();
  SetNNUE();
  SetBook();
  SetFen();
}

void UciLoop() {
  while (Uci()) continue; // Exe UCI commands
}

} // namespace mayhem
