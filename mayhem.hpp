/*
Mayhem. Linux UCI Chess960 engine. Written in C++14
Copyright (C) 2020 Toni Helminen

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

#include <memory>
#include <iostream>
#include <vector>
#include <fstream>
#include <ctime>
#include <cmath>
#include <unistd.h>
#ifdef WINDOWS
#include <conio.h>
#endif
#include <sys/time.h>
#include "lib/nnue.hpp"
#include "lib/eucalyptus.hpp"
#include "lib/polyglotbook.hpp"

// Namespace

namespace mayhem {

// Constants

const std::string
  kName = "Mayhem 1.6", kStartpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0";

constexpr int
  kMaxMoves = 218, kDepthLimit = 35, kInf = 1048576, kKingVectors[16] = {1,0,0,1,0,-1,-1,0,1,1,-1,-1,1,-1,-1,1}, kKnightVectors[16] = {2,1,-2,1,2,-1,-2,-1,1,2,-1,2,1,-2,-1,-2},
  kBishopVectors[8] = {1,1,-1,-1,1,-1,-1,1}, kRookVectors[8] = {1,0,0,1,0,-1,-1,0}, kMvv[6][6] = {{85,96,97,98,99,100},{84,86,93,94,95,100},{82,83,87,91,92,100},{79,80,81,88,90,100},{75,76,77,78,89,100},{70,71,72,73,74,100}},
  kCenter[64] = {0,1,3,4,4,3,1,0,1,2,4,5,5,4,2,1,3,4,6,7,7,6,4,3,4,5,7,8,8,7,5,4,4,5,7,8,8,7,5,4,3,4,6,7,7,6,4,3,1,2,4,5,5,4,2,1,0,1,3,4,4,3,1,0};

constexpr std::uint64_t
  kRookMagic[64] =
    {0x548001400080106cULL,0x900184000110820ULL,0x428004200a81080ULL,0x140088082000c40ULL,0x1480020800011400ULL,0x100008804085201ULL,0x2a40220001048140ULL,0x50000810000482aULL,
     0x250020100020a004ULL,0x3101880100900a00ULL,0x200a040a00082002ULL,0x1004300044032084ULL,0x2100408001013ULL,0x21f00440122083ULL,0xa204280406023040ULL,0x2241801020800041ULL,
     0xe10100800208004ULL,0x2010401410080ULL,0x181482000208805ULL,0x4080101000021c00ULL,0xa250210012080022ULL,0x4210641044000827ULL,0x8081a02300d4010ULL,0x8008012000410001ULL,
     0x28c0822120108100ULL,0x500160020aa005ULL,0xc11050088c1000ULL,0x48c00101000a288ULL,0x494a184408028200ULL,0x20880100240006ULL,0x10b4010200081ULL,0x40a200260000490cULL,
     0x22384003800050ULL,0x7102001a008010ULL,0x80020c8010900c0ULL,0x100204082a001060ULL,0x8000118188800428ULL,0x58e0020009140244ULL,0x100145040040188dULL,0x44120220400980ULL,
     0x114001007a00800ULL,0x80a0100516304000ULL,0x7200301488001000ULL,0x1000151040808018ULL,0x3000a200010e0020ULL,0x1000849180802810ULL,0x829100210208080ULL,0x1004050021528004ULL,
     0x61482000c41820b0ULL,0x241001018a401a4ULL,0x45020c009cc04040ULL,0x308210c020081200ULL,0xa000215040040ULL,0x10a6024001928700ULL,0x42c204800c804408ULL,0x30441a28614200ULL,
     0x40100229080420aULL,0x9801084000201103ULL,0x8408622090484202ULL,0x4022001048a0e2ULL,0x280120020049902ULL,0x1200412602009402ULL,0x914900048020884ULL,0x104824281002402ULL},
  kRookMask[64] =
    {0x101010101017eULL,0x202020202027cULL,0x404040404047aULL,0x8080808080876ULL,0x1010101010106eULL,0x2020202020205eULL,0x4040404040403eULL,0x8080808080807eULL,
     0x1010101017e00ULL,0x2020202027c00ULL,0x4040404047a00ULL,0x8080808087600ULL,0x10101010106e00ULL,0x20202020205e00ULL,0x40404040403e00ULL,0x80808080807e00ULL,
     0x10101017e0100ULL,0x20202027c0200ULL,0x40404047a0400ULL,0x8080808760800ULL,0x101010106e1000ULL,0x202020205e2000ULL,0x404040403e4000ULL,0x808080807e8000ULL,
     0x101017e010100ULL,0x202027c020200ULL,0x404047a040400ULL,0x8080876080800ULL,0x1010106e101000ULL,0x2020205e202000ULL,0x4040403e404000ULL,0x8080807e808000ULL,
     0x1017e01010100ULL,0x2027c02020200ULL,0x4047a04040400ULL,0x8087608080800ULL,0x10106e10101000ULL,0x20205e20202000ULL,0x40403e40404000ULL,0x80807e80808000ULL,
     0x17e0101010100ULL,0x27c0202020200ULL,0x47a0404040400ULL,0x8760808080800ULL,0x106e1010101000ULL,0x205e2020202000ULL,0x403e4040404000ULL,0x807e8080808000ULL,
     0x7e010101010100ULL,0x7c020202020200ULL,0x7a040404040400ULL,0x76080808080800ULL,0x6e101010101000ULL,0x5e202020202000ULL,0x3e404040404000ULL,0x7e808080808000ULL,
     0x7e01010101010100ULL,0x7c02020202020200ULL,0x7a04040404040400ULL,0x7608080808080800ULL,0x6e10101010101000ULL,0x5e20202020202000ULL,0x3e40404040404000ULL,0x7e80808080808000ULL},
  kRookMoveMagic[64] =
    {0x101010101017eULL,0x202020202027cULL,0x404040404047aULL,0x8080808080876ULL,0x1010101010106eULL,0x2020202020205eULL,0x4040404040403eULL,0x8080808080807eULL,
     0x1010101017e00ULL,0x2020202027c00ULL,0x4040404047a00ULL,0x8080808087600ULL,0x10101010106e00ULL,0x20202020205e00ULL,0x40404040403e00ULL,0x80808080807e00ULL,
     0x10101017e0100ULL,0x20202027c0200ULL,0x40404047a0400ULL,0x8080808760800ULL,0x101010106e1000ULL,0x202020205e2000ULL,0x404040403e4000ULL,0x808080807e8000ULL,
     0x101017e010100ULL,0x202027c020200ULL,0x404047a040400ULL,0x8080876080800ULL,0x1010106e101000ULL,0x2020205e202000ULL,0x4040403e404000ULL,0x8080807e808000ULL,
     0x1017e01010100ULL,0x2027c02020200ULL,0x4047a04040400ULL,0x8087608080800ULL,0x10106e10101000ULL,0x20205e20202000ULL,0x40403e40404000ULL,0x80807e80808000ULL,
     0x17e0101010100ULL,0x27c0202020200ULL,0x47a0404040400ULL,0x8760808080800ULL,0x106e1010101000ULL,0x205e2020202000ULL,0x403e4040404000ULL,0x807e8080808000ULL,
     0x7e010101010100ULL,0x7c020202020200ULL,0x7a040404040400ULL,0x76080808080800ULL,0x6e101010101000ULL,0x5e202020202000ULL,0x3e404040404000ULL,0x7e808080808000ULL,
     0x7e01010101010100ULL,0x7c02020202020200ULL,0x7a04040404040400ULL,0x7608080808080800ULL,0x6e10101010101000ULL,0x5e20202020202000ULL,0x3e40404040404000ULL,0x7e80808080808000ULL},
  kBishopMagic[64] =
    {0x2890208600480830ULL,0x324148050f087ULL,0x1402488a86402004ULL,0xc2210a1100044bULL,0x88450040b021110cULL,0xc0407240011ULL,0xd0246940cc101681ULL,0x1022840c2e410060ULL,
     0x4a1804309028d00bULL,0x821880304a2c0ULL,0x134088090100280ULL,0x8102183814c0208ULL,0x518598604083202ULL,0x67104040408690ULL,0x1010040020d000ULL,0x600001028911902ULL,
     0x8810183800c504c4ULL,0x2628200121054640ULL,0x28003000102006ULL,0x4100c204842244ULL,0x1221c50102421430ULL,0x80109046e0844002ULL,0xc128600019010400ULL,0x812218030404c38ULL,
     0x1224152461091c00ULL,0x1c820008124000aULL,0xa004868015010400ULL,0x34c080004202040ULL,0x200100312100c001ULL,0x4030048118314100ULL,0x410000090018ULL,0x142c010480801ULL,
     0x8080841c1d004262ULL,0x81440f004060406ULL,0x400a090008202ULL,0x2204020084280080ULL,0xb820060400008028ULL,0x110041840112010ULL,0x8002080a1c84400ULL,0x212100111040204aULL,
     0x9412118200481012ULL,0x804105002001444cULL,0x103001280823000ULL,0x40088e028080300ULL,0x51020d8080246601ULL,0x4a0a100e0804502aULL,0x5042028328010ULL,0xe000808180020200ULL,
     0x1002020620608101ULL,0x1108300804090c00ULL,0x180404848840841ULL,0x100180040ac80040ULL,0x20840000c1424001ULL,0x82c00400108800ULL,0x28c0493811082aULL,0x214980910400080cULL,
     0x8d1a0210b0c000ULL,0x164c500ca0410cULL,0xc6040804283004ULL,0x14808001a040400ULL,0x180450800222a011ULL,0x600014600490202ULL,0x21040100d903ULL,0x10404821000420ULL},
  kBishopMask[64] =
    {0x40201008040200ULL,0x402010080400ULL,0x4020100a00ULL,0x40221400ULL,0x2442800ULL,0x204085000ULL,0x20408102000ULL,0x2040810204000ULL,
     0x20100804020000ULL,0x40201008040000ULL,0x4020100a0000ULL,0x4022140000ULL,0x244280000ULL,0x20408500000ULL,0x2040810200000ULL,0x4081020400000ULL,
     0x10080402000200ULL,0x20100804000400ULL,0x4020100a000a00ULL,0x402214001400ULL,0x24428002800ULL,0x2040850005000ULL,0x4081020002000ULL,0x8102040004000ULL,
     0x8040200020400ULL,0x10080400040800ULL,0x20100a000a1000ULL,0x40221400142200ULL,0x2442800284400ULL,0x4085000500800ULL,0x8102000201000ULL,0x10204000402000ULL,
     0x4020002040800ULL,0x8040004081000ULL,0x100a000a102000ULL,0x22140014224000ULL,0x44280028440200ULL,0x8500050080400ULL,0x10200020100800ULL,0x20400040201000ULL,
     0x2000204081000ULL,0x4000408102000ULL,0xa000a10204000ULL,0x14001422400000ULL,0x28002844020000ULL,0x50005008040200ULL,0x20002010080400ULL,0x40004020100800ULL,
     0x20408102000ULL,0x40810204000ULL,0xa1020400000ULL,0x142240000000ULL,0x284402000000ULL,0x500804020000ULL,0x201008040200ULL,0x402010080400ULL,
     0x2040810204000ULL,0x4081020400000ULL,0xa102040000000ULL,0x14224000000000ULL,0x28440200000000ULL,0x50080402000000ULL,0x20100804020000ULL,0x40201008040200ULL},
  kBishopMoveMagics[64] =
    {0x40201008040200ULL,0x402010080400ULL,0x4020100a00ULL,0x40221400ULL,0x2442800ULL,0x204085000ULL,0x20408102000ULL,0x2040810204000ULL,
     0x20100804020000ULL,0x40201008040000ULL,0x4020100a0000ULL,0x4022140000ULL,0x244280000ULL,0x20408500000ULL,0x2040810200000ULL,0x4081020400000ULL,
     0x10080402000200ULL,0x20100804000400ULL,0x4020100a000a00ULL,0x402214001400ULL,0x24428002800ULL,0x2040850005000ULL,0x4081020002000ULL,0x8102040004000ULL,
     0x8040200020400ULL,0x10080400040800ULL,0x20100a000a1000ULL,0x40221400142200ULL,0x2442800284400ULL,0x4085000500800ULL,0x8102000201000ULL,0x10204000402000ULL,
     0x4020002040800ULL,0x8040004081000ULL,0x100a000a102000ULL,0x22140014224000ULL,0x44280028440200ULL,0x8500050080400ULL,0x10200020100800ULL,0x20400040201000ULL,
     0x2000204081000ULL,0x4000408102000ULL,0xa000a10204000ULL,0x14001422400000ULL,0x28002844020000ULL,0x50005008040200ULL,0x20002010080400ULL,0x40004020100800ULL,
     0x20408102000ULL,0x40810204000ULL,0xa1020400000ULL,0x142240000000ULL,0x284402000000ULL,0x500804020000ULL,0x201008040200ULL,0x402010080400ULL,
     0x2040810204000ULL,0x4081020400000ULL,0xa102040000000ULL,0x14224000000000ULL,0x28440200000000ULL,0x50080402000000ULL,0x20100804020000ULL,0x40201008040200ULL};

// Structures

struct Board_t {
  std::uint64_t
    white[6],   // White bitboards
    black[6];   // Black bitboards
  std::int32_t
    score;      // Sorting score
  std::int8_t
    pieces[64], // Pieces black and white
    epsq;       // En passant square
  std::uint8_t
    index,      // Sorting index
    from,       // From square
    to,         // To square
    type,       // Move type (0: Normal, 1: OOw, 2: OOOw, 3: OOb, 4: OOOb, 5: =n, 6: =b, 7: =r, 8: =q)
    castle,     // Castling rights (0x1: K, 0x2: Q, 0x4: k, 0x8: q)
    rule50;     // Rule 50 counter
};

struct Hash_t {std::uint64_t eval_hash, sort_hash; std::int32_t score; std::uint8_t killer, good, quiet;};

// Enums

enum Move_t {kKiller, kGood, kQuiet};

// Variables

int
  g_level = 10, g_move_overhead = 15, g_rook_w[2] = {}, g_rook_b[2] = {}, g_root_n = 0, g_king_w = 0, g_king_b = 0, g_moves_n = 0, g_max_depth = kDepthLimit, g_qs_depth = 6, g_depth = 0, g_best_score = 0;

std::uint32_t
  g_hash_key = 0, g_tokens_nth = 0, g_hash_mb = 256;

std::uint64_t
  g_seed = 131783, g_black = 0, g_white = 0, g_both = 0, g_empty = 0, g_good = 0, g_pawn_sq = 0, g_pawn_1_moves_w[64] = {}, g_pawn_1_moves_b[64] = {}, g_pawn_2_moves_w[64] = {}, g_pawn_2_moves_b[64] = {},
  g_bishop_moves[64] = {}, g_rook_moves[64] = {}, g_queen_moves[64] = {}, g_knight_moves[64] = {}, g_king_moves[64] = {}, g_pawn_checks_w[64] = {}, g_pawn_checks_b[64] = {}, g_castle_w[2] = {}, g_castle_b[2] = {},
  g_castle_empty_w[2] = {}, g_castle_empty_b[2] = {}, g_bishop_magic_moves[64][512] = {}, g_rook_magic_moves[64][4096] = {}, g_zobrist_ep[64] = {}, g_zobrist_castle[16] = {}, g_zobrist_wtm[2] = {},
  g_zobrist_board[13][64] = {}, g_stop_search_time = 0, g_easy_draws[13] = {}, g_r50_positions[128] = {}, g_nodes = 0;

bool
  g_chess960 = false, g_wtm = false, g_stop_search = false, g_underpromos = true, g_nullmove_on = false, g_is_pv = false, g_analyzing = false, g_book_exist = true, g_bare_king = false, g_nnue_exist = true, g_classical = false;

std::vector<std::string>
  g_tokens = {};

struct Board_t
  g_board_tmp = {}, *g_board = &g_board_tmp, *g_moves = 0, *g_board_orig = 0, g_root[kMaxMoves] = {};

PolyglotBook
  g_book;

std::unique_ptr<struct Hash_t[]>
  g_hash;

std::string
  g_eval_file = "nn-c3ca321c51c9.nnue", g_book_file = "performance.bin";

// Prototypes

int SearchB(const int, int, const int, const int);
int SearchW(int, const int, const int, const int);
int QSearchB(const int, int, const int);
int Evaluate(const bool);
std::uint64_t RookMagicMoves(const int, const std::uint64_t);
std::uint64_t BishopMagicMoves(const int, const std::uint64_t);

// Utils

inline std::uint64_t White() {return g_board->white[0] | g_board->white[1] | g_board->white[2] | g_board->white[3] | g_board->white[4] | g_board->white[5];}
inline std::uint64_t Black() {return g_board->black[0] | g_board->black[1] | g_board->black[2] | g_board->black[3] | g_board->black[4] | g_board->black[5];}
inline std::uint64_t Both() {return White() | Black();}
inline int Ctz(const std::uint64_t bb) {return __builtin_ctzll(bb);}
inline int PopCount(const std::uint64_t bb) {return __builtin_popcountll(bb);}
inline std::uint64_t ClearBit(const std::uint64_t bb) {return bb & (bb - 1);}
inline std::uint8_t Xcoord(const std::uint64_t bb) {return bb & 7;}
inline std::uint8_t Ycoord(const std::uint64_t bb) {return bb >> 3;}
inline std::uint64_t Bit(const int nbits) {return 0x1ULL << nbits;}
template <class Type> Type Between(const Type x, const Type y, const Type z) {return std::max(x, std::min(y, z));}
std::uint32_t Nps(const std::uint64_t nodes, const std::uint32_t ms) {return (1000 * nodes) / (ms + 1);}
void Assert(const bool test, const std::string &msg) {if (test) return; std::cerr << msg << std::endl; std::exit(EXIT_FAILURE);}
const std::string MoveStr(const int from, const int to) {char str[5]; str[0] = 'a' + Xcoord(from); str[1] = '1' + Ycoord(from); str[2] = 'a' + Xcoord(to); str[3] = '1' + Ycoord(to); str[4] = '\0'; return std::string(str);}
std::uint64_t Now() {struct timeval tv; if (gettimeofday(&tv, NULL)) return 0; return (std::uint64_t) (1000 * tv.tv_sec + tv.tv_usec / 1000);}
std::uint64_t Random64() {
  static std::uint64_t va = 0X12311227ULL, vb = 0X1931311ULL, vc = 0X13138141ULL;
  const auto mixer = [](std::uint64_t num) {return (num << 7) ^ (num >> 5);};
  va ^= vb + vc;
  vb ^= vb * vc + 0x1717711ULL;
  vc = (3 * vc) + 1;
  return mixer(va) ^ mixer(vb) ^ mixer(vc);
}
std::uint64_t Random8x64() {std::uint64_t ret = 0; for (auto i = 0; i < 8; i++) ret ^= Random64() << (8 * i); return ret;}
int Random(const int max) {const std::uint64_t ret = (g_seed ^ Random64()) & 0xFFFFFFFFULL; g_seed = (g_seed << 5) ^ (g_seed + 1) ^ (g_seed >> 3); return (int) (max * (0.0001f * (float) (ret % 10000)));}
int Random(const int x, const int y) {return x + Random(y - x + 1);}
bool OnBoard(const int x, const int y) {return x >= 0 && x <= 7 && y >= 0 && y <= 7;}

template <class Type> void Splitter(const std::string& str, Type& container, const std::string& delims = " ") {
  std::size_t current = str.find_first_of(delims), previous = 0;
  while (current != std::string::npos) {
    container.push_back(str.substr(previous, current - previous));
    previous = current + 1;
    current  = str.find_first_of(delims, previous);
  }
  container.push_back(str.substr(previous, current - previous));
}

void Input() {
  std::string line;
  std::getline(std::cin, line);
  g_tokens_nth = 0;
  g_tokens.clear();
  Splitter<std::vector<std::string>>(line, g_tokens, " \n");
}

char PromoLetter(const char piece) {
  switch (std::abs(piece)) {
  case 2:  return 'n';
  case 3:  return 'b';
  case 4:  return 'r';
  default: return 'q';}
}

const std::string MoveName(const struct Board_t *move) {
  auto from = move->from, to = move->to;
  switch (move->type) {
  case 1: from = g_king_w; to = g_chess960 ? g_rook_w[0] : 6; break;
  case 2: from = g_king_w; to = g_chess960 ? g_rook_w[1] : 2; break;
  case 3: from = g_king_b; to = g_chess960 ? g_rook_b[0] : 56 + 6; break;
  case 4: from = g_king_b; to = g_chess960 ? g_rook_b[1] : 56 + 2; break;
  case 5: case 6: case 7: case 8: return MoveStr(from, to) + PromoLetter(move->pieces[to]);}
  return MoveStr(from, to);
}

// Lib

void SetupBook() {
  static std::string filename = "???";
  if (filename == g_book_file) return;
  if (g_book_file == "-") {g_book_exist = false;} else {g_book_exist = g_book.open_book(g_book_file); filename = g_book_file;}
  if (!g_book_exist) std::cerr << "Warning: Missing BookFile !" << std::endl;
}

void SetupNNUE() {
  static std::string filename = "???";
  if (filename == g_eval_file) return;
  if (g_eval_file == "-") {g_nnue_exist = false;} else {g_nnue_exist = nnue_init(g_eval_file.c_str()); filename = g_eval_file;}
  if (!g_nnue_exist) std::cerr << "Warning: Missing NNUE EvalFile !" << std::endl;
}

// Hash

std::uint32_t PowerOf2(const std::uint32_t num) {std::uint32_t ret = 1; if (num <= 1) return num; while (ret < num) {if ((ret *= 2) == num) return ret;} return ret / 2;}

void SetupHashtable() {
  g_hash_mb = Between<std::uint32_t>(1, g_hash_mb, 1024 * 1024);
  const auto hashsize = (1 << 20) * g_hash_mb, hash_count = PowerOf2(hashsize / sizeof(struct Hash_t));
  g_hash_key = hash_count - 1;
  g_hash.reset(new struct Hash_t[hash_count]);
  for (std::size_t i = 0; i < hash_count; i++) {g_hash[i].eval_hash = g_hash[i].sort_hash = g_hash[i].score = g_hash[i].killer = g_hash[i].good = g_hash[i].quiet = 0;}
}

inline std::uint64_t Hash(const int wtm) {
  auto hash = g_zobrist_ep[g_board->epsq + 1] ^ g_zobrist_wtm[wtm] ^ g_zobrist_castle[g_board->castle], both = Both();
  for (; both; both = ClearBit(both)) {const auto sq = Ctz(both); hash ^= g_zobrist_board[g_board->pieces[sq] + 6][sq];}
  return hash;
}

// Tokenizer

bool TokenOk(const int look_ahead = 0) {return g_tokens_nth + look_ahead < g_tokens.size();}
const std::string TokenCurrent(const int look_ahead = 0) {return TokenOk(look_ahead) ? g_tokens[g_tokens_nth + look_ahead] : "";}
void TokenPop(const int pop_howmany = 1) {g_tokens_nth += pop_howmany;}
bool Token(const std::string &token, const int pop_howmany = 1) {if (TokenOk(0) && token == TokenCurrent()) {TokenPop(pop_howmany); return true;} return false;}
int TokenNumber(const int look_ahead = 0) {return TokenOk(look_ahead) ? std::stoi(g_tokens[g_tokens_nth + look_ahead]) : 0;}
bool TokenPeek(const std::string &str, const int look_ahead = 0) {return TokenOk(look_ahead) ? str == g_tokens[g_tokens_nth + look_ahead] : false;}

// Board

void BuildBitboards() {
  for (auto i = 0; i < 64; i++)
    if (     g_board->pieces[i] > 0) g_board->white[+g_board->pieces[i] - 1] |= Bit(i);
    else if (g_board->pieces[i] < 0) g_board->black[-g_board->pieces[i] - 1] |= Bit(i);
}

std::uint64_t Fill(int from, const int to) {
  auto ret = Bit(from);
  const auto diff = from > to ? -1 : 1;
  if (from < 0 || to < 0 || from > 63 || to > 63) return 0;
  if (from == to) return ret;
  do {
    from += diff;
    ret  |= Bit(from);
  } while (from != to);
  return ret;
}

void FindKings() {for (auto i = 0; i < 64; i++) if (g_board->pieces[i] == +6) g_king_w = i; else if (g_board->pieces[i] == -6) g_king_b = i;}

void BuildCastlingBitboards() {
  g_castle_w[0]       = Fill(g_king_w, 6);
  g_castle_w[1]       = Fill(g_king_w, 2);
  g_castle_b[0]       = Fill(g_king_b, 56 + 6);
  g_castle_b[1]       = Fill(g_king_b, 56 + 2);
  g_castle_empty_w[0] = (g_castle_w[0] | Fill(g_rook_w[0], 5     )) ^ (Bit(g_king_w) | Bit(g_rook_w[0]));
  g_castle_empty_b[0] = (g_castle_b[0] | Fill(g_rook_b[0], 56 + 5)) ^ (Bit(g_king_b) | Bit(g_rook_b[0]));
  g_castle_empty_w[1] = (g_castle_w[1] | Fill(g_rook_w[1], 3     )) ^ (Bit(g_king_w) | Bit(g_rook_w[1]));
  g_castle_empty_b[1] = (g_castle_b[1] | Fill(g_rook_b[1], 56 + 3)) ^ (Bit(g_king_b) | Bit(g_rook_b[1]));
  for (auto i = 0; i < 2; i++) {
    g_castle_empty_w[i] &= 0xFFULL;
    g_castle_w[i]       &= 0xFFULL;
    g_castle_empty_b[i] &= 0xFF00000000000000ULL;
    g_castle_b[i]       &= 0xFF00000000000000ULL;
  }
}

int Piece(const char piece) {for (auto i = 0; i < 6; i++) {if (piece == "pnbrqk"[i]) return -i - 1; else if (piece == "PNBRQK"[i]) return +i + 1;} return 0;}

void FenBoard(const std::string &board) {
  int sq = 56;
  for (std::size_t i = 0; i < board.length() && sq >= 0; i++) {if (board[i] == '/') sq -= 16; else if (std::isdigit(board[i])) sq += board[i] - '0'; else g_board->pieces[sq++] = Piece(board[i]);}
}

void FenKQkq(const std::string &kqkq) {
  for (std::size_t i = 0; i < kqkq.length(); i++)
    if (     kqkq[i] == 'K') {g_rook_w[0] = 7;      g_board->castle |= 1;}
    else if (kqkq[i] == 'Q') {g_rook_w[1] = 0;      g_board->castle |= 2;}
    else if (kqkq[i] == 'k') {g_rook_b[0] = 56 + 7; g_board->castle |= 4;}
    else if (kqkq[i] == 'q') {g_rook_b[1] = 56 + 0; g_board->castle |= 8;}
    else if (kqkq[i] >= 'A' && kqkq[i] <= 'H') {
      const auto tmp = kqkq[i] - 'A';
      if (tmp > g_king_w) {g_rook_w[0] = tmp; g_board->castle |= 1;} else if (tmp < g_king_w) {g_rook_w[1] = tmp; g_board->castle |= 2;}
    } else if (kqkq[i] >= 'a' && kqkq[i] <= 'h') {
      const auto tmp = kqkq[i] - 'a';
      if (tmp > Xcoord(g_king_b)) {g_rook_b[0] = 56 + tmp; g_board->castle |= 4;} else if (tmp < Xcoord(g_king_b)) {g_rook_b[1] = 56 + tmp; g_board->castle |= 8;}
    }
}

void FenEp(const std::string &ep) {
  if (ep.length() != 2) return;
  g_board->epsq = (ep[0] - 'a') + 8 * (ep[1] - '1');
}

void FenRule50(const std::string &rule50) {
  if (rule50.length() == 0 || rule50[0] == '-') return;
  g_board->rule50 = Between<std::uint8_t>(0, std::stoi(rule50), 100);
}

void FenGen(const std::string &fen) {
  std::vector<std::string> tokens = {};
  Splitter<std::vector<std::string>>(std::string(fen), tokens, " ");
  Assert(tokens.size() >= 3, "Error #1: Bad fen !");
  FenBoard(tokens[0]);
  g_wtm = tokens[1][0] == 'w';
  FindKings();
  FenKQkq(tokens[2]);
  BuildCastlingBitboards();
  FenEp(tokens[3]);
  FenRule50(tokens[4]);
}

void FenReset() {
  constexpr struct Board_t empty = {};
  g_board_tmp   = empty;
  g_board       = &g_board_tmp;
  g_wtm         = 1;
  g_board->epsq = -1;
  g_king_w = g_king_b = 0;
  for (auto i = 0; i < 2; i++) g_rook_w[i] = g_rook_b[i] = 0;
  for (auto i = 0; i < 6; i++) g_board->white[i] = g_board->black[i] = 0;
}

void Fen(const std::string &fen) {
  FenReset();
  FenGen(fen);
  BuildBitboards();
  Assert(PopCount(g_board->white[5]) == 1 && PopCount(g_board->black[5]) == 1, "Error #2: Bad kings !");
}

// Checks

inline bool ChecksHereW(const int sq) {
  const auto both = Both();
  return ((g_pawn_checks_b[sq] & g_board->white[0]) | (g_knight_moves[sq] & g_board->white[1]) | (BishopMagicMoves(sq, both) & (g_board->white[2] | g_board->white[4]))
         | (RookMagicMoves(sq, both) & (g_board->white[3] | g_board->white[4])) | (g_king_moves[sq] & g_board->white[5]));
}

inline bool ChecksHereB(const int sq) {
  const auto both = Both();
  return ((g_pawn_checks_w[sq] & g_board->black[0]) | (g_knight_moves[sq] & g_board->black[1]) | (BishopMagicMoves(sq, both) & (g_board->black[2] | g_board->black[4]))
         | (RookMagicMoves(sq, both) & (g_board->black[3] | g_board->black[4])) | (g_king_moves[sq] & g_board->black[5]));
}

bool ChecksCastleW(std::uint64_t squares) {for (; squares; squares = ClearBit(squares)) {if (ChecksHereW(Ctz(squares))) return true;} return false;}
bool ChecksCastleB(std::uint64_t squares) {for (; squares; squares = ClearBit(squares)) {if (ChecksHereB(Ctz(squares))) return true;} return false;}
bool ChecksW() {return ChecksHereW(Ctz(g_board->black[5]));}
bool ChecksB() {return ChecksHereB(Ctz(g_board->white[5]));}

// Sorting

inline void Swap(struct Board_t *brda, struct Board_t *brdb) {const auto tmp = *brda; *brda = *brdb; *brdb = tmp;}
void SortNthMoves(const int nth) {for (auto i = 0; i < nth; i++) {for (auto j = i + 1; j < g_moves_n; j++) {if (g_moves[j].score > g_moves[i].score) {Swap(g_moves + j, g_moves + i);}}}}
int EvaluateMoves() {int tactics = 0; for (auto i = 0; i < g_moves_n; i++) {if (g_moves[i].score) {tactics++;} g_moves[i].index = i;} return tactics;}
void SortAll() {SortNthMoves(g_moves_n);}

void SortByScore(const struct Hash_t *entry, const std::uint64_t hash) {
  if (entry->sort_hash == hash) {
    if (entry->killer) {g_moves[entry->killer - 1].score += 10000;} else if (entry->good) {g_moves[entry->good - 1].score += 1000;}
    if (entry->quiet) {g_moves[entry->quiet - 1].score += 500;}
  }
  SortNthMoves(EvaluateMoves());
}

void EvalRootMoves() {
  auto *tmp = g_board;
  for (auto i = 0; i < g_root_n; i++) {
    g_board = g_root + i;
    g_board->score += (g_board->type == 8 ? 1000 : 0) + (g_board->type >= 1 && g_board->type <= 4 ? 100 : 0) + (g_board->type >= 5 && g_board->type <= 7 ? -5000 : 0) + (g_wtm ? +1 : -1) * Evaluate(g_wtm) + Random(-2, +2);
  }
  g_board = tmp;
}

void SortRoot(const int index) {
  if (!index) return;
  const auto tmp = g_root[index];
  for (auto i = index; i > 0; i--) g_root[i] = g_root[i - 1];
  g_root[0] = tmp;
}

// Move generator

inline std::uint64_t BishopMagicIndex(const int position, const std::uint64_t mask) {return ((mask & kBishopMask[position]) * kBishopMagic[position]) >> 55;}
inline std::uint64_t RookMagicIndex(const int position, const std::uint64_t mask) {return ((mask & kRookMask[position]) * kRookMagic[position]) >> 52;}
inline std::uint64_t BishopMagicMoves(const int position, const std::uint64_t mask) {return g_bishop_magic_moves[position][BishopMagicIndex(position, mask)];}
inline std::uint64_t RookMagicMoves(const int position, const std::uint64_t mask) {return g_rook_magic_moves[position][RookMagicIndex(position, mask)];}

void HandleCastlingW(const int mtype, const int from, const int to) {
  g_moves[g_moves_n] = *g_board;
  g_board            = &g_moves[g_moves_n];
  g_board->score     = 0;
  g_board->epsq      = -1;
  g_board->from      = from;
  g_board->to        = to;
  g_board->type      = mtype;
  g_board->castle   &= 4 | 8;
  g_board->rule50    = 0;
}

void AddCastleOOW() {
  if (ChecksCastleB(g_castle_w[0])) return;
  HandleCastlingW(1, g_king_w, 6);
  g_board->pieces[g_rook_w[0]] = 0;
  g_board->pieces[g_king_w]    = 0;
  g_board->pieces[5]           = 4;
  g_board->pieces[6]           = 6;
  g_board->white[3]            = (g_board->white[3] ^ Bit(g_rook_w[0])) | Bit(5);
  g_board->white[5]            = (g_board->white[5] ^ Bit(g_king_w))    | Bit(6);
  if (ChecksB()) return;
  g_moves_n++;
}

void AddCastleOOOW() {
  if (ChecksCastleB(g_castle_w[1])) return;
  HandleCastlingW(2, g_king_w, 2);
  g_board->pieces[g_rook_w[1]] = 0;
  g_board->pieces[g_king_w]    = 0;
  g_board->pieces[3]           = 4;
  g_board->pieces[2]           = 6;
  g_board->white[3]            = (g_board->white[3] ^ Bit(g_rook_w[1])) | Bit(3);
  g_board->white[5]            = (g_board->white[5] ^ Bit(g_king_w))    | Bit(2);
  if (ChecksB()) return;
  g_moves_n++;
}

void MgenCastlingMovesW() {
  if ((g_board->castle & 1) && !(g_castle_empty_w[0] & g_both)) {AddCastleOOW();  g_board = g_board_orig;}
  if ((g_board->castle & 2) && !(g_castle_empty_w[1] & g_both)) {AddCastleOOOW(); g_board = g_board_orig;}
}

void HandleCastlingB(const int mtype, const int from, const int to) {
  g_moves[g_moves_n] = *g_board;
  g_board            = &g_moves[g_moves_n];
  g_board->score     = 0;
  g_board->epsq      = -1;
  g_board->from      = from;
  g_board->to        = to;
  g_board->type      = mtype;
  g_board->castle   &= 1 | 2;
  g_board->rule50    = 0;
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
  g_moves_n++;
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
  g_moves_n++;
}

void MgenCastlingMovesB() {
  if ((g_board->castle & 4) && !(g_castle_empty_b[0] & g_both)) {AddCastleOOB();  g_board = g_board_orig;}
  if ((g_board->castle & 8) && !(g_castle_empty_b[1] & g_both)) {AddCastleOOOB(); g_board = g_board_orig;}
}

void CheckCastlingRightsW() {
  if (g_board->pieces[g_king_w]    != 6) {g_board->castle &= 4 | 8; return;}
  if (g_board->pieces[g_rook_w[0]] != 4) {g_board->castle &= 2 | 4 | 8;}
  if (g_board->pieces[g_rook_w[1]] != 4) {g_board->castle &= 1 | 4 | 8;}
}

void CheckCastlingRightsB() {
  if (g_board->pieces[g_king_b]    != -6) {g_board->castle &= 1 | 2; return;}
  if (g_board->pieces[g_rook_b[0]] != -4) {g_board->castle &= 1 | 2 | 8;}
  if (g_board->pieces[g_rook_b[1]] != -4) {g_board->castle &= 1 | 2 | 4;}
}

void HandleCastlingRights() {
  if (!g_board->castle) return;
  CheckCastlingRightsW();
  CheckCastlingRightsB();
}

void ModifyPawnStuffW(const int from, const int to) {
  g_board->rule50 = 0;
  if (to == g_board_orig->epsq) {
    g_board->score          = 85;
    g_board->pieces[to - 8] = 0;
    g_board->black[0]      ^= Bit(to - 8);
  } else if (Ycoord(to) - Ycoord(from) == 2) {
    g_board->epsq = to - 8;
  } else if (Ycoord(to) == 6) {
    g_board->score = 102;
  }
}

void AddPromotionW(const int from, const int to, const int piece) {
  const auto eat = g_board->pieces[to];
  g_moves[g_moves_n]    = *g_board;
  g_board               = &g_moves[g_moves_n];
  g_board->from         = from;
  g_board->to           = to;
  g_board->score        = 100;
  g_board->type         = 3 + piece;
  g_board->epsq         = -1;
  g_board->rule50       = 0;
  g_board->pieces[to]   = piece;
  g_board->pieces[from] = 0;
  g_board->white[0]    ^= Bit(from);
  g_board->white[piece - 1] |= Bit(to);
  if (eat <= -1) g_board->black[-eat - 1] ^= Bit(to);
  if (ChecksB()) return;
  HandleCastlingRights();
  g_moves_n++;
}

void AddPromotionStuffW(const int from, const int to) {
  if (!g_underpromos) {AddPromotionW(from, to, 5); return;}
  auto *tmp = g_board;
  for (auto piece = 2; piece <= 5; piece++) {AddPromotionW(from, to, piece); g_board = tmp;}
}

void AddNormalStuffW(const int from, const int to) {
  const auto me = g_board->pieces[from], eat = g_board->pieces[to];
  if (me <= 0) return;
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
  g_board->rule50++;
  if (eat <= -1) {
    g_board->black[-eat - 1] ^= Bit(to);
    g_board->score            = kMvv[me - 1][-eat - 1];
    g_board->rule50           = 0;
  }
  if (g_board->pieces[to] == 1) ModifyPawnStuffW(from, to);
  if (ChecksB()) return;
  HandleCastlingRights();
  g_moves_n++;
}

void ModifyPawnStuffB(const int from, const int to) {
  g_board->rule50 = 0;
  if (to == g_board_orig->epsq) {
    g_board->score          = 85;
    g_board->pieces[to + 8] = 0;
    g_board->white[0]      ^= Bit(to + 8);
  } else if (Ycoord(to) - Ycoord(from) == -2) {
    g_board->epsq = to + 8;
  } else if (Ycoord(to) == 1) {
    g_board->score = 102;
  }
}

void AddNormalStuffB(const int from, const int to) {
  const auto me = g_board->pieces[from], eat = g_board->pieces[to];
  if (me >= 0) return;
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
  g_board->rule50++;
  if (eat >= 1) {
    g_board->score           = kMvv[-me - 1][eat - 1];
    g_board->rule50          = 0;
    g_board->white[eat - 1] ^= Bit(to);
  }
  if (g_board->pieces[to] == -1) ModifyPawnStuffB(from, to);
  if (ChecksW()) return;
  HandleCastlingRights();
  g_moves_n++;
}

void AddPromotionB(const int from, const int to, const int piece) {
  const auto eat        = g_board->pieces[to];
  g_moves[g_moves_n]    = *g_board;
  g_board               = &g_moves[g_moves_n];
  g_board->from         = from;
  g_board->to           = to;
  g_board->score        = 100;
  g_board->type         = 3 + (-piece);
  g_board->epsq         = -1;
  g_board->rule50       = 0;
  g_board->pieces[from] = 0;
  g_board->pieces[to]   = piece;
  g_board->black[0]    ^= Bit(from);
  g_board->black[-piece - 1] |= Bit(to);
  if (eat >= 1) g_board->white[eat - 1] ^= Bit(to);
  if (ChecksW()) return;
  HandleCastlingRights();
  g_moves_n++;
}

void AddPromotionStuffB(const int from, const int to) {
  if (!g_underpromos) {AddPromotionB(from, to, -5); return;}
  auto *tmp = g_board;
  for (auto piece = 2; piece <= 5; piece++) {AddPromotionB(from, to, -piece); g_board = tmp;}
}

void AddW(const int from, const int to) {if (g_board->pieces[from] == 1  && Ycoord(from) == 6) AddPromotionStuffW(from, to); else AddNormalStuffW(from, to);}
void AddB(const int from, const int to) {if (g_board->pieces[from] == -1 && Ycoord(from) == 1) AddPromotionStuffB(from, to); else AddNormalStuffB(from, to);}
void AddMovesW(const int from, std::uint64_t moves) {for (; moves; moves = ClearBit(moves)) {AddW(from, Ctz(moves)); g_board = g_board_orig;}}
void AddMovesB(const int from, std::uint64_t moves) {for (; moves; moves = ClearBit(moves)) {AddB(from, Ctz(moves)); g_board = g_board_orig;}}

void MgenSetupW() {
  g_white   = White();
  g_black   = Black();
  g_both    = g_white | g_black;
  g_empty   = ~g_both;
  g_pawn_sq = g_board->epsq > 0 ? g_black | (Bit(g_board->epsq) & 0x0000FF0000000000ULL) : g_black;
}

void MgenSetupB() {
  g_white   = White();
  g_black   = Black();
  g_both    = g_white | g_black;
  g_empty   = ~g_both;
  g_pawn_sq = g_board->epsq > 0 ? g_white | (Bit(g_board->epsq) & 0x0000000000FF0000ULL) : g_white;
}

void MgenPawnsW() {
  for (auto pieces = g_board->white[0]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesW(sq, g_pawn_checks_w[sq] & g_pawn_sq);
    if (Ycoord(sq) == 1) {
      if (g_pawn_1_moves_w[sq] & g_empty) AddMovesW(sq, g_pawn_2_moves_w[sq] & g_empty);
    } else {
      AddMovesW(sq, g_pawn_1_moves_w[sq] & g_empty);
    }
  }
}

void MgenPawnsB() {
  for (auto pieces = g_board->black[0]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesB(sq, g_pawn_checks_b[sq] & g_pawn_sq);
    if (Ycoord(sq) == 6) {
      if (g_pawn_1_moves_b[sq] & g_empty) AddMovesB(sq, g_pawn_2_moves_b[sq] & g_empty);
    } else {
      AddMovesB(sq, g_pawn_1_moves_b[sq] & g_empty);
    }
  }
}

void MgenPawnsOnlyCapturesW() {
  for (auto pieces = g_board->white[0]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesW(sq, Ycoord(sq) == 6 ? g_pawn_1_moves_w[sq] & (~g_both) : g_pawn_checks_w[sq] & g_pawn_sq);
  }
}

void MgenPawnsOnlyCapturesB() {
  for (auto pieces = g_board->black[0]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesB(sq, Ycoord(sq) == 1 ? g_pawn_1_moves_b[sq] & (~g_both) : g_pawn_checks_b[sq] & g_pawn_sq);
  }
}

void MgenKnightsW() {
  for (auto pieces = g_board->white[1]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesW(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenKnightsB() {
  for (auto pieces = g_board->black[1]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesB(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenBishopsPlusQueensW() {
  for (auto pieces = g_board->white[2] | g_board->white[4]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesW(sq, BishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenBishopsPlusQueensB() {
  for (auto pieces = g_board->black[2] | g_board->black[4]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesB(sq, BishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensW() {
  for (auto pieces = g_board->white[3] | g_board->white[4]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesW(sq, RookMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensB() {
  for (auto pieces = g_board->black[3] | g_board->black[4]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    AddMovesB(sq, RookMagicMoves(sq, g_both) & g_good);
  }
}

void MgenKingW() {
  const auto sq = Ctz(g_board->white[5]);
  AddMovesW(sq, g_king_moves[sq] & g_good);
}

void MgenKingB() {
  const auto sq = Ctz(g_board->black[5]);
  AddMovesB(sq, g_king_moves[sq] & g_good);
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

int MgenW(struct Board_t *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
  MgenAllW();
  return g_moves_n;
}

int MgenB(struct Board_t *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
  MgenAllB();
  return g_moves_n;
}

int MgenCapturesW(struct Board_t *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
  MgenAllCapturesW();
  return g_moves_n;
}

int MgenCapturesB(struct Board_t *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
  MgenAllCapturesB();
  return g_moves_n;
}

int MgenTacticalW(struct Board_t *moves) {return ChecksB() ? MgenW(moves) : MgenCapturesW(moves);}
int MgenTacticalB(struct Board_t *moves) {return ChecksW() ? MgenB(moves) : MgenCapturesB(moves);}

void MgenRoot() {g_root_n = g_wtm ? MgenW(g_root) : MgenB(g_root);}

// Evaluate

std::uint64_t DrawKey(const int wnn, const int wbn, const int bnn, const int bbn) {return g_zobrist_board[0][wnn] ^ g_zobrist_board[1][wbn] ^ g_zobrist_board[2][bnn] ^ g_zobrist_board[3][bbn];}

bool EasyDraw(const bool wtm) {
  if (g_board->white[3] | g_board->white[4] | g_board->black[3] | g_board->black[4]) return false;
  if (g_board->white[1] | g_board->white[2] | g_board->black[1] | g_board->black[2]) { // NB draws ?
    if (g_board->white[0] | g_board->black[0]) return false;
    const auto hash = DrawKey(PopCount(g_board->white[1]), PopCount(g_board->white[2]), PopCount(g_board->black[1]), PopCount(g_board->black[2]));
    for (auto i = 0; i < 13; i++) {if (g_easy_draws[i] == hash) return true;}
    return false;
  }
  if (PopCount(g_board->white[0] | g_board->black[0]) == 1) { // Probe KPK
    return g_board->white[0] ? eucalyptus::probe(Ctz(g_board->white[5]), Ctz(g_board->white[0]), Ctz(g_board->black[5]), wtm)
                             : eucalyptus::probe(63 - Ctz(g_board->black[5]), 63 - Ctz(g_board->black[0]), 63 - Ctz(g_board->white[5]), !wtm);
  }
  return false;
}

int CloserBonus(const int sq1, const int sq2) {return std::pow(7 - std::abs(Xcoord(sq1) - Xcoord(sq2)), 2) + std::pow(7 - std::abs(Ycoord(sq1) - Ycoord(sq2)), 2);}
int AnyCornerBonus(const int sq) {return std::max(std::max(CloserBonus(sq, 0), CloserBonus(sq, 7)), std::max(CloserBonus(sq, 56), CloserBonus(sq, 63)));}
int BonusKNBK(const bool is_white) {
  const auto wk = Ctz(g_board->white[5]), bk = Ctz(g_board->black[5]);
  return is_white ? 2 * ((g_board->white[2] & 0xaa55aa55aa55aa55ULL) ? std::max(CloserBonus(0, bk), CloserBonus(63, bk)) : std::max(CloserBonus(7, bk), CloserBonus(56, bk)))
                  : 2 * ((g_board->black[2] & 0xaa55aa55aa55aa55ULL) ? std::max(CloserBonus(0, wk), CloserBonus(63, wk)) : std::max(CloserBonus(7, wk), CloserBonus(56, wk)));
}

int EvaluateClassical(const bool wtm) {
  if (EasyDraw(wtm)) return 0;
  const auto white = White(), black = Black(), both = white | black;
  const auto wk = Ctz(g_board->white[5]), bk = Ctz(g_board->black[5]);
  int score = (wtm ? +5 : -5) + (g_nodes & 0x1);
  for (auto pieces = both; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Ctz(pieces);
    switch (g_board->pieces[sq]) {
    case +1: score += 100 +     (g_bare_king ? 10 : 1) * Ycoord(sq); break;
    case +2: score += 300 + 2 * (g_bare_king ? 0  : kCenter[sq] + PopCount(g_knight_moves[sq] & (~white))); break;
    case +3: score += 300 + 2 * (g_bare_king ? 0  : kCenter[sq] + PopCount(BishopMagicMoves(sq, both) & (~white)))
                      + ((PopCount(both) == 4 && g_board->white[1] && g_board->white[2]) ? BonusKNBK(true) : 0); break;
    case +4: score += 500 + 2 * (g_bare_king ? 0  : kCenter[sq] + PopCount(RookMagicMoves(sq, both) & (~white))); break;
    case +5: score += 900 + 2 * (g_bare_king ? 0  : kCenter[sq] + PopCount((BishopMagicMoves(sq, both) | RookMagicMoves(sq, both)) & (~white))); break;
    case +6: score += PopCount(g_king_moves[sq]); break;
    case -1: score -= 100 +     (g_bare_king ? 10 : 1) * (7 - Ycoord(sq)); break;
    case -2: score -= 300 + 2 * (g_bare_king ? 0  : kCenter[sq] + PopCount(g_knight_moves[sq] & (~black))); break;
    case -3: score -= 300 + 2 * (g_bare_king ? 0  : kCenter[sq] + PopCount(BishopMagicMoves(sq, both) & (~black)))
                      + ((PopCount(both) == 4 && g_board->black[1] && g_board->black[2]) ? BonusKNBK(false) : 0); break;
    case -4: score -= 500 + 2 * (g_bare_king ? 0  : kCenter[sq] + PopCount(RookMagicMoves(sq, both) & (~black))); break;
    case -5: score -= 900 + 2 * (g_bare_king ? 0  : kCenter[sq] + PopCount((BishopMagicMoves(sq, both) | RookMagicMoves(sq, both)) & (~black))); break;
    case -6: score -= PopCount(g_king_moves[sq]); break;}
  }
  if (ChecksW()) score += 10; else if (ChecksB()) score -= 10;
  if (!g_bare_king) return score;
  return PopCount(white) >= 2 ? score + 5 * AnyCornerBonus(bk) + 2 * CloserBonus(wk, bk) : score - 5 * AnyCornerBonus(wk) - 2 * CloserBonus(bk, wk);
}

int ProbeNNUE(const bool wtm) {
  int pieces[33], squares[33], index = 2;
  for (auto i = 0; i < 64; i++) {
    switch (g_board->pieces[i]) {
    case +1: case +2: case +3: case +4: case +5: pieces[index] = 7  - ((int) g_board->pieces[i]); squares[index++] = i; break;
    case -1: case -2: case -3: case -4: case -5: pieces[index] = 13 + ((int) g_board->pieces[i]); squares[index++] = i; break;
    case +6: pieces[0] = 1; squares[0] = i; break;
    case -6: pieces[1] = 7; squares[1] = i; break;}
  }
  pieces[index] = squares[index] = 0;
  return (wtm ? 1 : -1) * nnue_evaluate(!wtm, pieces, squares);
}

int EvaluateNNUE(const bool wtm) {
  const auto hash = Hash(wtm);
  auto *entry = &g_hash[(std::uint32_t) (hash & g_hash_key)];
  if (entry->eval_hash == hash) return entry->score;
  entry->eval_hash = hash;
  return entry->score = EasyDraw(wtm) ? 0 : ProbeNNUE(wtm);
}

int Evaluate(const bool wtm) {return (int) ((1.0f - (((float) g_board->rule50) / 100.0f)) * (g_classical ? EvaluateClassical(wtm) : EvaluateNNUE(wtm)));}

// Search

void Speak(const int score, const std::uint64_t search_time) {
  std::cout << "info depth " << std::min(g_max_depth, g_depth + 1) << " nodes " << g_nodes << " time " << search_time << " nps " << Nps(g_nodes, search_time)
            << " score cp " << ((g_wtm ? 1 : -1) * (int) ((std::abs(score) >= kInf ? 0.01f : 0.5f) * score)) << " pv " << MoveName(g_root) << std::endl;
}

bool Draw() {
  if (g_board->rule50 >= 100) return true;
  const auto hash = g_r50_positions[g_board->rule50];
  for (auto i = g_board->rule50 - 2; i >= 0; i -= 2) {if (g_r50_positions[i] == hash) return true;}
  return false;
}

#ifdef WINDOWS
bool InputAvailable() {return _kbhit();}
#else
bool InputAvailable() {
  fd_set fd;
  struct timeval tv;
  FD_ZERO(&fd);
  FD_SET(STDIN_FILENO, &fd);
  tv.tv_sec = tv.tv_usec = 0;
  select(STDIN_FILENO + 1, &fd, 0, 0, &tv);
  return FD_ISSET(STDIN_FILENO, &fd) > 0;
}
#endif

bool UserStop() {
  if (!g_analyzing || !InputAvailable()) return false;
  Input();
  if (Token("stop")) return true;
  return false;
}

bool TimeCheckSearch() {
  static std::uint64_t ticks = 0;
  if (++ticks & 0xFFULL) return false;
  if ((Now() >= g_stop_search_time) || UserStop()) g_stop_search = true;
  return g_stop_search;
}

int QSearchW(int alpha, const int beta, const int depth) {
  g_nodes++;
  if (g_stop_search || TimeCheckSearch()) return 0;
  alpha = std::max(alpha, Evaluate(1));
  if (depth <= 0 || alpha >= beta) return alpha;
  struct Board_t moves[64];
  const auto moves_n = MgenTacticalW(moves);
  SortAll();
  for (auto i = 0; i < moves_n; i++) {g_board = moves + i; if ((alpha = std::max(alpha, QSearchB(alpha, beta, depth - 1))) >= beta) return alpha;}
  return alpha;
}

int QSearchB(const int alpha, int beta, const int depth) {
  g_nodes++;
  if (g_stop_search) return 0;
  beta = std::min(beta, Evaluate(0));
  if (depth <= 0 || alpha >= beta) return beta;
  struct Board_t moves[64];
  const auto moves_n = MgenTacticalB(moves);
  SortAll();
  for (auto i = 0; i < moves_n; i++) {g_board = moves + i; if (alpha >= (beta = std::min(beta, QSearchW(alpha, beta, depth - 1)))) return beta;}
  return beta;
}

void UpdateSort(struct Hash_t *entry, enum Move_t type, const std::uint64_t hash, const std::uint8_t index) {
  entry->sort_hash = hash;
  switch (type) {case kKiller: entry->killer = index + 1; break; case kGood: entry->good = index + 1; break; case kQuiet: entry->quiet = index + 1; break;}
}

int SearchMovesW(int alpha, const int beta, int depth, const int ply) {
  const auto hash = g_r50_positions[g_board->rule50];
  const auto checks = ChecksB();
  struct Board_t moves[kMaxMoves];
  const auto moves_n = MgenW(moves);
  if (!moves_n) return checks ? -kInf : 0; else if (moves_n == 1 || (ply < 5 && checks)) depth++;
  auto ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  auto *entry = &g_hash[(std::uint32_t) (hash & g_hash_key)];
  SortByScore(entry, hash);
  for (auto i = 0; i < moves_n; i++) {
    g_board = moves + i;
    g_is_pv = i <= 1 && !moves[i].score;
    if (ok_lmr && i >= 2 && (!g_board->score) && !ChecksW()) {if (SearchB(alpha, beta, depth - 2 - std::min(1, i / 23), ply + 1) <= alpha) continue; g_board = moves + i;}
    const auto score = SearchB(alpha, beta, depth - 1, ply + 1);
    if (score > alpha) {
      alpha  = score;
      ok_lmr = 0;
      if (alpha >= beta) {UpdateSort(entry, kKiller, hash, moves[i].index); return alpha;}
      UpdateSort(entry, moves[i].score ? kGood : kQuiet, hash, moves[i].index);
    }
  }
  return alpha;
}

int TryNullMove(const int alpha, const int beta, const int depth, const int ply, const bool wtm) {
  if (!g_nullmove_on && !g_is_pv && depth >= 4 && !(wtm ? ChecksB() : ChecksW())) {
    g_nullmove_on = true;
    const auto ep = g_board->epsq;
    auto *tmp     = g_board;
    g_board->epsq = -1;
    const auto score = wtm ? SearchB(alpha, beta, depth - 4, ply) : SearchW(alpha, beta, depth - 4, ply);
    g_nullmove_on = false;
    g_board       = tmp;
    g_board->epsq = ep;
    if (wtm) {if (score >= beta) {return score;}} else {if (alpha >= score) {return score;}}
  }
  return wtm ? alpha : beta;
}

int SearchW(int alpha, const int beta, const int depth, const int ply) {
  g_nodes++;
  if (g_stop_search || TimeCheckSearch()) return 0;
  if (depth <= 0 || ply >= kDepthLimit) return QSearchW(alpha, beta, g_qs_depth);
  const auto rule50 = g_board->rule50;
  const auto tmp = g_r50_positions[rule50];
  const auto null_score = TryNullMove(alpha, beta, depth, ply, 1);
  if (null_score >= beta) return null_score;
  g_r50_positions[rule50] = Hash(1);
  alpha = Draw() ? 0 : SearchMovesW(alpha, beta, depth, ply);
  g_r50_positions[rule50] = tmp;
  return alpha;
}

int SearchMovesB(const int alpha, int beta, int depth, const int ply) {
  const auto hash = g_r50_positions[g_board->rule50];
  const auto checks = ChecksW();
  struct Board_t moves[kMaxMoves];
  const auto moves_n = MgenB(moves);
  if (!moves_n) return checks ? kInf : 0; else if (moves_n == 1 || (ply < 5 && checks)) depth++;
  auto ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  auto *entry = &g_hash[(std::uint32_t) (hash & g_hash_key)];
  SortByScore(entry, hash);
  for (auto i = 0; i < moves_n; i++) {
    g_board = moves + i;
    g_is_pv = i <= 1 && !moves[i].score;
    if (ok_lmr && i >= 2 && !g_board->score && !ChecksB()) {if (SearchW(alpha, beta, depth - 2 - std::min(1, i / 23), ply + 1) >= beta) continue; g_board = moves + i;}
    const auto score = SearchW(alpha, beta, depth - 1, ply + 1);
    if (score < beta) {
      beta   = score;
      ok_lmr = 0;
      if (alpha >= beta) {UpdateSort(entry, kKiller, hash, moves[i].index); return beta;}
      UpdateSort(entry, moves[i].score ? kGood : kQuiet, hash, moves[i].index);
    }
  }
  return beta;
}

int SearchB(const int alpha, int beta, const int depth, const int ply) {
  g_nodes++;
  if (g_stop_search) return 0;
  if (depth <= 0 || ply >= kDepthLimit) return QSearchB(alpha, beta, g_qs_depth);
  const auto rule50 = g_board->rule50;
  const auto tmp = g_r50_positions[rule50];
  const auto null_score = TryNullMove(alpha, beta, depth, ply, 0);
  if (alpha >= null_score) return null_score;
  g_r50_positions[rule50] = Hash(0);
  beta = Draw() ? 0 : SearchMovesB(alpha, beta, depth, ply);
  g_r50_positions[rule50] = tmp;
  return beta;
}

int BestW() {
  auto score = 0, best_i = 0, alpha = -kInf;
  for (auto i = 0; i < g_root_n; i++) {
    g_board = g_root + i;
    g_is_pv = i <= 1 && !g_root[i].score;
    if (g_depth >= 1 && i >= 1) {
      if ((score = SearchB(alpha, alpha + 1, g_depth, 0)) > alpha) {g_board = g_root + i; score = SearchB(alpha, kInf, g_depth, 0);}
    } else {
      score = SearchB(alpha, kInf, g_depth, 0);
    }
    if (g_stop_search) return g_best_score;
    if (score > alpha) {if ((g_root[i].type >= 5 && g_root[i].type <= 7) && std::abs(score) < kInf / 2) continue; /* No underpromos */ alpha = score; best_i = i;}
  }
  SortRoot(best_i);
  return alpha;
}

