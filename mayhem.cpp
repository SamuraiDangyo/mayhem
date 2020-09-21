/*
Mayhem is just (an experimental engine) Sapeli 1.90 written in C++14
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

// Headers

#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

// Namespace

namespace mayhem {

// Constants

constexpr char
  kName[]     = "Mayhem 0.13",
  kStartpos[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

constexpr int
  kMaxMoves        = 218,
  kMoveOverhead    = 1,
  kDepthLimit      = 30,
  kInf             = 1048576,
  kTempoBonus      = 50,
  kKingVectors[]   = {1,0,0,1,0,-1,-1,0,1,1,-1,-1,1,-1,-1,1},
  kKnightVectors[] = {2,1,-2,1,2,-1,-2,-1,1,2,-1,2,1,-2,-1,-2},
  kBishopVectors[] = {1,1,-1,-1,1,-1,-1,1},
  kRookVectors[]   = {1,0,0,1,0,-1,-1,0},
  kPieceMg[]       = {1000, 3200, 3300, 5500, 11050, 0},
  kPieceEg[]       = {1250, 3750, 3970, 5970, 12000, 0},
  kAttacks[][6]    = {{2,6,6,7,11,12},{1,5,5,6,12,15},{1,5,5,8,16,22},{1,4,4,5,10,19},{1,3,3,4,7,17},{1,2,2,3,4,15}},
  kCenter[]        = {-2,-1,1,2,2,1,-1,-2,-1,0,2,3,3,2,0,-1,1,2,4,5,5,4,2,1,2,3,5,6,6,5,3,2,2,3,5,6,6,5,3,2,1,2,4,5,5,4,2,1,-1,0,2,3,3,2,0,-1,-2,-1,1,2,2,1,-1,-2},
  kPstMg[6][64]    =
    {{0,0,0,0,0,0,0,0,22,35,21,9,9,21,35,22,28,21,-20,25,25,-20,21,28,-6,-27,-15,95,95,-15,-27,-6,-26,-36,-21,-22,-22,-21,-36,-26,3,5,6,8,8,6,5,3,15,17,16,19,19,16,17,15,0,0,0,0,0,0,0,0},
    {-236,-103,-97,-69,-69,-97,-103,-236,-6,0,6,9,9,6,0,-6,-1,7,12,15,15,12,7,-1,6,9,35,18,18,35,9,6,6,10,50,73,73,50,10,6,1,6,12,50,50,12,6,1,-5,0,6,9,9,6,0,-5,-141,-28,-22,6,6,-22,-28,-141},
    {-161,-38,-34,-16,-16,-34,-38,-161,-5,30,6,9,9,6,30,-5,2,16,12,15,15,12,16,2,6,9,16,18,18,16,9,6,6,9,15,19,19,15,9,6,3,6,12,16,16,12,6,3,-4,0,6,9,9,6,0,-4,-141,-11,-7,6,6,-7,-11,-141},
    {-96,-3,48,71,71,48,-3,-96,-5,3,11,44,44,11,3,-5,3,6,12,40,40,12,6,3,6,9,15,18,18,15,9,6,6,9,15,18,18,15,9,6,3,6,12,15,15,12,6,3,52,55,61,64,64,61,55,52,-6,13,23,26,26,23,13,-6},
    {-96,-3,2,7,7,2,-3,-96,-3,3,6,8,8,6,3,-3,3,6,12,15,15,12,6,3,6,9,15,18,18,15,9,6,6,9,16,19,19,16,9,6,3,6,12,15,15,12,6,3,-3,0,6,8,8,6,0,-3,-36,-3,3,6,6,3,-3,-36},
    {-56,100,3,-116,-116,3,100,-56,-3,0,-75,-79,-79,-75,0,-3,3,6,12,15,15,12,6,3,6,9,15,19,19,15,9,6,6,9,15,18,18,15,9,6,3,6,12,15,15,12,6,3,-3,0,6,9,9,6,0,-3,-16,-3,3,6,6,3,-3,-16}},
  kPstEg[6][64]    =
    {{0,0,0,0,0,0,0,0,-5,0,10,15,15,10,0,-5,5,10,20,25,25,20,10,5,45,50,60,65,65,60,50,45,259,264,274,279,279,274,264,259,725,730,740,745,745,740,730,725,995,1000,1010,1015,1015,1010,1000,995,0,0,0,0,0,0,0,0},
    {-195,-30,-20,5,5,-20,-30,-195,-3,0,10,15,15,10,0,-3,-10,10,20,25,25,20,10,-10,10,18,25,30,30,25,18,10,10,15,28,35,35,28,15,10,-8,10,20,25,25,20,10,-8,-2,0,10,15,15,10,0,-2,-155,-10,-5,10,10,-5,-10,-155},
    {-220,-55,-45,-15,-15,-45,-55,-220,-15,0,10,15,15,10,0,-15,-10,10,20,25,25,20,10,-10,10,15,25,30,30,25,15,10,15,15,25,30,30,25,15,15,-5,10,20,25,25,20,10,-5,-10,0,10,15,15,10,0,-10,-155,-30,-20,10,10,-20,-30,-155},
    {-70,-5,5,10,10,5,-5,-70,-5,0,10,15,15,10,0,-5,5,10,20,25,25,20,10,5,10,15,25,30,30,25,15,10,10,15,25,30,30,25,15,10,5,10,20,25,25,20,10,5,0,5,15,20,20,15,5,0,-30,-5,5,10,10,5,-5,-30},
    {-50,-1,7,10,10,7,-1,-50,-2,7,10,15,15,10,7,-2,5,10,20,25,25,20,10,5,10,15,25,50,50,25,15,10,10,15,25,55,55,25,15,10,5,10,20,25,25,20,10,5,-2,7,10,15,15,10,7,-2,-20,-1,8,10,10,8,-1,-20},
    {-70,-25,5,7,7,5,-25,-70,-5,0,10,15,15,10,0,-5,5,10,20,25,25,20,10,5,10,15,35,55,55,35,15,10,10,15,25,57,57,25,15,10,5,10,20,27,27,20,10,5,-5,0,10,15,15,10,0,-5,-30,-5,5,10,10,5,-5,-30}},
  kMvv[6][6]       = {{85,96,97,98,99,100},{84,86,93,94,95,100},{82,83,87,91,92,100},{79,80,81,88,90,100},{75,76,77,78,89,100},{70,71,72,73,74,100}};

constexpr uint64_t
  kFreeColumns[8]       =
    {0x0202020202020202ULL,0x0505050505050505ULL,0x0A0A0A0A0A0A0A0AULL,0x1414141414141414ULL,0x2828282828282828ULL,0x5050505050505050ULL,0xA0A0A0A0A0A0A0A0ULL,0x4040404040404040ULL},
  kRookMagic[64]        =
    {0x548001400080106cULL,0x900184000110820ULL,0x428004200a81080ULL,0x140088082000c40ULL,0x1480020800011400ULL,0x100008804085201ULL,0x2a40220001048140ULL,0x50000810000482aULL,
     0x250020100020a004ULL,0x3101880100900a00ULL,0x200a040a00082002ULL,0x1004300044032084ULL,0x2100408001013ULL,0x21f00440122083ULL,0xa204280406023040ULL,0x2241801020800041ULL,
     0xe10100800208004ULL,0x2010401410080ULL,0x181482000208805ULL,0x4080101000021c00ULL,0xa250210012080022ULL,0x4210641044000827ULL,0x8081a02300d4010ULL,0x8008012000410001ULL,
     0x28c0822120108100ULL,0x500160020aa005ULL,0xc11050088c1000ULL,0x48c00101000a288ULL,0x494a184408028200ULL,0x20880100240006ULL,0x10b4010200081ULL,0x40a200260000490cULL,
     0x22384003800050ULL,0x7102001a008010ULL,0x80020c8010900c0ULL,0x100204082a001060ULL,0x8000118188800428ULL,0x58e0020009140244ULL,0x100145040040188dULL,0x44120220400980ULL,
     0x114001007a00800ULL,0x80a0100516304000ULL,0x7200301488001000ULL,0x1000151040808018ULL,0x3000a200010e0020ULL,0x1000849180802810ULL,0x829100210208080ULL,0x1004050021528004ULL,
     0x61482000c41820b0ULL,0x241001018a401a4ULL,0x45020c009cc04040ULL,0x308210c020081200ULL,0xa000215040040ULL,0x10a6024001928700ULL,0x42c204800c804408ULL,0x30441a28614200ULL,
     0x40100229080420aULL,0x9801084000201103ULL,0x8408622090484202ULL,0x4022001048a0e2ULL,0x280120020049902ULL,0x1200412602009402ULL,0x914900048020884ULL,0x104824281002402ULL},
  kRookMask[64]         =
    {0x101010101017eULL,0x202020202027cULL,0x404040404047aULL,0x8080808080876ULL,0x1010101010106eULL,0x2020202020205eULL,0x4040404040403eULL,0x8080808080807eULL,
     0x1010101017e00ULL,0x2020202027c00ULL,0x4040404047a00ULL,0x8080808087600ULL,0x10101010106e00ULL,0x20202020205e00ULL,0x40404040403e00ULL,0x80808080807e00ULL,
     0x10101017e0100ULL,0x20202027c0200ULL,0x40404047a0400ULL,0x8080808760800ULL,0x101010106e1000ULL,0x202020205e2000ULL,0x404040403e4000ULL,0x808080807e8000ULL,
     0x101017e010100ULL,0x202027c020200ULL,0x404047a040400ULL,0x8080876080800ULL,0x1010106e101000ULL,0x2020205e202000ULL,0x4040403e404000ULL,0x8080807e808000ULL,
     0x1017e01010100ULL,0x2027c02020200ULL,0x4047a04040400ULL,0x8087608080800ULL,0x10106e10101000ULL,0x20205e20202000ULL,0x40403e40404000ULL,0x80807e80808000ULL,
     0x17e0101010100ULL,0x27c0202020200ULL,0x47a0404040400ULL,0x8760808080800ULL,0x106e1010101000ULL,0x205e2020202000ULL,0x403e4040404000ULL,0x807e8080808000ULL,
     0x7e010101010100ULL,0x7c020202020200ULL,0x7a040404040400ULL,0x76080808080800ULL,0x6e101010101000ULL,0x5e202020202000ULL,0x3e404040404000ULL,0x7e808080808000ULL,
     0x7e01010101010100ULL,0x7c02020202020200ULL,0x7a04040404040400ULL,0x7608080808080800ULL,0x6e10101010101000ULL,0x5e20202020202000ULL,0x3e40404040404000ULL,0x7e80808080808000ULL},
  kRookMoveMagic[64]    =
    {0x101010101017eULL,0x202020202027cULL,0x404040404047aULL,0x8080808080876ULL,0x1010101010106eULL,0x2020202020205eULL,0x4040404040403eULL,0x8080808080807eULL,
     0x1010101017e00ULL,0x2020202027c00ULL,0x4040404047a00ULL,0x8080808087600ULL,0x10101010106e00ULL,0x20202020205e00ULL,0x40404040403e00ULL,0x80808080807e00ULL,
     0x10101017e0100ULL,0x20202027c0200ULL,0x40404047a0400ULL,0x8080808760800ULL,0x101010106e1000ULL,0x202020205e2000ULL,0x404040403e4000ULL,0x808080807e8000ULL,
     0x101017e010100ULL,0x202027c020200ULL,0x404047a040400ULL,0x8080876080800ULL,0x1010106e101000ULL,0x2020205e202000ULL,0x4040403e404000ULL,0x8080807e808000ULL,
     0x1017e01010100ULL,0x2027c02020200ULL,0x4047a04040400ULL,0x8087608080800ULL,0x10106e10101000ULL,0x20205e20202000ULL,0x40403e40404000ULL,0x80807e80808000ULL,
     0x17e0101010100ULL,0x27c0202020200ULL,0x47a0404040400ULL,0x8760808080800ULL,0x106e1010101000ULL,0x205e2020202000ULL,0x403e4040404000ULL,0x807e8080808000ULL,
     0x7e010101010100ULL,0x7c020202020200ULL,0x7a040404040400ULL,0x76080808080800ULL,0x6e101010101000ULL,0x5e202020202000ULL,0x3e404040404000ULL,0x7e808080808000ULL,
     0x7e01010101010100ULL,0x7c02020202020200ULL,0x7a04040404040400ULL,0x7608080808080800ULL,0x6e10101010101000ULL,0x5e20202020202000ULL,0x3e40404040404000ULL,0x7e80808080808000ULL},
  kBishopMagic[64]      =
    {0x2890208600480830ULL,0x324148050f087ULL,0x1402488a86402004ULL,0xc2210a1100044bULL,0x88450040b021110cULL,0xc0407240011ULL,0xd0246940cc101681ULL,0x1022840c2e410060ULL,
     0x4a1804309028d00bULL,0x821880304a2c0ULL,0x134088090100280ULL,0x8102183814c0208ULL,0x518598604083202ULL,0x67104040408690ULL,0x1010040020d000ULL,0x600001028911902ULL,
     0x8810183800c504c4ULL,0x2628200121054640ULL,0x28003000102006ULL,0x4100c204842244ULL,0x1221c50102421430ULL,0x80109046e0844002ULL,0xc128600019010400ULL,0x812218030404c38ULL,
     0x1224152461091c00ULL,0x1c820008124000aULL,0xa004868015010400ULL,0x34c080004202040ULL,0x200100312100c001ULL,0x4030048118314100ULL,0x410000090018ULL,0x142c010480801ULL,
     0x8080841c1d004262ULL,0x81440f004060406ULL,0x400a090008202ULL,0x2204020084280080ULL,0xb820060400008028ULL,0x110041840112010ULL,0x8002080a1c84400ULL,0x212100111040204aULL,
     0x9412118200481012ULL,0x804105002001444cULL,0x103001280823000ULL,0x40088e028080300ULL,0x51020d8080246601ULL,0x4a0a100e0804502aULL,0x5042028328010ULL,0xe000808180020200ULL,
     0x1002020620608101ULL,0x1108300804090c00ULL,0x180404848840841ULL,0x100180040ac80040ULL,0x20840000c1424001ULL,0x82c00400108800ULL,0x28c0493811082aULL,0x214980910400080cULL,
     0x8d1a0210b0c000ULL,0x164c500ca0410cULL,0xc6040804283004ULL,0x14808001a040400ULL,0x180450800222a011ULL,0x600014600490202ULL,0x21040100d903ULL,0x10404821000420ULL},
  kBishopMask[64]       =
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

typedef struct {
  uint64_t
    white[6],   // White bitboards
    black[6],   // Black bitboards
    hash;       // Hash signature
  int
    score;      // Sorting
  char
    pieces[64], // Pieces black and white
    epsq;       // En passant square
  uint8_t
    index,      // Sorting
    from,       // From square
    to,         // To square
    type,       // Move type (0: Normal, 1: OOw, 2: OOOw, 3: OOb, 4: OOOb, 5: =n, 6: =b, 7: =r, 8: =q)
    castle,     // Castling rights
    rule50;     // Rule 50 counter
} Board;

typedef struct {
  uint64_t hash;
  uint8_t index;
} Sort;

typedef struct {
  uint64_t hash;
  int score;
} Hash;

// Global Variables

int
  g_level = 100;
uint64_t
  g_seed = 131783;
bool
  g_uci = 0, g_uci_chess960 = 0;
unsigned int
  g_tokens_nth = 0;
std::vector<std::string>
  g_tokens = {};

// Eval Variables

uint64_t
  e_white = 0, e_black = 0, e_empty = 0, e_both = 0, e_columns_down[64] = {0}, e_black_material = 0,  e_white_material = 0, e_king_ring[64] = {0}, e_columns_up[64] = {0};
int
  e_pos_mg = 0, e_pos_eg = 0, e_mat_mg = 0, e_mat_eg = 0, e_white_king_sq = 0, e_black_king_sq = 0, e_both_n = 0, e_pst_mg_b[6][64] = {{0}}, e_pst_eg_b[6][64] = {{0}};

// Move Generator Variables

constexpr Board
  m_board_empty = {{0},{0},0,0,{0},0,0,0,0,0,0,0};
Board
  m_board_tmp = {{0},{0},0,0,{0},0,0,0,0,0,0,0}, *m_board = &m_board_tmp, *m_moves = 0, *m_board_orig = 0, m_root[kMaxMoves] = {{{0},{0},0,0,{0},0,0,0,0,0,0,0}};
uint64_t
  m_black = 0, m_both = 0, m_empty = 0, m_good = 0, m_pawn_sq = 0, m_white = 0, m_pawn_1_moves_w[64] = {0}, m_pawn_1_moves_b[64] = {0}, m_pawn_2_moves_w[64] = {0}, m_pawn_2_moves_b[64] = {0},
  m_bishop[64] = {0}, m_rook[64] = {0}, m_queen[64] = {0}, m_knight[64] = {0}, m_king[64] = {0}, m_pawn_checks_w[64] = {0}, m_pawn_checks_b[64] = {0},
  m_castle_w[2] = {0}, m_castle_b[2] = {0}, m_castle_empty_w[2] = {0}, m_castle_empty_b[2] = {0}, m_bishop_magic_moves[64][512] = {{0}}, m_rook_magic_moves[64][4096] = {{0}};
int
  m_rook_w[2] = {0}, m_rook_b[2] = {0}, m_root_n = 0, m_king_w = 0, m_king_b = 0, m_moves_n = 0;
bool
  m_wtm = 0;

// Zobrist Variables

uint64_t
  z_ep[64] = {0}, z_castle[16] = {0}, z_wtm[2] = {0}, z_board[13][64] = {{0}};

// Search Variables

uint64_t
  s_stop_time = 0, s_sure_draws[15] = {0}, s_r50_positions[128] = {0}, s_nodes = 0;
int
  s_max_depth = kDepthLimit, s_qs_depth = 6, s_depth = 0, s_best_score = 0;
double
  s_r50_factor[128] = {0};
bool
  s_stop = 0;

// Hash Variables

constexpr uint32_t
  kEvalHashKey                   = (1 << 22) - 1,
  kKillerMovesKey                = (1 << 21) - 1,
  kGoodMovesKey                  = (1 << 22) - 1,
  kEgWinsKey                     = (1 << 22) - 1;
Hash
  h_eval[kEvalHashKey + 1]       = {{0,0}};
Sort
  h_killers[kKillerMovesKey + 1] = {{0,0}},
  h_goodmoves[kGoodMovesKey + 1] = {{0,0}};
uint64_t
  h_white_wins[kEgWinsKey + 1]   = {0},
  h_black_wins[kEgWinsKey + 1]   = {0};

// Function Prototypes

int SearchB(const int, int, const int, const int);
int QSearchB(const int, int, const int);
int Eval(const bool);
void MakeMove();
uint64_t RookMagicMoves(const int, const uint64_t);
uint64_t BishopMagicMoves(const int, const uint64_t);

// Utils

uint64_t White() {return m_board->white[0] | m_board->white[1] | m_board->white[2] | m_board->white[3] | m_board->white[4] | m_board->white[5];}
uint64_t Black() {return m_board->black[0] | m_board->black[1] | m_board->black[2] | m_board->black[3] | m_board->black[4] | m_board->black[5];}
uint64_t Both() {return White() | Black();}
uint8_t Xcoord(const uint64_t bb) {return bb & 7;}
uint8_t Ycoord(const uint64_t bb) {return bb >> 3;}
bool Underpromo() {return m_board->type >= 5 && m_board->type <= 7;}
template <class Type> Type Between(const Type val_a, const Type val_b, const Type val_c) {return std::max(val_a, std::min(val_b, val_c));}
inline unsigned int Lsb(const uint64_t bb) {return __builtin_ctzll(bb);}
inline unsigned int Popcount(const uint64_t bb) {return __builtin_popcountll(bb);}
inline uint64_t ClearBit(const uint64_t bb) {return bb & (bb - 1);}
uint32_t Nps(const uint64_t nodes, const uint32_t ms) {return (1000 * nodes) / (ms + 1);}
inline uint64_t Bit(const int nbits) {return 0x1ULL << nbits;}
int Mirror(const int pos) {return pos ^ 56;}
void Assert(const bool test, const std::string msg,  const int line_number) {if (test) return; std::cerr << "Error ( " << line_number << " ): " << msg << std::endl; exit(EXIT_FAILURE);}
const std::string MoveStr(const int from, const int to) {char str[5]; str[0] = 'a' + Xcoord(from), str[1] = '1' + Ycoord(from), str[2] = 'a' + Xcoord(to), str[3] = '1' + Ycoord(to), str[4] = '\0'; return std::string(str);}
uint64_t Now() {struct timeval tv; if (gettimeofday(&tv, NULL)) return 0; return (uint64_t) (1000 * tv.tv_sec + tv.tv_usec / 1000);}
bool StopSearch() {return Now() >= s_stop_time;}
uint64_t RandomBB() {
  static uint64_t val_a = 0X12311227ULL, val_b = 0X1931311ULL, val_c = 0X13138141ULL;
  auto mixer = [](uint64_t num) {return ((num) << 7) ^ ((num) >> 5);};
  val_a ^= val_b + val_c; val_b ^= val_b * val_c + 0x1717711ULL; val_c  = (3 * val_c) + 1;
  return mixer(val_a) ^ mixer(val_b) ^ mixer(val_c);
}
uint64_t Random8x64() {uint64_t val = 0; for (auto i = 0; i < 8; i++) val ^= RandomBB() << (8 * i); return val;}
int Random1(const int max) {const uint64_t rnum = (g_seed ^ RandomBB()) & 0xFFFFFFFFULL; g_seed = (g_seed << 5) ^ (g_seed + 1) ^ (g_seed >> 3); return (int) (max * (0.0001 * (double) (rnum % 10000)));}
int Random(const int min_val, const int max_val) {return max_val < min_val ? Random(max_val, min_val) : min_val + Random1(max_val - min_val + 1);}
bool OnBoard(const int x, const int y) {return x >= 0 && x <= 7 && y >= 0 && y <= 7;}

template <class Type> void Splitter(const std::string& str, Type& container, const std::string& delims = " ") {
  std::size_t current, previous = 0;
  current = str.find_first_of(delims);
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

const std::string MoveName(const Board *move) {
  int from = move->from, to = move->to;
  switch (move->type) {
  case 1:
    from = m_king_w;
    to   = g_uci_chess960 || !g_uci ? m_rook_w[0] : 6;
    break;
  case 2:
    from = m_king_w;
    to   = g_uci_chess960 || !g_uci ? m_rook_w[1] : 2;
    break;
  case 3:
    from = m_king_b;
    to   = g_uci_chess960 || !g_uci ? m_rook_b[0] : 56 + 6;
    break;
  case 4:
    from = m_king_b;
    to   = g_uci_chess960 || !g_uci ? m_rook_b[1] : 56 + 2;
    break;
  case 5: case 6: case 7: case 8:
    return MoveStr(from, to) + PromoLetter(move->pieces[to]);}
  return MoveStr(from, to);
}

inline uint64_t GenHash(const int wtm) {
  uint64_t hash = z_ep[m_board->epsq + 1] ^ z_wtm[wtm] ^ z_castle[m_board->castle], both = Both();
  for (int pos; both; both = ClearBit(both)) pos = Lsb(both), hash ^= z_board[m_board->pieces[pos] + 6][pos];
  return hash;
}

// Tokenizer

bool TokenOk() {return g_tokens_nth < g_tokens.size();}
const std::string TokenCurrent() {return TokenOk() ? g_tokens[g_tokens_nth] : "";}
bool TokenIs(const std::string token) {return TokenOk() && token == TokenCurrent();}
void TokenPop(const int howmany = 1) {g_tokens_nth += howmany;}
bool Token(const std::string token) {if (!TokenIs(token)) return 0; TokenPop(); return 1;}
int TokenInt() {int val = 0; if (TokenOk()) val = std::stoi(g_tokens[g_tokens_nth]), TokenPop(); return val;}
bool TokenPeek(const std::string str, const int index = 0) {return g_tokens_nth + index < g_tokens.size() ? str == g_tokens[g_tokens_nth + index] : 0;}

// Board

void BuildBitboards() {
  memset(m_board->white, 0, sizeof(m_board->white));
  memset(m_board->black, 0, sizeof(m_board->black));
  for (auto i = 0; i < 64; i++)
    if (     m_board->pieces[i] > 0) m_board->white[ m_board->pieces[i] - 1] |= Bit(i);
    else if (m_board->pieces[i] < 0) m_board->black[-m_board->pieces[i] - 1] |= Bit(i);
}

uint64_t Fill(int from, const int to) {
  uint64_t ret   = Bit(from);
  const int diff = from > to ? -1 : 1;
  if (from < 0 || to < 0 || from > 63 || to > 63) return 0;
  if (from == to) return ret;
  do {
    from += diff;
    ret  |= Bit(from);
  } while (from != to);
  return ret;
}

void FindKings() {
  for (auto i = 0; i < 64; i++)
    if (     m_board->pieces[i] ==  6) m_king_w = i;
    else if (m_board->pieces[i] == -6) m_king_b = i;
}

void FindRank1Rank8Rooks() {
  for (auto i = m_king_w + 1; i <  8; i++) if (m_board->pieces[i] == 4) m_rook_w[0] = i;
  for (auto i = m_king_w - 1; i > -1; i--) if (m_board->pieces[i] == 4) m_rook_w[1] = i;
  for (auto i = m_king_b + 1; i < 64;         i++) if (m_board->pieces[i] == -4) m_rook_b[0] = i;
  for (auto i = m_king_b - 1; i > 64 - 8 - 1; i--) if (m_board->pieces[i] == -4) m_rook_b[1] = i;
}

void FindCastlingRooksAndKings() {
  m_king_w = m_king_b = 0;
  FindKings();
  memset(m_rook_w, 0, sizeof(m_rook_w));
  memset(m_rook_b, 0, sizeof(m_rook_b));
  FindRank1Rank8Rooks();
}

void BuildCastlingBitboards() {
  m_castle_w[0]       = Fill(m_king_w, 6);
  m_castle_w[1]       = Fill(m_king_w, 2);
  m_castle_b[0]       = Fill(m_king_b, 56 + 6);
  m_castle_b[1]       = Fill(m_king_b, 56 + 2);
  m_castle_empty_w[0] = (m_castle_w[0] | Fill(m_rook_w[0], 5     )) ^ (Bit(m_king_w) | Bit(m_rook_w[0]));
  m_castle_empty_b[0] = (m_castle_b[0] | Fill(m_rook_b[0], 56 + 5)) ^ (Bit(m_king_b) | Bit(m_rook_b[0]));
  m_castle_empty_w[1] = (m_castle_w[1] | Fill(m_rook_w[1], 3     )) ^ (Bit(m_king_w) | Bit(m_rook_w[1]));
  m_castle_empty_b[1] = (m_castle_b[1] | Fill(m_rook_b[1], 56 + 3)) ^ (Bit(m_king_b) | Bit(m_rook_b[1]));
  for (auto i = 0; i < 2; i++) {
    m_castle_empty_w[i] &= 0xFFULL;
    m_castle_w[i]       &= 0xFFULL;
    m_castle_empty_b[i] &= 0xFF00000000000000ULL;
    m_castle_b[i]       &= 0xFF00000000000000ULL;
  }
}

int Piece(const char piece) {
  for (auto i = 0; i < 6; i++)
    if (     piece == "pnbrqk"[i]) return -i - 1;
    else if (piece == "PNBRQK"[i]) return  i + 1;
  return 0;
}

void FenBoard(const std::string fen) {
  int pos = 56;
  for (unsigned int i = 0; i < fen.length() && pos >= 0; i++) if (fen[i] == '/') pos -= 16; else if (isdigit(fen[i])) pos += fen[i] - '0'; else m_board->pieces[pos] = Piece(fen[i]), pos++;
}

void FenKQkq(const std::string fen) {
  for (unsigned int i = 0; i < fen.length(); i++)
    if (fen[i] == 'K') {
      m_board->castle |= 1;
    } else if (fen[i] == 'Q') {
      m_board->castle |= 2;
    } else if (fen[i] == 'k') {
      m_board->castle |= 4;
    } else if (fen[i] == 'q') {
      m_board->castle |= 8;
    } else if (fen[i] >= 'A' && fen[i] <= 'H') {
      const int tmp = fen[i] - 'A';
      if (tmp > m_king_w) m_rook_w[0] = tmp, m_board->castle |= 1; else if (tmp < m_king_w) m_rook_w[1] = tmp, m_board->castle |= 2;
    } else if (fen[i] >= 'a' && fen[i] <= 'h') {
      const int tmp = fen[i] - 'a';
      if (tmp > Xcoord(m_king_b)) m_rook_b[0] = 56 + tmp, m_board->castle |= 4; else if (tmp < Xcoord(m_king_b)) m_rook_b[1] = 56 + tmp, m_board->castle |= 8;
    }
}

void FenEp(const std::string fen) {
  if (fen.length() != 2) return;
  m_board->epsq = (fen[0] - 'a') + 8 * (fen[1] - '1');
}

void FenRule50(const std::string fen) {
  if (fen.length() == 0 || fen[0] == '-') return;
  m_board->rule50 = Between<int>(0, std::stoi(fen), 100);
}

void FenGen(const std::string str) {
  std::vector<std::string> fentokens = {};
  Splitter<std::vector<std::string>>(std::string(str), fentokens, " ");
  Assert(fentokens.size() >= 3, "Bad fen", __LINE__);
  if (!fentokens[0].length()) return;
  FenBoard(fentokens[0]);
  if (!fentokens[1].length()) return;
  m_wtm = fentokens[1][0] == 'w';
  if (!fentokens[2].length()) return;
  FindCastlingRooksAndKings();
  FenKQkq(fentokens[2]);
  BuildCastlingBitboards();
  if (fentokens.size() < 4 || !fentokens[3].length()) return;
  FenEp(fentokens[3]);
  if (fentokens.size() < 5 || !fentokens[4].length()) return;
  FenRule50(fentokens[4]);
}

void FenReset() {
  m_board_tmp   = m_board_empty;
  m_board       = &m_board_tmp;
  m_wtm         = 1;
  m_board->epsq = -1;
  m_king_w      = 0;
  m_king_b      = 0;
  memset(m_rook_w, 0, sizeof(m_rook_w));
  memset(m_rook_b, 0, sizeof(m_rook_b));
}

void Fen(const std::string fen) {
  FenReset();
  FenGen(fen);
  BuildBitboards();
  m_board->hash = GenHash(m_wtm);
  Assert(Popcount(m_board->white[5]) == 1 && Popcount(m_board->black[5]) == 1, "Bad fen", __LINE__);
}

// Checks

inline bool ChecksHereW(const int pos) {
  const uint64_t both = Both();
  return ((m_pawn_checks_b[pos]        &  m_board->white[0])                      |
          (m_knight[pos]               &  m_board->white[1])                      |
          (BishopMagicMoves(pos, both) & (m_board->white[2] | m_board->white[4])) |
          (RookMagicMoves(pos, both)   & (m_board->white[3] | m_board->white[4])) |
          (m_king[pos]                 &  m_board->white[5]));
}

inline bool ChecksHereB(const int pos) {
  const uint64_t both = Both();
  return ((m_pawn_checks_w[pos]        &  m_board->black[0])                      |
          (m_knight[pos]               &  m_board->black[1])                      |
          (BishopMagicMoves(pos, both) & (m_board->black[2] | m_board->black[4])) |
          (RookMagicMoves(pos, both)   & (m_board->black[3] | m_board->black[4])) |
          (m_king[pos]                 &  m_board->black[5]));
}

bool ChecksCastleW(uint64_t squares) {for (; squares; squares = ClearBit(squares)) if (ChecksHereW(Lsb(squares))) return 1; return 0;}
bool ChecksCastleB(uint64_t squares) {for (; squares; squares = ClearBit(squares)) if (ChecksHereB(Lsb(squares))) return 1; return 0;}
bool ChecksW() {return ChecksHereW(Lsb(m_board->black[5]));}
bool ChecksB() {return ChecksHereB(Lsb(m_board->white[5]));}

// Move generator

inline uint64_t BishopMagicIndex(const int position, const uint64_t mask) {return ((mask & kBishopMask[position]) * kBishopMagic[position]) >> 55;}
inline uint64_t RookMagicIndex(const int position, const uint64_t mask)   {return ((mask & kRookMask[position]) * kRookMagic[position]) >> 52;}
inline uint64_t BishopMagicMoves(const int position, const uint64_t mask) {return m_bishop_magic_moves[position][BishopMagicIndex(position, mask)];}
inline uint64_t RookMagicMoves(const int position, const uint64_t mask)   {return m_rook_magic_moves[position][RookMagicIndex(position, mask)];}

void HandleCastlingW(const int mtype, const int from, const int to) {
  m_moves[m_moves_n] = *m_board;
  m_board            = &m_moves[m_moves_n];
  m_board->score     = 0;
  m_board->epsq      = -1;
  m_board->from      = from;
  m_board->to        = to;
  m_board->type      = mtype;
  m_board->castle   &= 4 | 8;
  m_board->rule50    = 0;
}

void AddCastleOOW() {
  if (ChecksCastleB(m_castle_w[0])) return;
  HandleCastlingW(1, m_king_w, 6);
  m_board->pieces[m_rook_w[0]] = 0;
  m_board->pieces[m_king_w]    = 0;
  m_board->pieces[5]           = 4;
  m_board->pieces[6]           = 6;
  m_board->white[3]            = (m_board->white[3] ^ Bit(m_rook_w[0])) | Bit(5);
  m_board->white[5]            = (m_board->white[5] ^ Bit(m_king_w))    | Bit(6);
  if (ChecksB()) return;
  m_moves_n++;
}

void AddCastleOOOW() {
  if (ChecksCastleB(m_castle_w[1])) return;
  HandleCastlingW(2, m_king_w, 2);
  m_board->pieces[m_rook_w[1]] = 0;
  m_board->pieces[m_king_w]    = 0;
  m_board->pieces[3]           = 4;
  m_board->pieces[2]           = 6;
  m_board->white[3]            = (m_board->white[3] ^ Bit(m_rook_w[1])) | Bit(3);
  m_board->white[5]            = (m_board->white[5] ^ Bit(m_king_w))    | Bit(2);
  if (ChecksB()) return;
  m_moves_n++;
}

void MgenCastlingMovesW() {
  if ((m_board->castle & 1) && !(m_castle_empty_w[0] & m_both)) {AddCastleOOW();  m_board = m_board_orig;}
  if ((m_board->castle & 2) && !(m_castle_empty_w[1] & m_both)) {AddCastleOOOW(); m_board = m_board_orig;}
}

void HandleCastlingB(const int mtype, const int from, const int to) {
  m_moves[m_moves_n] = *m_board;
  m_board            = &m_moves[m_moves_n];
  m_board->score     = 0;
  m_board->epsq      = -1;
  m_board->from      = from;
  m_board->to        = to;
  m_board->type      = mtype;
  m_board->castle   &= 1 | 2;
  m_board->rule50    = 0;
}

void AddCastleOOB() {
  if (ChecksCastleW(m_castle_b[0])) return;
  HandleCastlingB(3, m_king_b, 56 + 6);
  m_board->pieces[m_rook_b[0]] = 0;
  m_board->pieces[m_king_b]    = 0;
  m_board->pieces[56 + 5]      = -4;
  m_board->pieces[56 + 6]      = -6;
  m_board->black[3]            = (m_board->black[3] ^ Bit(m_rook_b[0])) | Bit(56 + 5);
  m_board->black[5]            = (m_board->black[5] ^ Bit(m_king_b))    | Bit(56 + 6);
  if (ChecksW()) return;
  m_moves_n++;
}

void AddCastleOOOB() {
  if (ChecksCastleW(m_castle_b[1])) return;
  HandleCastlingB(4, m_king_b, 56 + 2);
  m_board->pieces[m_rook_b[1]] = 0;
  m_board->pieces[m_king_b]    = 0;
  m_board->pieces[56 + 3]      = -4;
  m_board->pieces[56 + 2]      = -6;
  m_board->black[3]            = (m_board->black[3] ^ Bit(m_rook_b[1])) | Bit(56 + 3);
  m_board->black[5]            = (m_board->black[5] ^ Bit(m_king_b))    | Bit(56 + 2);
  if (ChecksW()) return;
  m_moves_n++;
}

void MgenCastlingMovesB() {
  if ((m_board->castle & 4) && !(m_castle_empty_b[0] & m_both)) AddCastleOOB(),  m_board = m_board_orig;
  if ((m_board->castle & 8) && !(m_castle_empty_b[1] & m_both)) AddCastleOOOB(), m_board = m_board_orig;
}

void CheckCastlingRightsW() {
  if (m_board->pieces[m_king_w]    != 6) {m_board->castle &= 4 | 8; return;}
  if (m_board->pieces[m_rook_w[0]] != 4)  m_board->castle &= 2 | 4 | 8;
  if (m_board->pieces[m_rook_w[1]] != 4)  m_board->castle &= 1 | 4 | 8;
}

void CheckCastlingRightsB() {
  if (m_board->pieces[m_king_b]    != -6) {m_board->castle &= 1 | 2; return;}
  if (m_board->pieces[m_rook_b[0]] != -4)  m_board->castle &= 1 | 2 | 8;
  if (m_board->pieces[m_rook_b[1]] != -4)  m_board->castle &= 1 | 2 | 4;
}

void HandleCastlingRights() {
  if (!m_board->castle) return;
  CheckCastlingRightsW();
  CheckCastlingRightsB();
}

void ModifyPawnStuffW(const int from, const int to) {
  m_board->rule50 = 0;
  if (to == m_board_orig->epsq) {
    m_board->score          = 85;
    m_board->pieces[to - 8] = 0;
    m_board->black[0]      ^= Bit(to - 8);
  } else if (Ycoord(to) - Ycoord(from) == 2) {
    m_board->epsq = to - 8;
  } else if (Ycoord(to) == 6) {
    m_board->score = 102;
  }
}

void AddPromotionW(const int from, const int to, const int piece, const int score) {
  const int eat = m_board->pieces[to];
  m_moves[m_moves_n]    = *m_board;
  m_board               = &m_moves[m_moves_n];
  m_board->from         = from;
  m_board->to           = to;
  m_board->score        = score;
  m_board->type         = 3 + piece;
  m_board->epsq         = -1;
  m_board->rule50       = 0;
  m_board->pieces[to]   = piece;
  m_board->pieces[from] = 0;
  m_board->white[0]    ^= Bit(from);
  m_board->white[piece - 1] |= Bit(to);
  if (eat <= -1) m_board->black[-eat - 1] ^= Bit(to);
  if (ChecksB()) return;
  HandleCastlingRights();
  m_moves_n++;
}

void AddPromotionStuffW(const int from, const int to) {
  Board *tmp = m_board;
  for (int piece = 2; piece <= 5; piece++) AddPromotionW(from, to, piece, piece == 5 ? 100 : 0), m_board = tmp;
}

void AddNormalStuffW(const int from, const int to) {
  const int me = m_board->pieces[from], eat = m_board->pieces[to];
  if (me <= 0) return;
  m_moves[m_moves_n]     = *m_board;
  m_board                = &m_moves[m_moves_n];
  m_board->from          = from;
  m_board->to            = to;
  m_board->score         = 0;
  m_board->type          = 0;
  m_board->epsq          = -1;
  m_board->pieces[from]  = 0;
  m_board->pieces[to]    = me;
  m_board->white[me - 1] = (m_board->white[me - 1] ^ Bit(from)) | Bit(to);
  m_board->rule50++;
  if (eat <= -1) {
    m_board->black[-eat - 1] ^= Bit(to);
    m_board->score            = kMvv[me - 1][-eat - 1];
    m_board->rule50           = 0;
  }
  if (m_board->pieces[to] == 1) ModifyPawnStuffW(from, to);
  if (ChecksB()) return;
  HandleCastlingRights();
  m_moves_n++;
}


void ModifyPawnStuffB(const int from, const int to) {
  m_board->rule50 = 0;
  if (to == m_board_orig->epsq) {
    m_board->score          = 85;
    m_board->pieces[to + 8] = 0;
    m_board->white[0]      ^= Bit(to + 8);
  } else if (Ycoord(to) - Ycoord(from) == -2) {
    m_board->epsq = to + 8;
  } else if (Ycoord(to) == 1) {
    m_board->score = 102;
  }
}

void AddNormalStuffB(const int from, const int to) {
  const int me = m_board->pieces[from], eat = m_board->pieces[to];
  if (me >= 0) return;
  m_moves[m_moves_n]      = *m_board;
  m_board                 = &m_moves[m_moves_n];
  m_board->from           = from;
  m_board->to             = to;
  m_board->score          = 0;
  m_board->type           = 0;
  m_board->epsq           = -1;
  m_board->pieces[to]     = me;
  m_board->pieces[from]   = 0;
  m_board->black[-me - 1] = (m_board->black[-me - 1] ^ Bit(from)) | Bit(to);
  m_board->rule50++;
  if (eat >= 1) {
    m_board->score           = kMvv[-me - 1][eat - 1];
    m_board->rule50          = 0;
    m_board->white[eat - 1] ^= Bit(to);
  }
  if (m_board->pieces[to] == -1) ModifyPawnStuffB(from, to);
  if (ChecksW()) return;
  HandleCastlingRights();
  m_moves_n++;
}

void AddPromotionB(const int from, const int to, const int piece, const int score) {
  const int eat         = m_board->pieces[to];
  m_moves[m_moves_n]    = *m_board;
  m_board               = &m_moves[m_moves_n];
  m_board->from         = from;
  m_board->to           = to;
  m_board->score        = score;
  m_board->type         = 3 + (-piece);
  m_board->epsq         = -1;
  m_board->rule50       = 0;
  m_board->pieces[from] = 0;
  m_board->pieces[to]   = piece;
  m_board->black[0]    ^= Bit(from);
  m_board->black[-piece - 1] |= Bit(to);
  if (eat >= 1) m_board->white[eat - 1] ^= Bit(to);
  if (ChecksW()) return;
  HandleCastlingRights();
  m_moves_n++;
}

void AddPromotionStuffB(const int from, const int to) {
  Board *tmp = m_board;
  for (auto piece = 2; piece <= 5; piece++) AddPromotionB(from, to, -piece, piece == 5 ? 100 : 0), m_board = tmp;
}

void AddW(const int from, const int to) {if (m_board->pieces[from] == 1  && Ycoord(from) == 6) AddPromotionStuffW(from, to); else AddNormalStuffW(from, to);}
void AddB(const int from, const int to) {if (m_board->pieces[from] == -1 && Ycoord(from) == 1) AddPromotionStuffB(from, to); else AddNormalStuffB(from, to);}
void AddMovesW(const int from, uint64_t moves) {for (; moves; moves = ClearBit(moves)) AddW(from, Lsb(moves)), m_board = m_board_orig;}
void AddMovesB(const int from, uint64_t moves) {for (; moves; moves = ClearBit(moves)) AddB(from, Lsb(moves)), m_board = m_board_orig;}

void MgenSetupW() {
  m_white   = White();
  m_black   = Black();
  m_both    = m_white | m_black;
  m_empty   = ~m_both;
  m_pawn_sq = m_board->epsq > 0 ? m_black | (Bit(m_board->epsq) & 0x0000FF0000000000ULL) : m_black;
}

void MgenSetupB() {
  m_white   = White();
  m_black   = Black();
  m_both    = m_white | m_black;
  m_empty   = ~m_both;
  m_pawn_sq = m_board->epsq > 0 ? m_white | (Bit(m_board->epsq) & 0x0000000000FF0000ULL) : m_white;
}

void MgenPawnsW() {
  for (uint64_t pieces = m_board->white[0]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesW(pos, m_pawn_checks_w[pos] & m_pawn_sq);
    if (Ycoord(pos) == 1) {
      if (m_pawn_1_moves_w[pos] & m_empty) AddMovesW(pos, m_pawn_2_moves_w[pos] & m_empty);
    } else {
      AddMovesW(pos, m_pawn_1_moves_w[pos] & m_empty);
    }
  }
}

void MgenPawnsB() {
  for (uint64_t pieces = m_board->black[0]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesB(pos, m_pawn_checks_b[pos] & m_pawn_sq);
    if (Ycoord(pos) == 6) {
      if (m_pawn_1_moves_b[pos] & m_empty) AddMovesB(pos, m_pawn_2_moves_b[pos] & m_empty);
    } else {
      AddMovesB(pos, m_pawn_1_moves_b[pos] & m_empty);
    }
  }
}

void MgenPawnsOnlyCapturesW() {
  for (uint64_t pieces = m_board->white[0]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesW(pos, Ycoord(pos) == 6 ? m_pawn_1_moves_w[pos] & (~m_both) : m_pawn_checks_w[pos] & m_pawn_sq);
  }
}

void MgenPawnsOnlyCapturesB() {
  for (uint64_t pieces = m_board->black[0]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesB(pos, Ycoord(pos) == 1 ? m_pawn_1_moves_b[pos] & (~m_both) : m_pawn_checks_b[pos] & m_pawn_sq);
  }
}

void MgenKnightsW() {
  for (uint64_t pieces = m_board->white[1]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesW(pos, m_knight[pos] & m_good);
  }
}

void MgenKnightsB() {
  for (uint64_t pieces = m_board->black[1]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesB(pos, m_knight[pos] & m_good);
  }
}

void MgenBishopsPlusQueensW() {
  for (uint64_t pieces = m_board->white[2] | m_board->white[4]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesW(pos, BishopMagicMoves(pos, m_both) & m_good);
  }
}

void MgenBishopsPlusQueensB() {
  for (uint64_t pieces = m_board->black[2] | m_board->black[4]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesB(pos, BishopMagicMoves(pos, m_both) & m_good);
  }
}

void MgenRooksPlusQueensW() {
  for (uint64_t pieces = m_board->white[3] | m_board->white[4]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesW(pos, RookMagicMoves(pos, m_both) & m_good);
  }
}

void MgenRooksPlusQueensB() {
  for (uint64_t pieces = m_board->black[3] | m_board->black[4]; pieces; pieces = ClearBit(pieces)) {
    const auto pos = Lsb(pieces);
    AddMovesB(pos, RookMagicMoves(pos, m_both) & m_good);
  }
}

void MgenKingW() {
  const auto pos = Lsb(m_board->white[5]);
  AddMovesW(pos, m_king[pos] & m_good);
}

void MgenKingB() {
  const auto pos = Lsb(m_board->black[5]);
  AddMovesB(pos, m_king[pos] & m_good);
}

void MgenAllW() {
  MgenSetupW();
  m_good = ~m_white;
  MgenPawnsW();
  MgenKnightsW();
  MgenBishopsPlusQueensW();
  MgenRooksPlusQueensW();
  MgenKingW();
  MgenCastlingMovesW();
}

void MgenAllB() {
  MgenSetupB();
  m_good = ~m_black;
  MgenPawnsB();
  MgenKnightsB();
  MgenBishopsPlusQueensB();
  MgenRooksPlusQueensB();
  MgenKingB();
  MgenCastlingMovesB();
}

void MgenAllCapturesW() {
  MgenSetupW();
  m_good = m_black;
  MgenPawnsOnlyCapturesW();
  MgenKnightsW();
  MgenBishopsPlusQueensW();
  MgenRooksPlusQueensW();
  MgenKingW();
}

void MgenAllCapturesB() {
  MgenSetupB();
  m_good = m_white;
  MgenPawnsOnlyCapturesB();
  MgenKnightsB();
  MgenBishopsPlusQueensB();
  MgenRooksPlusQueensB();
  MgenKingB();
}

inline void Swap(Board *board_a, Board *board_b) {
  const auto tmp = *board_a;
  *board_a       = *board_b;
  *board_b       = tmp;
}

void SortMgen(const int killermove, const int goodmove) {
  int tactics = 0, i, j;
  if (killermove >= 0 && killermove < m_moves_n) m_moves[killermove].score += 10000;
  if (goodmove   >= 0 && goodmove   < m_moves_n) m_moves[goodmove  ].score += 1000;
  for (i = 0; i < m_moves_n; i++) {
    if (m_moves[i].score) tactics++;
    m_moves[i].index = i;
  }
  tactics = std::min(tactics, m_moves_n);
  for (i = 0; i < tactics; i++)
    for (j = i + 1; j < m_moves_n; j++)
      if (m_moves[j].score > m_moves[i].score)
        Swap(m_moves + j, m_moves + i);
}

void SortAlmostCaptures() {
  int tactics = 0, i, j;
  for (i = 0; i < m_moves_n; i++) if (m_moves[i].score) tactics++;
  for (i = 0; i < tactics; i++)
    for (j = i + 1; j < m_moves_n; j++)
      if (m_moves[j].score > m_moves[i].score)
        Swap(m_moves + j, m_moves + i);
}

void SortCaptures() {
  for (auto i = 0; i < m_moves_n; i++)
    for (auto j = i + 1; j < m_moves_n; j++)
      if (m_moves[j].score > m_moves[i].score)
        Swap(m_moves + j, m_moves + i);
}

int MgenW(Board *moves) {
  m_moves_n    = 0;
  m_moves      = moves;
  m_board_orig = m_board;
  MgenAllW();
  return m_moves_n;
}

int MgenB(Board *moves) {
  m_moves_n    = 0;
  m_moves      = moves;
  m_board_orig = m_board;
  MgenAllB();
  return m_moves_n;
}

int MgenCapturesW(Board *moves) {
  m_moves_n    = 0;
  m_moves      = moves;
  m_board_orig = m_board;
  MgenAllCapturesW();
  return m_moves_n;
}

int MgenCapturesB(Board *moves) {
  m_moves_n    = 0;
  m_moves      = moves;
  m_board_orig = m_board;
  MgenAllCapturesB();
  return m_moves_n;
}

int MgenTacticalW(Board *moves) {
  if (ChecksB()) {const int len = MgenW(moves); SortAlmostCaptures();   return len;}
  else           {const int len = MgenCapturesW(moves); SortCaptures(); return len;}
}

int MgenTacticalB(Board *moves) {
  if (ChecksW()) {const int len = MgenB(moves); SortAlmostCaptures();   return len;}
  else           {const int len = MgenCapturesB(moves); SortCaptures(); return len;}
}

void SortRoot() {
  Board *tmp = m_board;
  for (auto i = 0; i < m_root_n; i++) {
    m_board = m_root + i;
    m_board->score += (Underpromo() ? -10000 : 0) + (m_board->type >= 1 && m_board->type <= 4 ? 5000 : 0) + (m_wtm ? 1 : -1) * Eval(m_wtm) + Random(-10, 10);
  }
  for (auto i = 0; i < m_root_n; i++)
    for (auto j = i + 1; j < m_root_n; j++)
      if (m_root[j].score > m_root[i].score)
        Swap(m_root + j, m_root + i);
  m_board = tmp;
}

void MgenRoot() {
  m_root_n = m_wtm ? MgenW(m_root) : MgenB(m_root);
  SortRoot();
}

// Evaluation

void MixScoreW(const int mg, const int eg) {e_pos_mg += mg; e_pos_eg += eg;}
void MixScoreB(const int mg, const int eg) {e_pos_mg -= mg; e_pos_eg -= eg;}
void ScoreW(const int score, const int mg, const int eg) {MixScoreW(mg * score, eg * score);}
void ScoreB(const int score, const int mg, const int eg) {MixScoreB(mg * score, eg * score);}
void MaterialW(const int piece) {e_mat_mg += kPieceMg[piece]; e_mat_eg += kPieceEg[piece];}
void MaterialB(const int piece) {e_mat_mg -= kPieceMg[piece]; e_mat_eg -= kPieceEg[piece];}
void AttacksW(const int me, uint64_t moves, const int mg, const int eg) {int score = 0; for (moves &= e_black; moves; moves = ClearBit(moves)) score += kAttacks[me][std::max(0, -m_board->pieces[Lsb(moves)] - 1)];
  ScoreW(score, mg, eg);}
void AttacksB(const int me, uint64_t moves, const int mg, const int eg) {int score = 0; for (moves &= e_white; moves; moves = ClearBit(moves)) score += kAttacks[me][std::max(0,  m_board->pieces[Lsb(moves)] - 1)];
  ScoreB(score, mg, eg);}
int EvalClose(const int pos_a, const int pos_b) {return std::pow(7 - std::max(std::abs(Xcoord(pos_a) - Xcoord(pos_b)), std::abs(Ycoord(pos_a) - Ycoord(pos_b))), 2);}
void PstW(const int piece, const int index) {MixScoreW(kPstMg[piece][index], kPstEg[piece][index]);}
void PstB(const int piece, const int index) {MixScoreB(e_pst_mg_b[piece][index], e_pst_eg_b[piece][index]);}
void MobilityW(const uint64_t moves, const int mg, const int eg) {ScoreW(Popcount(moves & (~e_white)), mg, eg);}
void MobilityB(const uint64_t moves, const int mg, const int eg) {ScoreB(Popcount(moves & (~e_black)), mg, eg);}
void CloseToEnemyKingW(const int pos, const int mg, const int eg) {ScoreW(EvalClose(pos, e_black_king_sq), mg, eg);}
void CloseToEnemyKingB(const int pos, const int mg, const int eg) {ScoreB(EvalClose(pos, e_white_king_sq), mg, eg);}
void AttacksToKingW(const uint64_t moves, const int mg, const int eg) {if (moves & m_board->black[5]) MixScoreW(mg, eg);}
void AttacksToKingB(const uint64_t moves, const int mg, const int eg) {if (moves & m_board->white[5]) MixScoreB(mg, eg);}

void EvalPawnsW(const int pos) {
  MaterialW(0);
  PstW(0, pos);
  AttacksW(0, m_pawn_checks_w[pos], 2, 1);
  ScoreW(Popcount(0xFFFFFFFFULL & e_columns_up[pos] & m_board->white[0]), -35, -55);
  if (!(kFreeColumns[Xcoord(pos)] & m_board->white[0]))               MixScoreW(-55, 0);
  if (m_pawn_checks_w[pos] & m_board->white[0])                       MixScoreW(15, 55);
  if (m_pawn_checks_w[pos] & (m_board->white[1] | m_board->white[2])) MixScoreW(55, 25);
  if (!(e_columns_up[pos] & (m_board->black[0] | m_board->white[0]))) ScoreW(Ycoord(pos), 23, 57);
}

void EvalPawnsB(const int pos) {
  MaterialB(0);
  PstB(0, pos);
  AttacksB(0, m_pawn_checks_b[pos], 2, 1);
  ScoreB(Popcount(0xFFFFFFFF00000000ULL & e_columns_down[pos] & m_board->black[0]), -35, -55);
  if (!(kFreeColumns[Xcoord(pos)] & m_board->black[0]))                 MixScoreB(-55, 0);
  if (m_pawn_checks_b[pos] & m_board->black[0])                         MixScoreB(15, 55);
  if (m_pawn_checks_b[pos] & (m_board->black[1] | m_board->black[2]))   MixScoreB(55, 25);
  if (!(e_columns_down[pos] & (m_board->white[0] | m_board->black[0]))) ScoreB(7 - Ycoord(pos), 23, 57);
}

void EvalKnightsW(const int pos) {
  MaterialW(1);
  PstW(1, pos);
  MobilityW(m_knight[pos], 22, 18);
  AttacksW(1, m_knight[pos] | Bit(pos), 2, 1);
  CloseToEnemyKingW(pos, 1, 0);
  AttacksToKingW(m_knight[pos], 15, 1);
}

void EvalKnightsB(const int pos) {
  MaterialB(1);
  PstB(1, pos);
  MobilityB(m_knight[pos], 22, 18);
  AttacksB(1, m_knight[pos] | Bit(pos), 2, 1);
  CloseToEnemyKingB(pos, 1, 0);
  AttacksToKingB(m_knight[pos], 15, 1);
}

void BonusBishopAndPawnsEg(const int me, const int bonus, const uint64_t own_pawns, const uint64_t enemy_pawns) {
  if (Bit(me) & 0x55AA55AA55AA55AAULL) e_pos_eg += bonus * Popcount(0x55AA55AA55AA55AAULL & own_pawns) + 2 * bonus * Popcount(0x55AA55AA55AA55AAULL & enemy_pawns);
  else                                 e_pos_eg += bonus * Popcount(0xAA55AA55AA55AA55ULL & own_pawns) + 2 * bonus * Popcount(0xAA55AA55AA55AA55ULL & enemy_pawns);
}

void EvalBishopsW(const int pos) {
  MaterialW(2);
  PstW(2, pos);
  MobilityW(BishopMagicMoves(pos, e_both), 29, 21);
  AttacksW(2, m_bishop[pos] | Bit(pos), 5, 1);
  BonusBishopAndPawnsEg(pos, 30, m_board->white[0], m_board->black[0]);
}

void EvalBishopsB(const int pos) {
  MaterialB(2);
  PstB(2, pos);
  MobilityB(BishopMagicMoves(pos, e_both), 29, 21);
  AttacksB(2, m_bishop[pos] | Bit(pos), 5, 1);
  BonusBishopAndPawnsEg(pos, -30, m_board->black[0], m_board->white[0]);
}

void EvalRooksW(const int pos) {
  MaterialW(3);
  PstW(3, pos);
  MobilityW(RookMagicMoves(pos, e_both), 26, 14);
  AttacksW(3, m_rook[pos] | Bit(pos), 3, 1);
  CloseToEnemyKingW(pos, 1, 0);
  e_pos_mg += 5 * Popcount(e_columns_up[pos] & e_empty);
  if (e_columns_up[pos] & m_board->white[3]) e_pos_mg += 70;
  if (e_columns_up[pos] & m_board->white[0] & 0xFFFFFFFF00000000ULL)   e_pos_eg += 50;
  if (e_columns_down[pos] & m_board->black[0] & 0x00000000FFFFFFFFULL) e_pos_eg += 30;
}

void EvalRooksB(const int pos) {
  MaterialB(3);
  PstB(3, pos);
  MobilityB(RookMagicMoves(pos, e_both), 26, 14);
  AttacksB(3, m_rook[pos] | Bit(pos), 3, 1);
  CloseToEnemyKingB(pos, 1, 0);
  e_pos_mg -= 5 * Popcount(e_columns_down[pos] & e_empty);
  if (e_columns_down[pos] & m_board->black[3]) e_pos_mg -= 70;
  if (e_columns_down[pos] & m_board->black[0] & 0x00000000FFFFFFFFULL) e_pos_eg -= 50;
  if (e_columns_up[pos] & m_board->white[0] & 0xFFFFFFFF00000000ULL)   e_pos_eg -= 30;
}

void EvalQueensW(const int pos) {
  MaterialW(4);
  PstW(4, pos);
  MobilityW(BishopMagicMoves(pos, e_both) | RookMagicMoves(pos, e_both), 19, 15);
  AttacksW(4, m_queen[pos] | Bit(pos), 1, 2);
}

void EvalQueensB(const int pos) {
  MaterialB(4);
  PstB(4, pos);
  MobilityB(BishopMagicMoves(pos, e_both) | RookMagicMoves(pos, e_both), 19, 15);
  AttacksB(4, m_queen[pos] | Bit(pos), 1, 2);
}

void BonusKingShield(const int pos, const int color, const bool own_shield) {
  if (own_shield) e_pos_mg += 42 * color;
  if (m_board->pieces[pos + color * 8] == 1 * color) e_pos_mg += 100 * color;
  if (m_board->pieces[pos + color * 8] == 3 * color) e_pos_mg += 50 * color;
}

void EvalKingsW(const int pos) {
  PstW(5, pos);
  MobilityW(m_king[pos], 7, 35);
  AttacksW(5, m_king[pos] | Bit(pos), 0, 5);
  ScoreW(Popcount(e_king_ring[pos] & e_black), -200, 5);
  if (!m_board->black[4]) {
    ScoreW(kCenter[pos], 5, 17);
    if (!m_board->black[3]) ScoreW(kCenter[pos], 3, 7);
  }
  if (m_king[pos] & (e_empty & 0x00FFFFFFFFFFFF00ULL)) MixScoreW(112, 25);
  if (Popcount(e_black) < 7) return;
  switch (pos) {
  case 1: BonusKingShield(1, 1, (Bit(8)  | Bit(9)  | Bit(10)) & e_white); break;
  case 2: BonusKingShield(2, 1, (Bit(9)  | Bit(10) | Bit(11)) & e_white); break;
  case 6: BonusKingShield(6, 1, (Bit(13) | Bit(14) | Bit(15)) & e_white); break;}
}

void EvalKingsB(const int pos) {
  PstB(5, pos);
  MobilityB(m_king[pos], 7, 35);
  AttacksB(5, m_king[pos] | Bit(pos), 0, 5);
  ScoreB(Popcount(e_king_ring[pos] & e_white), -200, 5);
  if (!m_board->white[4]) {
    ScoreB(kCenter[pos], 5, 17);
    if (!m_board->white[3]) ScoreB(kCenter[pos], 3, 7);
  }
  if (m_king[pos] & (e_empty & 0x00FFFFFFFFFFFF00ULL)) MixScoreB(112, 25);
  if (Popcount(e_white) < 7) return;
  switch (pos) {
  case 57: BonusKingShield(57, -1, (Bit(48) | Bit(49) | Bit(50)) & e_black); break;
  case 58: BonusKingShield(58, -1, (Bit(49) | Bit(50) | Bit(51)) & e_black); break;
  case 62: BonusKingShield(62, -1, (Bit(53) | Bit(54) | Bit(55)) & e_black); break;}
}

void MatingW() {
  ScoreW(-kCenter[e_black_king_sq],                   11, 11);
  ScoreW(EvalClose(e_white_king_sq, e_black_king_sq), 11, 11);
}

void MatingB() {
  ScoreB(-kCenter[e_white_king_sq],                   11, 11);
  ScoreB(EvalClose(e_black_king_sq, e_white_king_sq), 11, 11);
}

void EvalSetup() {
  e_pos_mg         = 0;
  e_pos_eg         = 0;
  e_mat_mg         = 0;
  e_mat_eg         = 0;
  e_white_material = 0;
  e_black_material = 0;
  e_white_king_sq  = Lsb(m_board->white[5]);
  e_black_king_sq  = Lsb(m_board->black[5]);
  e_white          = White();
  e_black          = Black();
  e_both           = e_white | e_black;
  e_empty          = ~e_both;
  e_both_n         = Popcount(e_both);
}

double EvalCalculateScale(const bool wtm) {
  double scale = Between<double>(0, (e_both_n - 2.0) * (1.0 / ((2.0 * 16.0) - 2.0)), 1.0);
  scale = 0.5 * (1.0 + scale);
  scale = scale * scale;
  if (wtm) {
    if (!m_board->black[4]) scale *= 0.9;
    if (!m_board->black[3]) scale *= 0.95;
  } else {
    if (!m_board->white[4]) scale *= 0.9;
    if (!m_board->white[3]) scale *= 0.95;
  }
  return scale;
}

void EvalBonusPair(const int piece, const int mg, const int eg) {
  if (Popcount(m_board->white[piece]) >= 2) MixScoreW(mg, eg);
  if (Popcount(m_board->black[piece]) >= 2) MixScoreB(mg, eg);
}

void EvalBonusChecks() {
  if (     ChecksB()) MixScoreB(350, 80);
  else if (ChecksW()) MixScoreW(350, 80);
}

void EvalEndgame() {
  if (e_both_n > 6) return;
  if (e_white_material >= e_black_material + 3 || !e_black_material)      MatingW();
  else if (e_black_material >= e_white_material + 3 || !e_white_material) MatingB();
}

void EvalPieces() {
  for (uint64_t both = e_both; both; both = ClearBit(both)) {
    const int pos = Lsb(both);
    switch (m_board->pieces[pos] + 6) {
    case  1 + 6: EvalPawnsW(pos);   e_white_material += 1; break;
    case -1 + 6: EvalPawnsB(pos);   e_black_material += 1; break;
    case  2 + 6: EvalKnightsW(pos); e_white_material += 3; break;
    case -2 + 6: EvalKnightsB(pos); e_black_material += 3; break;
    case  3 + 6: EvalBishopsW(pos); e_white_material += 3; break;
    case -3 + 6: EvalBishopsB(pos); e_black_material += 3; break;
    case  4 + 6: EvalRooksW(pos);   e_white_material += 5; break;
    case -4 + 6: EvalRooksB(pos);   e_black_material += 5; break;
    case  5 + 6: EvalQueensW(pos);  e_white_material += 9; break;
    case -5 + 6: EvalQueensB(pos);  e_black_material += 9; break;
    case  6 + 6: EvalKingsW(pos);                          break;
    case -6 + 6: EvalKingsB(pos);                          break;}
  }
}

int EvalCalculateScore(const bool wtm) {
  const double scale = EvalCalculateScale(wtm);
  return (int) ((scale  * (0.82 * (double) (e_pos_mg + e_mat_mg))) + ((1.0 - scale) * (0.82 * (double) (e_pos_eg + e_mat_eg))));
}

int EvalAll(const bool wtm) {
  EvalSetup();
  EvalPieces();
  EvalEndgame();
  EvalBonusPair(1,  95,  70);
  EvalBonusPair(2, 300, 500);
  EvalBonusPair(3,  50, 200);
  EvalBonusChecks();
  return EvalCalculateScore(wtm);
}

int Eval(const bool wtm) {
  const uint64_t hash = GenHash(wtm);

  // Probe the endgame bases
  if (h_white_wins[(uint32_t) kEgWinsKey] == hash) return +kInf;
  if (h_black_wins[(uint32_t) kEgWinsKey] == hash) return -kInf;

  // Probe the eval hash
  Hash *eval_entry = &h_eval[(uint32_t) (hash & kEvalHashKey)];
  const int level = std::pow(100 - g_level, 2), noise = level ? Random(-level, level) : 0;
  if (eval_entry->hash == hash) return eval_entry->score + noise;

  eval_entry->hash  = hash;
  eval_entry->score = EvalAll(wtm) + (wtm ? kTempoBonus : -kTempoBonus);
  return eval_entry->score + noise;
}

// Search

void Speak(const int score, const uint64_t search_time) {
  std::cout << "info depth " << std::min(s_max_depth, s_depth + 1) <<
               " nodes "     << s_nodes <<
               " time "      << search_time <<
               " nps "       << Nps(s_nodes, search_time) <<
               " score cp "  << ((m_wtm ? 1 : -1) * (int) ((std::abs(score) >= kInf ? 0.01 : 0.1) * score)) <<
               " pv "        << MoveName(m_root) << std::endl;
}

uint64_t DrawKey(const int n_knights_w, const int n_bishops_w, const int n_knights_b, const int n_bishops_b) {
  return z_board[0][n_knights_w] ^ z_board[1][n_bishops_w] ^ z_board[2][n_knights_b] ^ z_board[3][n_bishops_b];
}

bool DrawMaterial() {
  if (m_board->white[0] | m_board->black[0] | m_board->white[3] | m_board->white[4] | m_board->black[3] | m_board->black[4]) return 0;
  const uint64_t hash = DrawKey(Popcount(m_board->white[1]), Popcount(m_board->white[2]), Popcount(m_board->black[1]), Popcount(m_board->black[2]));
  for (auto i = 0; i < 15; i++) if (s_sure_draws[i] == hash) return 1;
  return 0;
}

bool DrawRule50And3repetition(const uint64_t hash) {
  if (m_board->rule50 > 99) return 1;
  for (auto i = m_board->rule50 - 2, reps = 0; i >= 0; i -= 2) if (s_r50_positions[i] == hash && ++reps == 2) return 1;
  return 0;
}

#if defined(__unix__)
bool InputAvailable() {
  fd_set fd;
  struct timeval tv;
  FD_ZERO(&fd);
  FD_SET(STDIN_FILENO, &fd);
  tv.tv_sec  = tv.tv_usec = 0;
  select(STDIN_FILENO + 1, &fd, 0, 0, &tv);
  return FD_ISSET(STDIN_FILENO, &fd) > 0;
}
#else
#include <conio.h>
bool InputAvailable() {return _kbhit();}
#endif

bool UserStop() {
  if (!InputAvailable()) return 0;
  Input();
  if (Token("stop")) return 1;
  return 0;
}

bool TimeCheckSearch() {
  static uint64_t ticks = 0;
  if ((++ticks & 0xFFULL) != 0) return 0;
  if (StopSearch() || ((!(ticks & 0xFFFFFULL)) && UserStop())) s_stop = 1;
  return s_stop;
}

int QSearchW(int alpha, const int beta, const int depth) {
  s_nodes++;
  if (s_stop || TimeCheckSearch() || DrawMaterial()) return 0;
  alpha = std::max(alpha, Eval(1));
  if (depth <= 0 || alpha >= beta) return alpha;
  Board moves[64];
  const int moves_n = MgenTacticalW(moves);
  for (auto i = 0; i < moves_n; i++) {
    m_board = moves + i;
    if (Underpromo()) continue;
    alpha = std::max(alpha, QSearchB(alpha, beta, depth - 1));
    if (alpha >= beta) return alpha;
  }
  return alpha;
}

int QSearchB(const int alpha, int beta, const int depth) {
  s_nodes++;
  if (s_stop || DrawMaterial()) return 0;
  beta = std::min(beta, Eval(0));
  if (depth <= 0 || alpha >= beta) return beta;
  Board moves[64];
  const int moves_n = MgenTacticalB(moves);
  for (auto i = 0; i < moves_n; i++) {
    m_board = moves + i;
    if (Underpromo()) continue;
    beta = std::min(beta, QSearchW(alpha, beta, depth - 1));
    if (alpha >= beta) return beta;
  }
  return beta;
}

void UpdateGoodmove(const uint64_t hash, const uint8_t index) {
  Sort *goodmove  = &h_goodmoves[(uint32_t) (hash & kGoodMovesKey)];
  goodmove->hash  = hash;
  goodmove->index = index;
}

void UpdateKiller(const uint64_t hash, const uint8_t index) {
  Sort *killermove  = &h_killers[(uint32_t) (hash & kKillerMovesKey)];
  killermove->hash  = hash;
  killermove->index = index;
}

void SortMoves(const uint64_t hash) {
  const Sort *killermove = &h_killers[(uint32_t) (hash & kKillerMovesKey)], *goodmove   = &h_goodmoves[(uint32_t) (hash & kGoodMovesKey)];
  SortMgen(killermove->hash == hash ? killermove->index : -1, goodmove->hash == hash ? goodmove->index : -1);
}

int SearchMovesW(int alpha, const int beta, int depth, const int ply) {
  int score, i;
  Board moves[kMaxMoves];
  const uint64_t hash = m_board->hash;
  const bool checks   = ChecksB();
  const int moves_n   = MgenW(moves);
  bool ok_lmr         = moves_n >= 5 && depth >= 2 && !checks;
  if (!moves_n) return checks ? -kInf : 0; else if (moves_n == 1 || (ply < 5 && checks)) depth++;
  SortMoves(hash);
  for (i = 0; i < moves_n; i++) {
    m_board = moves + i;
    if (Underpromo()) continue;
    if (ok_lmr && i >= 2 && (!m_board->score) && !ChecksW()) {
      score = SearchB(alpha, beta, depth - 2 - std::min(1, i / 25), ply + 1);
      if (score <= alpha) continue;
      m_board = moves + i;
    }
    score = SearchB(alpha, beta, depth - 1, ply + 1);
    if (score > alpha) {
      alpha  = score;
      ok_lmr = 0;
      if (alpha >= beta) {
        UpdateKiller(hash, moves[i].index);
        return alpha;
      }
      UpdateGoodmove(hash, moves[i].index);
    }
  }
  return alpha;
}

int SearchW(int alpha, const int beta, const int depth, const int ply) {
  const int rule50    = m_board->rule50;
  const uint64_t hash = GenHash(1), rep_hash = s_r50_positions[rule50];
  m_board->hash = hash;
  s_nodes++;
  if (s_stop || TimeCheckSearch() || DrawRule50And3repetition(hash) || DrawMaterial()) return 0;
  if (depth <= 0 || ply >= kDepthLimit) return (int) (s_r50_factor[rule50] * QSearchW(alpha, beta, s_qs_depth));
  s_r50_positions[rule50] = hash;
  alpha = SearchMovesW(alpha, beta, depth, ply);
  s_r50_positions[rule50] = rep_hash;
  return alpha;
}

int SearchMovesB(const int alpha, int beta, int depth, const int ply) {
  int score, i;
  Board moves[kMaxMoves];
  const uint64_t hash = m_board->hash;
  const bool checks   = ChecksW();
  const int moves_n   = MgenB(moves);
  bool ok_lmr         = moves_n >= 5 && depth >= 2 && !checks;
  if (!moves_n) return checks ? kInf : 0; else if (moves_n == 1 || (ply < 5 && checks)) depth++;
  SortMoves(hash);
  for (i = 0; i < moves_n; i++) {
    m_board = moves + i;
    if (Underpromo()) continue;
    if (ok_lmr && i >= 2 && (!m_board->score) && !ChecksB()) {
      score = SearchW(alpha, beta, depth - 2 - std::min(1, i / 25), ply + 1);
      if (score >= beta) continue;
      m_board = moves + i;
    }
    score = SearchW(alpha, beta, depth - 1, ply + 1);
    if (score < beta) {
      beta   = score;
      ok_lmr = 0;
      if (alpha >= beta) {
        UpdateKiller(hash, moves[i].index);
        return beta;
      }
      UpdateGoodmove(hash, moves[i].index);
    }
  }
  return beta;
}

int SearchB(const int alpha, int beta, const int depth, const int ply) {
  const int rule50    = m_board->rule50;
  const uint64_t hash = GenHash(0), rep_hash = s_r50_positions[rule50];
  m_board->hash = hash;
  s_nodes++;
  if (s_stop || DrawRule50And3repetition(hash) || DrawMaterial()) return 0;
  if (depth <= 0 || ply >= kDepthLimit) return (int) (s_r50_factor[rule50] * QSearchB(alpha, beta, s_qs_depth));
  s_r50_positions[rule50] = hash;
  beta = SearchMovesB(alpha, beta, depth, ply);
  s_r50_positions[rule50] = rep_hash;
  return beta;
}

void SortRoot(const int index) {
  if (!index) return;
  const Board tmp = m_root[index];
  for (auto i = index; i > 0; i--) m_root[i] = m_root[i - 1];
  m_root[0] = tmp;
}

int BestW() {
  int score = 0, best_i = 0, alpha = -kInf, i;
  for (i = 0; i < m_root_n; i++) {
    m_board = m_root + i;
    if (s_depth >= 1 && i >= 1) {
      score = SearchB(alpha, alpha + 1, s_depth, 0);
      if (score > alpha) {
        m_board = m_root + i;
        score   = SearchB(alpha, kInf, s_depth, 0);
      }
    } else {
      score = SearchB(alpha, kInf, s_depth, 0);
    }
    if (s_stop) return s_best_score;
    if (score > alpha) {
      alpha  = score;
      best_i = i;
    }
  }
  SortRoot(best_i);
  return alpha;
}

int BestB() {
  int score = 0, best_i = 0, beta = kInf, i;
  for (i = 0; i < m_root_n; i++) {
    m_board = m_root + i;
    if (s_depth >= 1 && i >= 1) {
      score = SearchW(beta - 1, beta, s_depth, 0);
      if (score < beta) {
        m_board = m_root + i;
        score   = SearchW(-kInf, beta, s_depth, 0);
      }
    } else {
      score = SearchW(-kInf, beta, s_depth, 0);
    }
    if (s_stop) return s_best_score;
    if (score < beta) {
      beta   = score;
      best_i = i;
    }
  }
  SortRoot(best_i);
  return beta;
}

void ThinkSetup(const int think_time) {
  s_stop       = 0;
  s_best_score = 0;
  s_nodes      = 0;
  s_depth      = 0;
  s_qs_depth   = 6;
  s_stop_time  = Now() + (uint64_t) std::max(0, think_time);
}

void RandomMove() {
  if (!m_root_n) return;
  const int root_i = Random(0, m_root_n - 1);
  if (root_i) Swap(m_root, m_root + root_i);
}

bool ThinkRandomMove() {
  if (g_level) return 0;
  RandomMove();
  return 1;
}

const std::string Ullx() {
  char buf[64], *out = &buf[63];
  uint64_t hval = GenHash(m_wtm);
  const unsigned int hbase = 16;
  buf[63] = '\0';
  do {
    *--out = "0123456789abcdef"[hval % hbase];
    hval /= hbase;
  } while (hval);
  return std::string(out);
}

// Make sure the move is legal too
bool SwapMove(const std::string move) {
  for (auto i = 0; i < m_root_n; i++)
    if (MoveName(m_root + i) == move) {
      if (i) Swap(m_root, m_root + i);
      return 1;
    }
  return 0;
}

void ReadGoodMovesFile(const std::string hash) {
  std::ifstream good_moves_file("good-moves.nn");
  std::string nn_hash, nn_move;
  while (!s_stop && good_moves_file >> nn_hash >> nn_move) {
    if (hash == nn_hash) {
      if (SwapMove(nn_move)) s_stop = 1;
      goto out;
    }
  }
out:
  good_moves_file.close();
}

void CheckFilesExists() {
  std::ifstream good_moves_file("good-moves.nn");
  std::ifstream white_wins_file("white-wins.nn");
  std::ifstream black_wins_file("black-wins.nn");
  Assert(good_moves_file.good(), "good-moves.nn missing! Put good-moves.nn in the binary folder.", __LINE__);
  Assert(white_wins_file.good(), "white-wins.nn missing! Put white-wins.nn in the binary folder.", __LINE__);
  Assert(black_wins_file.good(), "black-wins.nn missing! Put black-wins.nn in the binary folder.", __LINE__);
  good_moves_file.close();
  white_wins_file.close();
  black_wins_file.close();
}

void ReadWhitewinsFile() {
  std::ifstream white_wins_file("white-wins.nn");
  std::string hash;
  while (white_wins_file >> hash) {
    std::string::size_type sz = 0;
    const uint64_t ull = std::stoull("0x" + hash + "ULL", &sz, 16);
    h_white_wins[ull & kEgWinsKey] = ull;
  }
  white_wins_file.close();
}

void ReadBlackWinsFile() {
  std::ifstream black_wins_file("black-wins.nn");
  std::string hash;
  while (black_wins_file >> hash) {
    std::string::size_type sz = 0;
    const uint64_t ull = std::stoull("0x" + hash + "ULL", &sz, 16);
    h_black_wins[ull & kEgWinsKey] = ull;
  }
  black_wins_file.close();
}

void Think(const int think_time) {
  Board *tmp     = m_board;
  uint64_t start = Now();
  const std::string shash = Ullx();
  ThinkSetup(think_time);
  MgenRoot();
  if (ThinkRandomMove()) return;
  if (m_root_n <= 1) {Speak(0, 0); return;}
  std::thread nnread(ReadGoodMovesFile, shash);
  for (; std::abs(s_best_score) < 0.5 * kInf && s_depth < s_max_depth && !s_stop; s_depth++) {
    s_best_score = m_wtm ? BestW() : BestB();
    Speak(s_best_score, Now() - start);
    s_qs_depth = std::min(s_qs_depth + 2, 12);
  }
  m_board = tmp;
  nnread.join(); // Finish the nn thread
  Speak(s_best_score, Now() - start);
}

// UCI

void UciFen() {
  if (TokenIs("startpos")) {TokenPop(); return;}
  TokenPop();
  std::string posfen = "";
  for (; TokenOk() && !TokenIs("moves"); TokenPop()) posfen += TokenCurrent() + " ";
  Fen(posfen);
}

void UciMoves() {
  while (TokenOk()) MakeMove(), TokenPop();
}

void UciPosition() {
  Fen(kStartpos);
  UciFen();
  if (Token("moves")) UciMoves();
}

void UciSetoption() {
  if (TokenPeek("name") && TokenPeek("UCI_Chess960", 1) && TokenPeek("value", 2)) {
    g_uci_chess960 = TokenPeek("true", 3);
    TokenPop(4);
  } else if (TokenPeek("name") && TokenPeek("Level", 1) && TokenPeek("value", 2)) {
    TokenPop(3);
    g_level = Between<int>(0, TokenInt(), 100);
  }
}

void UciGo() {
  int wtime = 0, btime = 0, winc = 0, binc = 0, mtg = 30;
  while (TokenOk()) {
    if (     Token("infinite"))  {Think(kInf); goto out;}
    else if (Token("wtime"))     {wtime = TokenInt();}
    else if (Token("btime"))     {btime = TokenInt();}
    else if (Token("winc"))      {winc  = TokenInt();}
    else if (Token("binc"))      {binc  = TokenInt();}
    else if (Token("movestogo")) {mtg   = Between<int>(1, TokenInt(), 1024);}
    else if (Token("movetime"))  {Think(TokenInt()); goto out;}
    else if (Token("depth"))     {s_max_depth = Between<int>(1, TokenInt(), kDepthLimit); Think(kInf); s_max_depth = kDepthLimit; goto out;}
    else {TokenPop();}
  }
  mtg = std::min(mtg + 1, 30);
  Think(std::max(0, (m_wtm ? wtime / mtg + winc : btime / mtg + binc) - kMoveOverhead));
out:
  std::cout << "bestmove " << (m_root_n <= 0 ? "0000" : MoveName(m_root)) << std::endl;
}

void UciUci() {
  std::cout << "id name " << kName << std::endl;
  std::cout << "id author Toni Helminen" << std::endl;
  std::cout << "option name UCI_Chess960 type check default false" << std::endl;
  std::cout << "option name Level type spin default 100 min 0 max 100" << std::endl;
  std::cout << "uciok" << std::endl;
}

bool UciCommands() {
  if (TokenOk()) {
    if (     Token("position"))  UciPosition();
    else if (Token("go"))        UciGo();
    else if (Token("isready"))   std::cout << "readyok" << std::endl;
    else if (Token("setoption")) UciSetoption();
    else if (Token("uci"))       UciUci();
    else if (Token("quit"))      return 0;
  }
  while (TokenOk()) TokenPop();
  return 1;
}

bool Uci() {
  Input();
  return UciCommands();
}

void Make(const int root_i) {
  s_r50_positions[m_board->rule50] = GenHash(m_wtm);
  m_board_tmp = m_root[root_i];
  m_board     = &m_board_tmp;
  m_wtm       = !m_wtm;
}

void MakeMove() {
  const std::string move = TokenCurrent();
  MgenRoot();
  for (auto i = 0; i < m_root_n; i++)
    if (move == MoveName(m_root + i)) {
      Make(i);
      return;
    }
  Assert(0, "Bad move", __LINE__);
}

// Init

uint64_t PermutateBb(const uint64_t moves, const int index) {
  int i, total = 0, good[64] = {0};
  uint64_t permutations = 0;
  for (i = 0; i < 64; i++)
    if (moves & Bit(i)) {
      good[total] = i;
      total++;
    }
  const int popn = Popcount(moves);
  for (i = 0; i < popn; i++) if ((1 << i) & index) permutations |= Bit(good[i]);
  return permutations & moves;
}

uint64_t MakeSliderMagicMoves(const int *slider_vectors, const int pos, const uint64_t moves) {
  uint64_t tmp, possible_moves = 0;
  const int x_pos = Xcoord(pos), y_pos = Ycoord(pos);
  for (auto i = 0; i < 4; i++)
    for (int j = 1; j < 8; j++) {
      const int x = x_pos + j * slider_vectors[2 * i], y = y_pos + j * slider_vectors[2 * i + 1];
      if (!OnBoard(x, y)) break;
      tmp             = Bit(8 * y + x);
      possible_moves |= tmp;
      if (tmp & moves) break;
    }
  return possible_moves & (~Bit(pos));
}

void InitBishopMagics() {
  for (auto i = 0; i < 64; i++) {
    const uint64_t magics = kBishopMoveMagics[i] & (~Bit(i));
    for (int j = 0; j < 512; j++) {
      const uint64_t allmoves = PermutateBb(magics, j);
      m_bishop_magic_moves[i][BishopMagicIndex(i, allmoves)] = MakeSliderMagicMoves(kBishopVectors, i, allmoves);
    }
  }
}

void InitRookMagics() {
  for (auto i = 0; i < 64; i++) {
    const uint64_t magics = kRookMoveMagic[i] & (~Bit(i));
    for (int j = 0; j < 4096; j++) {
      const uint64_t allmoves = PermutateBb(magics, j);
      m_rook_magic_moves[i][RookMagicIndex(i, allmoves)] = MakeSliderMagicMoves(kRookVectors, i, allmoves);
    }
  }
}

uint64_t MakeSliderMoves(const int pos, const int *slider_vectors) {
  uint64_t moves  = 0;
  const int x_pos = Xcoord(pos), y_pos = Ycoord(pos);
  for (auto i = 0; i < 4; i++) {
    const int dx = slider_vectors[2 * i], dy = slider_vectors[2 * i + 1];
    uint64_t tmp = 0;
    for (int j = 1; j < 8; j++) {
      const int x = x_pos + j * dx, y = y_pos + j * dy;
      if (!OnBoard(x, y)) break;
      tmp |= Bit(8 * y + x);
    }
    moves |= tmp;
  }
  return moves;
}

void InitSliderMoves() {
  for (auto i = 0; i < 64; i++) {
    m_rook[i]   = MakeSliderMoves(i, kRookVectors);
    m_bishop[i] = MakeSliderMoves(i, kBishopVectors);
    m_queen[i]  = m_rook[i] | m_bishop[i];
  }
}

uint64_t MakeJumpMoves(const int pos, const int len, const int dy, const int *jump_vectors) {
  uint64_t moves  = 0;
  const int x_pos = Xcoord(pos), y_pos = Ycoord(pos);
  for (auto i = 0; i < len; i++) {
    const auto x = x_pos + jump_vectors[2 * i], y = y_pos + dy * jump_vectors[2 * i + 1];
    if (OnBoard(x, y)) moves |= Bit(8 * y + x);
  }
  return moves;
}

void InitJumpMoves() {
  const int pawn_check_vectors[2 * 2] = {-1,1,1,1}, pawn_1_vectors[1 * 2] = {0,1};
  for (auto i = 0; i < 64; i++) {
    m_king[i]           = MakeJumpMoves(i, 8,  1, kKingVectors);
    m_knight[i]         = MakeJumpMoves(i, 8,  1, kKnightVectors);
    m_pawn_checks_w[i]  = MakeJumpMoves(i, 2,  1, pawn_check_vectors);
    m_pawn_checks_b[i]  = MakeJumpMoves(i, 2, -1, pawn_check_vectors);
    m_pawn_1_moves_w[i] = MakeJumpMoves(i, 1,  1, pawn_1_vectors);
    m_pawn_1_moves_b[i] = MakeJumpMoves(i, 1, -1, pawn_1_vectors);
  }
  for (auto i = 0; i < 8; i++) {
    m_pawn_2_moves_w[ 8 + i] = MakeJumpMoves( 8 + i, 1,  1, pawn_1_vectors) | MakeJumpMoves( 8 + i, 1,  2, pawn_1_vectors);
    m_pawn_2_moves_b[48 + i] = MakeJumpMoves(48 + i, 1, -1, pawn_1_vectors) | MakeJumpMoves(48 + i, 1, -2, pawn_1_vectors);
  }
}

void InitEvalStuff() {
  for (auto i = 0; i < 64; i++) {
    for (auto j = 0; j < 8; j++) {
      auto x = Xcoord(i) + kKingVectors[2 * j];
      auto y = Ycoord(i) + kKingVectors[2 * j + 1];
      if (OnBoard(x, y)) e_king_ring[i] |= Bit(8 * y + x);
    }
    for (auto y = i + 8; y < 64; y += 8) e_columns_up[i]   |= Bit(y);
    for (auto y = i - 8; y > -1; y -= 8) e_columns_down[i] |= Bit(y);
  }
  for (auto i = 0; i < 6; i++)
    for (auto j = 0; j < 64; j++) {
      e_pst_mg_b[i][Mirror(j)] = kPstMg[i][j];
      e_pst_eg_b[i][Mirror(j)] = kPstEg[i][j];
    }
}

void InitDraws() {
  constexpr int draws[6 * 4] = {1,0,0,0,0,1,0,0,2,0,0,0,1,0,0,1,2,0,1,0,2,0,0,1}; // KNK / KBK / KNNK / KNKB / KNNKN / KNNKB
  int len       = 0;
  auto makedraw = [&len](int nkw, int nbw, int nkb, int nbb) {DrawKey(nkw, nbw, nkb, nbb); len++;};
  for (auto i = 0; i < 6; i++) {
    makedraw(draws[4 * i    ], draws[4 * i + 1], draws[4 * i + 2], draws[4 * i + 3]);
    makedraw(draws[4 * i + 2], draws[4 * i + 3], draws[4 * i    ], draws[4 * i + 1]);
  }
  makedraw(0,0,0,0); // KK / KNNKNN / KBKB
  makedraw(2,0,2,0);
  makedraw(0,1,0,1);
}

void InitZobrist() {
  for (auto i = 0; i < 13; i++) for (auto j = 0; j < 64; j++) z_board[i][j] = Random8x64();
  for (auto i = 0; i < 64; i++) z_ep[i]     = Random8x64();
  for (auto i = 0; i < 16; i++) z_castle[i] = Random8x64();
  for (auto i = 0; i <  2; i++) z_wtm[i]    = Random8x64();
}

void Init() {
  g_seed += std::time(nullptr);
  for (auto i = 0; i < 128; i++) s_r50_factor[i] = i < 80 ? 1.0 : 1.0 - ((double) i) / 30.0;
  InitEvalStuff();
  InitBishopMagics();
  InitRookMagics();
  InitZobrist();
  InitDraws();
  InitSliderMoves();
  InitJumpMoves();
  Fen(kStartpos);
  CheckFilesExists();
  ReadWhitewinsFile();
  ReadBlackWinsFile();
}

void PrintHash(const std::string fen) {
  Fen(fen);
  std::cout << Ullx() << std::endl;
}

void PrintLogo() {
  std::cout << "╋╋╋╋╋╋╋╋╋╋┏┓" << std::endl;
  std::cout << "╋╋╋╋╋╋╋╋╋╋┃┃" << std::endl;
  std::cout << "┏┓┏┳━━┳┓╋┏┫┗━┳━━┳┓┏┓" << std::endl;
  std::cout << "┃┗┛┃┏┓┃┃╋┃┃┏┓┃┃━┫┗┛┃" << std::endl;
  std::cout << "┃┃┃┃┏┓┃┗━┛┃┃┃┃┃━┫┃┃┃" << std::endl;
  std::cout << "┗┻┻┻┛┗┻━┓┏┻┛┗┻━━┻┻┻┛" << std::endl;
  std::cout << "╋╋╋╋╋╋┏━┛┃" << std::endl;
  std::cout << "╋╋╋╋╋╋┗━━┛" << std::endl;
}

void Bench() {
  uint64_t nodes = 0, start = Now();
  int nth = 0;
  const std::vector<std::string> suite = {
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w -",                        // +5 Standard positions
    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w -",
    "n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w -",
    "8/PPPk4/8/8/8/8/4Kppp/8 w -",
    "2nnrbkr/p1qppppp/8/1ppb4/6PP/3PP3/PPP2P2/BQNNRBKR w HEhe -", // +5 Chess960 positions
    "b1q1rrkb/pppppppp/3nn3/8/P7/1PPP4/4PPPP/BQNNRKRB w GE -",
    "q1bnrkr1/ppppp2p/2n2p2/4b1p1/2NP4/8/PPP1PPPP/QNB1RRKB w ge -",
    "qnr1bkrb/pppp2pp/3np3/5p2/8/P2P2P1/NPP1PP1P/QN1RBKRB w GDg -",
    "n1bqnrkr/pp1ppp1p/2p5/6p1/2P2b2/PN6/1PNPPPPP/1BBQ1RKR w HFhf -"
  };
  std::cout << ":: Benchmarks ::\n" << std::endl;
  for (auto fen : suite) {
    std::cout << ++nth << " / " << suite.size() << ": " << std::endl;
    Fen(fen);
    Think(1000);
    nodes += s_nodes;
    std::cout << std::endl;
  }
  std::cout << "Nps: " << Nps(nodes, Now() - start) << std::endl;
}

void PrintHelp() {
  std::cout << ":: Help ::" << std::endl;
  std::cout << "> mayhem     # Enters UCI mode" << std::endl;
  std::cout << "--help       This help" << std::endl;
  std::cout << "--version    Print version" << std::endl;
  std::cout << "--bench      Run benchmarks" << std::endl;
  std::cout << "-hash [FEN]  Generate hash" << std::endl;
}

void Loop() {
  g_uci = 1;
  while (Uci());
}

void Args(int argc, char **argv) {
  if (argc >= 3 && std::string(argv[1]) == "-hash")     {PrintHash(std::string(argv[2])); return;}
  if (argc >= 2 && std::string(argv[1]) == "--version") {std::cout << kName << std::endl; return;}
  if (argc >= 2 && std::string(argv[1]) == "--help")    {PrintHelp();                     return;}
  if (argc >= 2 && std::string(argv[1]) == "--bench")   {Bench();                         return;}
  PrintLogo();
  if (argc >= 2) return;
  Loop();
}}

// Execute

// “When the rich wage war it's the poor who die.” ― Jean-Paul Sartre
int main(int argc, char **argv) {
  mayhem::Init();
  mayhem::Args(argc, argv);
  return EXIT_SUCCESS;
}