int BestB() {
  auto score = 0, best_i = 0, beta = kInf;
  for (auto i = 0; i < g_root_n; i++) {
    g_board = g_root + i;
    g_is_pv = i <= 1 && !g_root[i].score;
    if (g_depth >= 1 && i >= 1) {
      if ((score = SearchW(beta - 1, beta, g_depth, 0)) < beta) {g_board = g_root + i; score = SearchW(-kInf, beta, g_depth, 0);}
    } else {
      score = SearchW(-kInf, beta, g_depth, 0);
    }
    if (g_stop_search) return g_best_score;
    if (score < beta) {if ((g_root[i].type >= 5 && g_root[i].type <= 7) && std::abs(score) < kInf / 2) continue; beta = score; best_i = i;}
  }
  SortRoot(best_i);
  return beta;
}

void ThinkSetup(const int think_time) {
  g_stop_search = g_is_pv = false;
  g_best_score = g_nodes = g_depth = 0;
  g_qs_depth = 4;
  g_stop_search_time = Now() + (std::uint64_t) std::max(0, think_time);
  g_bare_king = g_nullmove_on = (g_wtm && PopCount(Black()) == 1) || (!g_wtm && PopCount(White()) == 1); // vs bare king = active mate help + disable null move
  g_classical = g_bare_king || !g_nnue_exist;
}

bool ThinkRandomMove() {
  if (g_level) return false;
  const auto root_i = Random(0, g_root_n - 1);
  if (root_i) Swap(g_root, g_root + root_i);
  return true;
}

bool ProbeBook() {
  const int move = g_book.probe(g_board->pieces, g_board->castle, g_board->epsq, g_wtm, Random(0, 7) > 5);
  if (!move) return false;
  const std::uint8_t from = 8 * ((move >> 9) & 0x7) + ((move >> 6) & 0x7),
                     to   = 8 * ((move >> 3) & 0x7) + ((move >> 0) & 0x7);
  std::uint8_t type = 0;
  for (auto i = 0; i < 4; i++) {if (move & (0x1 << (12 + i))) type = 5 + i;} // Promos
  for (auto i = 0; i < g_root_n; i++) {
    if (g_root[i].from == from && g_root[i].to == to) {
      if (type && g_root[i].type != type) continue;
      SortRoot(i);
      return true;
    }
  }
  return false;
}

void Think(const int think_time) {
  auto *tmp = g_board;
  const auto start = Now();
  ThinkSetup(think_time);
  MgenRoot();
  if (g_root_n <= 1 || ThinkRandomMove()) {Speak(0, 0); return;}
  if (g_book_exist && think_time > 10 && !g_analyzing && ProbeBook()) {Speak(0, 0); return;}
  EvalRootMoves();
  SortAll();
  g_underpromos = false;
  for (; std::abs(g_best_score) < kInf / 2 && g_depth < g_max_depth && !g_stop_search; g_depth++) {
    g_best_score = g_wtm ? BestW() : BestB();
    Speak(g_best_score, Now() - start);
    g_qs_depth = std::min(g_qs_depth + 2, 12);
  }
  g_underpromos = true;
  if (g_level >= 1 && g_level <= 9) {if (Random(0, 9) >= g_level) Swap(g_root, g_root + 1);}
  g_board = tmp;
  Speak(g_best_score, Now() - start);
}

// UCI

void Make(const int root_i) {
  g_r50_positions[g_board->rule50] = Hash(g_wtm);
  g_board_tmp = g_root[root_i];
  g_board     = &g_board_tmp;
  g_wtm       = !g_wtm;
}

void MakeMove() {
  const auto move = TokenCurrent();
  MgenRoot();
  for (auto i = 0; i < g_root_n; i++) {if (move == MoveName(g_root + i)) {Make(i); return;}}
  Assert(false, "Error #3: Bad move !");
}

void UciFen() {
  if (Token("startpos")) return;
  TokenPop();
  std::string fen = "";
  for (; TokenOk() && !Token("moves", 0); TokenPop()) fen += TokenCurrent() + " ";
  Fen(fen);
}

void UciMoves() {while (TokenOk()) {MakeMove(); TokenPop();}}

void UciPosition() {
  Fen(kStartpos);
  UciFen();
  if (Token("moves")) UciMoves();
}

void UciSetoption() {
  if (     TokenPeek("name") && TokenPeek("UCI_Chess960", 1) && TokenPeek("value", 2)) {g_chess960 = TokenPeek("true", 3); TokenPop(4);}
  else if (TokenPeek("name") && TokenPeek("Level", 1)        && TokenPeek("value", 2)) {g_level = Between<int>(0, TokenNumber(3), 10); TokenPop(4);}
  else if (TokenPeek("name") && TokenPeek("Hash", 1)         && TokenPeek("value", 2)) {g_hash_mb = TokenNumber(3); SetupHashtable(); TokenPop(4);}
  else if (TokenPeek("name") && TokenPeek("MoveOverhead", 1) && TokenPeek("value", 2)) {g_move_overhead = Between<int>(0, TokenNumber(3), 5000); TokenPop(4);}
  else if (TokenPeek("name") && TokenPeek("EvalFile", 1)     && TokenPeek("value", 2)) {g_eval_file = TokenCurrent(3); SetupNNUE(); TokenPop(4);}
  else if (TokenPeek("name") && TokenPeek("BookFile", 1)     && TokenPeek("value", 2)) {g_book_file = TokenCurrent(3); SetupBook(); TokenPop(4);}
}

void UciGo() {
  int wtime = 0, btime = 0, winc = 0, binc = 0, mtg = 30;
  const auto print_best_move = []() {std::cout << "bestmove " << (g_root_n <= 0 ? "0000" : MoveName(g_root)) << std::endl;};
  for (; TokenOk(); TokenPop()) {
    if (     Token("infinite"))  {g_analyzing = 1; Think(kInf); g_analyzing = 0; print_best_move(); return;}
    else if (Token("wtime"))     {wtime = std::max(0, TokenNumber() - g_move_overhead);}
    else if (Token("btime"))     {btime = std::max(0, TokenNumber() - g_move_overhead);}
    else if (Token("winc"))      {winc = TokenNumber(); winc = std::max(0, winc ? winc - g_move_overhead : winc);}
    else if (Token("binc"))      {binc = TokenNumber(); binc = std::max(0, binc ? binc - g_move_overhead : binc);}
    else if (Token("movestogo")) {mtg = Between<int>(1, TokenNumber(), 30);}
    else if (Token("movetime"))  {Think(TokenNumber()); TokenPop(); print_best_move(); return;}
    else if (Token("depth"))     {g_max_depth = Between<int>(1, TokenNumber(), kDepthLimit); Think(kInf); g_max_depth = kDepthLimit; TokenPop(); print_best_move(); return;}
  }
  Think(std::max(0, g_wtm ? wtime / mtg + winc : btime / mtg + binc));
  print_best_move();
}

void UciUci() {
  std::cout << "id name " << kName << std::endl;
  std::cout << "id author Toni Helminen" << std::endl;
  std::cout << "option name UCI_Chess960 type check default " << (g_chess960 ? "true" : "false") << std::endl;
  std::cout << "option name Level type spin default " << g_level << " min 0 max 10" << std::endl;
  std::cout << "option name Hash type spin default " << g_hash_mb << " min 1 max 1048576" << std::endl;
  std::cout << "option name MoveOverhead type spin default " << g_move_overhead << " min 0 max 5000" << std::endl;
  std::cout << "option name EvalFile type string default " << g_eval_file << std::endl;
  std::cout << "option name BookFile type string default " << g_book_file << std::endl;
  std::cout << "uciok" << std::endl;
}

bool UciCommands() {
  if (TokenOk()) {
    if (Token("position"))       {UciPosition();}
    else if (Token("go"))        {UciGo();}
    else if (Token("isready"))   {std::cout << "readyok" << std::endl;}
    else if (Token("setoption")) {UciSetoption();}
    else if (Token("uci"))       {UciUci();}
    else if (Token("quit"))      {return false;}
  }
  while (TokenOk()) TokenPop();
  return true;
}

bool Uci() {
  Input();
  return UciCommands();
}

// Init

std::uint64_t PermutateBb(const std::uint64_t moves, const int index) {
  int total = 0, good[64] = {};
  std::uint64_t permutations = 0;
  for (auto i = 0; i < 64; i++) {if (moves & Bit(i)) good[total++] = i;}
  const int popn = PopCount(moves);
  for (auto i = 0; i < popn; i++) {if ((1 << i) & index) permutations |= Bit(good[i]);}
  return permutations & moves;
}

std::uint64_t MakeSliderMagicMoves(const int *slider_vectors, const int sq, const std::uint64_t moves) {
  std::uint64_t possible_moves = 0;
  const auto x_pos = Xcoord(sq), y_pos = Ycoord(sq);
  for (auto i = 0; i < 4; i++)
    for (auto j = 1; j < 8; j++) {
      const auto x = x_pos + j * slider_vectors[2 * i], y = y_pos + j * slider_vectors[2 * i + 1];
      if (!OnBoard(x, y)) break;
      const auto tmp = Bit(8 * y + x);
      possible_moves |= tmp;
      if (tmp & moves) break;
    }
  return possible_moves & (~Bit(sq));
}

void InitBishopMagics() {
  for (auto i = 0; i < 64; i++) {
    const auto magics = kBishopMoveMagics[i] & (~Bit(i));
    for (auto j = 0; j < 512; j++) {
      const auto allmoves = PermutateBb(magics, j);
      g_bishop_magic_moves[i][BishopMagicIndex(i, allmoves)] = MakeSliderMagicMoves(kBishopVectors, i, allmoves);
    }
  }
}

void InitRookMagics() {
  for (auto i = 0; i < 64; i++) {
    const auto magics = kRookMoveMagic[i] & (~Bit(i));
    for (auto j = 0; j < 4096; j++) {
      const auto allmoves = PermutateBb(magics, j);
      g_rook_magic_moves[i][RookMagicIndex(i, allmoves)] = MakeSliderMagicMoves(kRookVectors, i, allmoves);
    }
  }
}

std::uint64_t MakeSliderMoves(const int sq, const int *slider_vectors) {
  std::uint64_t moves = 0;
  const auto x_pos = Xcoord(sq), y_pos = Ycoord(sq);
  for (auto i = 0; i < 4; i++) {
    const auto dx = slider_vectors[2 * i], dy = slider_vectors[2 * i + 1];
    std::uint64_t tmp = 0;
    for (auto j = 1; j < 8; j++) {
      const auto x = x_pos + j * dx, y = y_pos + j * dy;
      if (!OnBoard(x, y)) break;
      tmp |= Bit(8 * y + x);
    }
    moves |= tmp;
  }
  return moves;
}

void InitSliderMoves() {
  for (auto i = 0; i < 64; i++) {
    g_rook_moves[i]   = MakeSliderMoves(i, kRookVectors);
    g_bishop_moves[i] = MakeSliderMoves(i, kBishopVectors);
    g_queen_moves[i]  = g_rook_moves[i] | g_bishop_moves[i];
  }
}

std::uint64_t MakeJumpMoves(const int sq, const int len, const int dy, const int *jump_vectors) {
  std::uint64_t moves = 0;
  const auto x_pos = Xcoord(sq), y_pos = Ycoord(sq);
  for (auto i = 0; i < len; i++) {
    const auto x = x_pos + jump_vectors[2 * i], y = y_pos + dy * jump_vectors[2 * i + 1];
    if (OnBoard(x, y)) moves |= Bit(8 * y + x);
  }
  return moves;
}

void InitJumpMoves() {
  constexpr int pawn_check_vectors[2 * 2] = {-1,1,1,1}, pawn_1_vectors[1 * 2] = {0,1};
  for (auto i = 0; i < 64; i++) {
    g_king_moves[i]     = MakeJumpMoves(i, 8,  1, kKingVectors);
    g_knight_moves[i]   = MakeJumpMoves(i, 8,  1, kKnightVectors);
    g_pawn_checks_w[i]  = MakeJumpMoves(i, 2,  1, pawn_check_vectors);
    g_pawn_checks_b[i]  = MakeJumpMoves(i, 2, -1, pawn_check_vectors);
    g_pawn_1_moves_w[i] = MakeJumpMoves(i, 1,  1, pawn_1_vectors);
    g_pawn_1_moves_b[i] = MakeJumpMoves(i, 1, -1, pawn_1_vectors);
  }
  for (auto i = 0; i < 8; i++) {
    g_pawn_2_moves_w[ 8 + i] = MakeJumpMoves( 8 + i, 1, +1, pawn_1_vectors) | MakeJumpMoves( 8 + i, 1, +2, pawn_1_vectors);
    g_pawn_2_moves_b[48 + i] = MakeJumpMoves(48 + i, 1, -1, pawn_1_vectors) | MakeJumpMoves(48 + i, 1, -2, pawn_1_vectors);
  }
}

void InitDraws() {
  constexpr int draws[6 * 4] = {1,0,0,0 ,0,1,0,0, 2,0,0,0, 1,0,0,1, 2,0,1,0, 2,0,0,1}; // KNK, KBK, KNNK, KNKB, KNNKN, KNNKB
  std::size_t len = 0;
  for (auto i = 0; i < 6; i++) {
    g_easy_draws[len++] = DrawKey(draws[4 * i    ], draws[4 * i + 1], draws[4 * i + 2], draws[4 * i + 3]);
    g_easy_draws[len++] = DrawKey(draws[4 * i + 2], draws[4 * i + 3], draws[4 * i    ], draws[4 * i + 1]);
  }
  g_easy_draws[len++] = DrawKey(0,1,0,1); // KBKB
}

void InitZobrist() {
  for (auto i = 0; i < 13; i++) for (auto j = 0; j < 64; j++) g_zobrist_board[i][j] = Random8x64();
  for (auto i = 0; i < 64; i++) g_zobrist_ep[i]     = Random8x64();
  for (auto i = 0; i < 16; i++) g_zobrist_castle[i] = Random8x64();
  for (auto i = 0; i <  2; i++) g_zobrist_wtm[i]    = Random8x64();
}

void Init() {
  if (g_seed != 131783) return; // Init just once
  g_seed += std::time(nullptr);
  InitBishopMagics();
  InitRookMagics();
  InitZobrist();
  InitDraws();
  InitSliderMoves();
  InitJumpMoves();
  Fen(kStartpos);
  SetupHashtable();
  SetupNNUE();
  SetupBook();
}

void Bench() {
  const std::uint64_t start = Now();
  std::uint64_t nodes = 0;
  const std::vector<std::string> suite = {
    "brnnkbrq/pppppppp/8/8/8/8/PPPPPPPP/BRNNKBRQ w GBgb - 0 1",
    "qnnrkrbb/pppppppp/8/8/8/8/PPPPPPPP/QNNRKRBB w DFdf - 0 1",
    "nqbnrbkr/pppppppp/8/8/8/8/PPPPPPPP/NQBNRBKR w EHeh - 0 1"
  };
  for (const auto fen : suite) {
    std::cout << "[ " << fen << " ]" << std::endl;
    Fen(fen);
    Think(10000);
    nodes += g_nodes;
    std::cout << std::endl;
  }
  std::cout << "===\n\n" << "Nps: " << Nps(nodes, Now() - start) << std::endl;
}

void PrintVersion() {std::cout << kName << " by Toni Helminen" << std::endl;}
void UciLoop() {PrintVersion(); while (Uci());}
void PrintHelp() {std::cout << "> mayhem: Enter UCI mode\n--version: Show version\n--bench: Run benchmarks\n-list [FEN]: Show root list\n-eval [FEN]: Show evaluation" << std::endl;}
void PrintList(const std::string &fen) {Fen(fen); MgenRoot(); EvalRootMoves(); SortAll(); for (auto i = 0; i < g_root_n; i++) std::cout << i << ": " << MoveName(g_root + i) << " : " << g_root[i].score << std::endl;}
void PrintEval(const std::string &fen) {Fen(fen); std::cout << Evaluate(g_wtm) << std::endl;}}
