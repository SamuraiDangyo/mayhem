/*
Mayhem is Sapeli 1.90 written in C++14 with SF NNUE copy pasted (Credits to all involved)...
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
#include <ctime>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include "nnue.h"

// Namespace

namespace mayhem {

// Constants

const std::string
  kName = "Mayhem NNUE 0.45", kStartpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0";

constexpr uint32_t
  kSortKey = (1 << 22) - 1, kEvalHashKey = (1 << 23) - 1;

constexpr int
  kMaxMoves = 218, kDepthLimit = 30, kInf = 1048576, kKingVectors[] = {1,0,0,1,0,-1,-1,0,1,1,-1,-1,1,-1,-1,1}, kKnightVectors[] = {2,1,-2,1,2,-1,-2,-1,1,2,-1,2,1,-2,-1,-2},
  kBishopVectors[] = {1,1,-1,-1,1,-1,-1,1}, kRookVectors[] = {1,0,0,1,0,-1,-1,0}, kPieceMg[] = {100, 320, 330, 550, 1105}, kPieceEg[] = {125, 375, 397, 597, 1200}, kTempoBonus = 50,
  kMvv[6][6] = {{85,96,97,98,99,100},{84,86,93,94,95,100},{82,83,87,91,92,100},{79,80,81,88,90,100},{75,76,77,78,89,100},{70,71,72,73,74,100}},
  kCenter[] = {-2,-1,1,2,2,1,-1,-2,-1,0,2,3,3,2,0,-1,1,2,4,5,5,4,2,1,2,3,5,6,6,5,3,2,2,3,5,6,6,5,3,2,1,2,4,5,5,4,2,1,-1,0,2,3,3,2,0,-1,-2,-1,1,2,2,1,-1,-2},
  kPsqtMg[6][64] =
    {{0,0,0,0,0,0,0,0,22,35,21,9,9,21,35,22,28,21,-20,25,25,-20,21,28,-6,-27,-15,95,95,-15,-27,-6,-26,-36,-21,-22,-22,-21,-36,-26,3,5,6,8,8,6,5,3,15,17,16,19,19,16,17,15,0,0,0,0,0,0,0,0},
    {-236,-103,-97,-69,-69,-97,-103,-236,-6,0,6,9,9,6,0,-6,-1,7,12,15,15,12,7,-1,6,9,35,18,18,35,9,6,6,10,50,73,73,50,10,6,1,6,12,50,50,12,6,1,-5,0,6,9,9,6,0,-5,-141,-28,-22,6,6,-22,-28,-141},
    {-161,-38,-34,-16,-16,-34,-38,-161,-5,30,6,9,9,6,30,-5,2,16,12,15,15,12,16,2,6,9,16,18,18,16,9,6,6,9,15,19,19,15,9,6,3,6,12,16,16,12,6,3,-4,0,6,9,9,6,0,-4,-141,-11,-7,6,6,-7,-11,-141},
    {-96,-3,48,71,71,48,-3,-96,-5,3,11,44,44,11,3,-5,3,6,12,40,40,12,6,3,6,9,15,18,18,15,9,6,6,9,15,18,18,15,9,6,3,6,12,15,15,12,6,3,52,55,61,64,64,61,55,52,-6,13,23,26,26,23,13,-6},
    {-96,-3,2,7,7,2,-3,-96,-3,3,6,8,8,6,3,-3,3,6,12,15,15,12,6,3,6,9,15,18,18,15,9,6,6,9,16,19,19,16,9,6,3,6,12,15,15,12,6,3,-3,0,6,8,8,6,0,-3,-36,-3,3,6,6,3,-3,-36},
    {-56,100,3,-116,-116,3,100,-56,-3,0,-75,-79,-79,-75,0,-3,3,6,12,15,15,12,6,3,6,9,15,19,19,15,9,6,6,9,15,18,18,15,9,6,3,6,12,15,15,12,6,3,-3,0,6,9,9,6,0,-3,-16,-3,3,6,6,3,-3,-16}},
  kPsqtEg[6][64] =
    {{0,0,0,0,0,0,0,0,-5,0,10,15,15,10,0,-5,5,10,20,25,25,20,10,5,45,50,60,65,65,60,50,45,259,264,274,279,279,274,264,259,725,730,740,745,745,740,730,725,995,1000,1010,1015,1015,1010,1000,995,0,0,0,0,0,0,0,0},
    {-195,-30,-20,5,5,-20,-30,-195,-3,0,10,15,15,10,0,-3,-10,10,20,25,25,20,10,-10,10,18,25,30,30,25,18,10,10,15,28,35,35,28,15,10,-8,10,20,25,25,20,10,-8,-2,0,10,15,15,10,0,-2,-155,-10,-5,10,10,-5,-10,-155},
    {-220,-55,-45,-15,-15,-45,-55,-220,-15,0,10,15,15,10,0,-15,-10,10,20,25,25,20,10,-10,10,15,25,30,30,25,15,10,15,15,25,30,30,25,15,15,-5,10,20,25,25,20,10,-5,-10,0,10,15,15,10,0,-10,-155,-30,-20,10,10,-20,-30,-155},
    {-70,-5,5,10,10,5,-5,-70,-5,0,10,15,15,10,0,-5,5,10,20,25,25,20,10,5,10,15,25,30,30,25,15,10,10,15,25,30,30,25,15,10,5,10,20,25,25,20,10,5,0,5,15,20,20,15,5,0,-30,-5,5,10,10,5,-5,-30},
    {-50,-1,7,10,10,7,-1,-50,-2,7,10,15,15,10,7,-2,5,10,20,25,25,20,10,5,10,15,25,50,50,25,15,10,10,15,25,55,55,25,15,10,5,10,20,25,25,20,10,5,-2,7,10,15,15,10,7,-2,-20,-1,8,10,10,8,-1,-20},
    {-70,-25,5,7,7,5,-25,-70,-5,0,10,15,15,10,0,-5,5,10,20,25,25,20,10,5,10,15,35,55,55,35,15,10,10,15,25,57,57,25,15,10,5,10,20,27,27,20,10,5,-5,0,10,15,15,10,0,-5,-30,-5,5,10,10,5,-5,-30}};

constexpr uint64_t
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

typedef struct {
  uint64_t
    white[6],   // White bitboards
    black[6];   // Black bitboards
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

typedef struct {uint64_t hash; uint8_t index;} Sort;
typedef struct {uint64_t hash; int score;} Hash;

// Variables

int
  g_level = 100, g_move_overhead = 10, m_rook_w[2] = {0}, m_rook_b[2] = {0}, m_root_n = 0, m_king_w = 0, m_king_b = 0, m_moves_n = 0, s_max_depth = kDepthLimit, s_qs_depth = 6, s_depth = 0, s_best_score = 0,
  e_pos_mg = 0, e_pos_eg = 0, e_mat_mg = 0, e_mat_eg = 0, e_white_king_sq = 0, e_black_king_sq = 0, e_both_n = 0, e_pst_mg_b[6][64] = {{0}}, e_pst_eg_b[6][64] = {{0}};

uint64_t
  g_seed = 131783, m_black = 0, m_both = 0, m_empty = 0, m_good = 0, m_pawn_sq = 0, m_white = 0, m_pawn_1_moves_w[64] = {0}, m_pawn_1_moves_b[64] = {0}, m_pawn_2_moves_w[64] = {0}, m_pawn_2_moves_b[64] = {0},
  m_bishop[64] = {0}, m_rook[64] = {0}, m_queen[64] = {0}, m_knight[64] = {0}, m_king[64] = {0}, m_pawn_checks_w[64] = {0}, m_pawn_checks_b[64] = {0}, m_castle_w[2] = {0},
  m_castle_b[2] = {0}, m_castle_empty_w[2] = {0}, m_castle_empty_b[2] = {0}, m_bishop_magic_moves[64][512] = {{0}}, m_rook_magic_moves[64][4096] = {{0}},
  z_ep[64] = {0}, z_castle[16] = {0}, z_wtm[2] = {0}, z_board[13][64] = {{0}}, s_stop_time = 0, s_sure_draws[15] = {0}, s_r50_positions[128] = {0}, s_nodes = 0,
  e_white = 0, e_black = 0, e_empty = 0, e_both = 0, e_columns_down[64] = {0}, e_black_material = 0, e_white_material = 0, e_king_ring[64] = {0}, e_columns_up[64] = {0};

bool
  g_uci_chess960 = 0, m_wtm = 0, s_stop = 0, g_use_classical = 0, s_underpromos = 1;

size_t
  g_tokens_nth = 0;

std::vector<std::string>
  g_tokens = {};

Board
  m_board_tmp = {{0},{0},0,{0},0,0,0,0,0,0,0}, *m_board = &m_board_tmp, *m_moves = 0, *m_board_orig = 0, m_root[kMaxMoves] = {{{0},{0},0,{0},0,0,0,0,0,0,0}};

Hash
  h_eval[kEvalHashKey + 1] = {{0,0}};

Sort
  h_killers[kSortKey + 1] = {{0,0}}, h_goodmoves[kSortKey + 1] = {{0,0}};

std::string
  g_eval_file = "nn-eba324f53044.nnue";

// Function Prototypes

int SearchB(const int, int, const int, const int);
int QSearchB(const int, int, const int);
int Eval(const bool);
int EvaluationHashNNUE(const bool);
void MakeMove();
uint64_t RookMagicMoves(const int, const uint64_t);
uint64_t BishopMagicMoves(const int, const uint64_t);

// Utils

inline uint64_t White() {return m_board->white[0] | m_board->white[1] | m_board->white[2] | m_board->white[3] | m_board->white[4] | m_board->white[5];}
inline uint64_t Black() {return m_board->black[0] | m_board->black[1] | m_board->black[2] | m_board->black[3] | m_board->black[4] | m_board->black[5];}
inline uint64_t Both() {return White() | Black();}
uint8_t Xcoord(const uint64_t bb) {return bb & 7;}
uint8_t Ycoord(const uint64_t bb) {return bb >> 3;}
template <class Type> Type Between(const Type va, const Type vb, const Type vc) {return std::max(va, std::min(vb, vc));}
inline unsigned int Lsb(const uint64_t bb) {return __builtin_ctzll(bb);}
inline unsigned int Popcount(const uint64_t bb) {return __builtin_popcountll(bb);}
inline uint64_t ClearBit(const uint64_t bb) {return bb & (bb - 1);}
uint32_t Nps(const uint64_t nodes, const uint32_t ms) {return (1000 * nodes) / (ms + 1);}
inline uint64_t Bit(const int nbits) {return 0x1ULL << nbits;}
int Mirror(const int sq) {return sq ^ 56;}
void Assert(const bool test, const std::string msg) {if (test) return; std::cerr << msg << std::endl; exit(EXIT_FAILURE);}
const std::string MoveStr(const int from, const int to) {char str[5]; str[0] = 'a' + Xcoord(from); str[1] = '1' + Ycoord(from); str[2] = 'a' + Xcoord(to); str[3] = '1' + Ycoord(to); str[4] = '\0'; return std::string(str);}
uint64_t Now() {struct timeval tv; if (gettimeofday(&tv, NULL)) return 0; return (uint64_t) (1000 * tv.tv_sec + tv.tv_usec / 1000);}
uint64_t RandomBB() {
  static uint64_t va = 0X12311227ULL, vb = 0X1931311ULL, vc = 0X13138141ULL;
  auto mixer = [](uint64_t num) {return ((num) << 7) ^ ((num) >> 5);};
  va ^= vb + vc; vb ^= vb * vc + 0x1717711ULL; vc  = (3 * vc) + 1;
  return mixer(va) ^ mixer(vb) ^ mixer(vc);
}
uint64_t Random8x64() {uint64_t val = 0; for (auto i = 0; i < 8; i++) val ^= RandomBB() << (8 * i); return val;}
int Random1(const int max) {const uint64_t rnum = (g_seed ^ RandomBB()) & 0xFFFFFFFFULL; g_seed = (g_seed << 5) ^ (g_seed + 1) ^ (g_seed >> 3); return (int) (max * (0.0001 * (float) (rnum % 10000)));}
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
    to   = g_uci_chess960 ? m_rook_w[0] : 6;
    break;
  case 2:
    from = m_king_w;
    to   = g_uci_chess960 ? m_rook_w[1] : 2;
    break;
  case 3:
    from = m_king_b;
    to   = g_uci_chess960 ? m_rook_b[0] : 56 + 6;
    break;
  case 4:
    from = m_king_b;
    to   = g_uci_chess960 ? m_rook_b[1] : 56 + 2;
    break;
  case 5: case 6: case 7: case 8:
    return MoveStr(from, to) + PromoLetter(move->pieces[to]);}
  return MoveStr(from, to);
}

inline uint64_t GenHash(const int wtm) {
  uint64_t hash = z_ep[m_board->epsq + 1] ^ z_wtm[wtm] ^ z_castle[m_board->castle], both = Both();
  for (; both; both = ClearBit(both)) {const uint8_t sq = Lsb(both); hash ^= z_board[m_board->pieces[sq] + 6][sq];}
  return hash;
}

// Tokenizer

bool TokenOk(const int look_ahead = 0) {return g_tokens_nth + look_ahead < g_tokens.size();}
const std::string TokenCurrent(const int look_ahead = 1) {return TokenOk(look_ahead) ? g_tokens[g_tokens_nth + look_ahead] : "";}
void TokenPop(const int pop_howmany = 1) {g_tokens_nth += pop_howmany;}
bool Token(const std::string token, const int pop_howmany = 1) {if (TokenOk(0) && token == TokenCurrent(0)) {TokenPop(pop_howmany); return 1;} return 0;}
int TokenInt(const int look_ahead = 0) {return TokenOk(look_ahead) ? std::stoi(g_tokens[g_tokens_nth + look_ahead]) : 0;}
bool TokenPeek(const std::string str, const int look_ahead = 0) {return TokenOk(look_ahead) ? str == g_tokens[g_tokens_nth + look_ahead] : 0;}

// Board

void BuildBitboards() {
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

void FindKings() {for (auto i = 0; i < 64; i++) if (m_board->pieces[i] ==  6) m_king_w = i; else if (m_board->pieces[i] == -6) m_king_b = i;}

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

int Piece(const char piece) {for (auto i = 0; i < 6; i++) {if (piece == "pnbrqk"[i]) return -i - 1; else if (piece == "PNBRQK"[i]) return +i + 1;} return 0;}

void FenBoard(const std::string fen) {
  int sq = 56;
  for (unsigned int i = 0; i < fen.length() && sq >= 0; i++) if (fen[i] == '/') sq -= 16; else if (isdigit(fen[i])) sq += fen[i] - '0'; else m_board->pieces[sq++] = Piece(fen[i]);
}

void FenKQkq(const std::string fen) {
  for (unsigned int i = 0; i < fen.length(); i++)
    if (     fen[i] == 'K') {m_rook_w[0] = 7;      m_board->castle |= 1;}
    else if (fen[i] == 'Q') {m_rook_w[1] = 0;      m_board->castle |= 2;}
    else if (fen[i] == 'k') {m_rook_b[0] = 56 + 7; m_board->castle |= 4;}
    else if (fen[i] == 'q') {m_rook_b[1] = 56 + 0; m_board->castle |= 8;}
    else if (fen[i] >= 'A' && fen[i] <= 'H') {
      const int tmp = fen[i] - 'A';
      if (tmp > m_king_w) {m_rook_w[0] = tmp; m_board->castle |= 1;} else if (tmp < m_king_w) {m_rook_w[1] = tmp; m_board->castle |= 2;}
    } else if (fen[i] >= 'a' && fen[i] <= 'h') {
      const int tmp = fen[i] - 'a';
      if (tmp > Xcoord(m_king_b)) {m_rook_b[0] = 56 + tmp; m_board->castle |= 4;} else if (tmp < Xcoord(m_king_b)) {m_rook_b[1] = 56 + tmp; m_board->castle |= 8;}
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
  Assert(fentokens.size() >= 3, "Error #1: Bad fen");
  FenBoard(fentokens[0]);
  m_wtm = fentokens[1][0] == 'w';
  FindKings();
  FenKQkq(fentokens[2]);
  BuildCastlingBitboards();
  FenEp(fentokens[3]);
  FenRule50(fentokens[4]);
}

void FenReset() {
  constexpr Board empty = {{0},{0},0,{0},0,0,0,0,0,0,0};
  m_board_tmp   = empty;
  m_board       = &m_board_tmp;
  m_wtm         = 1;
  m_board->epsq = -1;
  m_king_w = m_king_b = 0;
  memset(m_rook_w, 0, sizeof(m_rook_w));
  memset(m_rook_b, 0, sizeof(m_rook_b));
  memset(m_board->white, 0, sizeof(m_board->white));
  memset(m_board->black, 0, sizeof(m_board->black));
}

void Fen(const std::string fen) {
  FenReset();
  FenGen(fen);
  BuildBitboards();
  Assert(Popcount(m_board->white[5]) == 1 && Popcount(m_board->black[5]) == 1, "Error #2: Bad kings");
}

// Checks

inline bool ChecksHereW(const int sq) {
  const uint64_t both = Both();
  return ((m_pawn_checks_b[sq] & m_board->white[0]) | (m_knight[sq] & m_board->white[1]) | (BishopMagicMoves(sq, both) & (m_board->white[2] | m_board->white[4])) |
          (RookMagicMoves(sq, both) & (m_board->white[3] | m_board->white[4])) | (m_king[sq] & m_board->white[5]));
}

inline bool ChecksHereB(const int sq) {
  const uint64_t both = Both();
  return ((m_pawn_checks_w[sq] & m_board->black[0]) | (m_knight[sq] & m_board->black[1]) | (BishopMagicMoves(sq, both) & (m_board->black[2] | m_board->black[4])) |
          (RookMagicMoves(sq, both) & (m_board->black[3] | m_board->black[4])) | (m_king[sq] & m_board->black[5]));
}

bool ChecksCastleW(uint64_t squares) {for (; squares; squares = ClearBit(squares)) {if (ChecksHereW(Lsb(squares))) return 1;} return 0;}
bool ChecksCastleB(uint64_t squares) {for (; squares; squares = ClearBit(squares)) {if (ChecksHereB(Lsb(squares))) return 1;} return 0;}
bool ChecksW() {return ChecksHereW(Lsb(m_board->black[5]));}
bool ChecksB() {return ChecksHereB(Lsb(m_board->white[5]));}

// Move generator

inline uint64_t BishopMagicIndex(const int position, const uint64_t mask) {return ((mask & kBishopMask[position]) * kBishopMagic[position]) >> 55;}
inline uint64_t RookMagicIndex(const int position, const uint64_t mask) {return ((mask & kRookMask[position]) * kRookMagic[position]) >> 52;}
inline uint64_t BishopMagicMoves(const int position, const uint64_t mask) {return m_bishop_magic_moves[position][BishopMagicIndex(position, mask)];}
inline uint64_t RookMagicMoves(const int position, const uint64_t mask) {return m_rook_magic_moves[position][RookMagicIndex(position, mask)];}

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
  if ((m_board->castle & 4) && !(m_castle_empty_b[0] & m_both)) {AddCastleOOB();  m_board = m_board_orig;}
  if ((m_board->castle & 8) && !(m_castle_empty_b[1] & m_both)) {AddCastleOOOB(); m_board = m_board_orig;}
}

void CheckCastlingRightsW() {
  if (m_board->pieces[m_king_w]    != 6) {m_board->castle &= 4 | 8; return;}
  if (m_board->pieces[m_rook_w[0]] != 4) {m_board->castle &= 2 | 4 | 8;}
  if (m_board->pieces[m_rook_w[1]] != 4) {m_board->castle &= 1 | 4 | 8;}
}

void CheckCastlingRightsB() {
  if (m_board->pieces[m_king_b]    != -6) {m_board->castle &= 1 | 2; return;}
  if (m_board->pieces[m_rook_b[0]] != -4) {m_board->castle &= 1 | 2 | 8;}
  if (m_board->pieces[m_rook_b[1]] != -4) {m_board->castle &= 1 | 2 | 4;}
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

void AddPromotionW(const int from, const int to, const int piece) {
  const int eat = m_board->pieces[to];
  m_moves[m_moves_n]    = *m_board;
  m_board               = &m_moves[m_moves_n];
  m_board->from         = from;
  m_board->to           = to;
  m_board->score        = 100;
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
  if (!s_underpromos) {AddPromotionW(from, to, 5); return;}
  Board *tmp = m_board;
  for (int piece = 2; piece <= 5; piece++) {AddPromotionW(from, to, piece); m_board = tmp;}
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

void AddPromotionB(const int from, const int to, const int piece) {
  const int eat         = m_board->pieces[to];
  m_moves[m_moves_n]    = *m_board;
  m_board               = &m_moves[m_moves_n];
  m_board->from         = from;
  m_board->to           = to;
  m_board->score        = 100;
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
  if (!s_underpromos) {AddPromotionB(from, to, -5); return;}
  Board *tmp = m_board;
  for (auto piece = 2; piece <= 5; piece++) {AddPromotionB(from, to, -piece); m_board = tmp;}
}

void AddW(const int from, const int to) {if (m_board->pieces[from] == 1  && Ycoord(from) == 6) AddPromotionStuffW(from, to); else AddNormalStuffW(from, to);}
void AddB(const int from, const int to) {if (m_board->pieces[from] == -1 && Ycoord(from) == 1) AddPromotionStuffB(from, to); else AddNormalStuffB(from, to);}
void AddMovesW(const int from, uint64_t moves) {for (; moves; moves = ClearBit(moves)) {AddW(from, Lsb(moves)); m_board = m_board_orig;}}
void AddMovesB(const int from, uint64_t moves) {for (; moves; moves = ClearBit(moves)) {AddB(from, Lsb(moves)); m_board = m_board_orig;}}

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
    const auto sq = Lsb(pieces);
    AddMovesW(sq, m_pawn_checks_w[sq] & m_pawn_sq);
    if (Ycoord(sq) == 1) {
      if (m_pawn_1_moves_w[sq] & m_empty) AddMovesW(sq, m_pawn_2_moves_w[sq] & m_empty);
    } else {
      AddMovesW(sq, m_pawn_1_moves_w[sq] & m_empty);
    }
  }
}

void MgenPawnsB() {
  for (uint64_t pieces = m_board->black[0]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesB(sq, m_pawn_checks_b[sq] & m_pawn_sq);
    if (Ycoord(sq) == 6) {
      if (m_pawn_1_moves_b[sq] & m_empty) AddMovesB(sq, m_pawn_2_moves_b[sq] & m_empty);
    } else {
      AddMovesB(sq, m_pawn_1_moves_b[sq] & m_empty);
    }
  }
}

void MgenPawnsOnlyCapturesW() {
  for (uint64_t pieces = m_board->white[0]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesW(sq, Ycoord(sq) == 6 ? m_pawn_1_moves_w[sq] & (~m_both) : m_pawn_checks_w[sq] & m_pawn_sq);
  }
}

void MgenPawnsOnlyCapturesB() {
  for (uint64_t pieces = m_board->black[0]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesB(sq, Ycoord(sq) == 1 ? m_pawn_1_moves_b[sq] & (~m_both) : m_pawn_checks_b[sq] & m_pawn_sq);
  }
}

void MgenKnightsW() {
  for (uint64_t pieces = m_board->white[1]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesW(sq, m_knight[sq] & m_good);
  }
}

void MgenKnightsB() {
  for (uint64_t pieces = m_board->black[1]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesB(sq, m_knight[sq] & m_good);
  }
}

void MgenBishopsPlusQueensW() {
  for (uint64_t pieces = m_board->white[2] | m_board->white[4]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesW(sq, BishopMagicMoves(sq, m_both) & m_good);
  }
}

void MgenBishopsPlusQueensB() {
  for (uint64_t pieces = m_board->black[2] | m_board->black[4]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesB(sq, BishopMagicMoves(sq, m_both) & m_good);
  }
}

void MgenRooksPlusQueensW() {
  for (uint64_t pieces = m_board->white[3] | m_board->white[4]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesW(sq, RookMagicMoves(sq, m_both) & m_good);
  }
}

void MgenRooksPlusQueensB() {
  for (uint64_t pieces = m_board->black[3] | m_board->black[4]; pieces; pieces = ClearBit(pieces)) {
    const auto sq = Lsb(pieces);
    AddMovesB(sq, RookMagicMoves(sq, m_both) & m_good);
  }
}

void MgenKingW() {
  const auto sq = Lsb(m_board->white[5]);
  AddMovesW(sq, m_king[sq] & m_good);
}

void MgenKingB() {
  const auto sq = Lsb(m_board->black[5]);
  AddMovesB(sq, m_king[sq] & m_good);
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

void SortNthMoves(const int nth) {
  for (auto i = 0; i < nth; i++)
    for (auto j = i + 1; j < m_moves_n; j++)
      if (m_moves[j].score > m_moves[i].score)
        Swap(m_moves + j, m_moves + i);
}

int EvaluateMoves() {int tactics = 0; for (auto i = 0; i < m_moves_n; i++) {if (m_moves[i].score) {tactics++;} m_moves[i].index = i;} return tactics;}
void SortAll() {SortNthMoves(m_moves_n);}

void SortByHash(const uint64_t hash) {
  const Sort *killer = &h_killers[(uint32_t) (hash & kSortKey)], *goodmove = &h_goodmoves[(uint32_t) (hash & kSortKey)];
  if (killer->hash == hash) m_moves[killer->index].score += 10000; else if (goodmove->hash == hash) m_moves[goodmove->index].score += 1000;
  SortNthMoves(EvaluateMoves());
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

int MgenTacticalW(Board *moves) {return ChecksB() ? MgenW(moves) : MgenCapturesW(moves);}
int MgenTacticalB(Board *moves) {return ChecksW() ? MgenB(moves) : MgenCapturesB(moves);}

void EvalRootMoves() {
  Board *tmp = m_board;
  for (auto i = 0; i < m_root_n; i++) {
    m_board = m_root + i;
    m_board->score += (m_wtm ? 1 : -1) * EvaluationHashNNUE(m_wtm) + Random(-5, 5);
  }
  m_board = tmp;
}

void MgenRoot() {
  m_root_n = m_wtm ? MgenW(m_root) : MgenB(m_root);
  EvalRootMoves();
  SortAll();
}

// "Classical" Evaluation

void MixScoreW(const int mg, const int eg) {e_pos_mg += mg; e_pos_eg += eg;}
void MixScoreB(const int mg, const int eg) {e_pos_mg -= mg; e_pos_eg -= eg;}
void ScoreW(const int score, const int mg, const int eg) {MixScoreW(mg * score, eg * score);}
void ScoreB(const int score, const int mg, const int eg) {MixScoreB(mg * score, eg * score);}
void MaterialW(const int piece) {e_mat_mg += kPieceMg[piece]; e_mat_eg += kPieceEg[piece];}
void MaterialB(const int piece) {e_mat_mg -= kPieceMg[piece]; e_mat_eg -= kPieceEg[piece];}
void PsqtW(const int piece, const int index) {MixScoreW(kPsqtMg[piece][index], kPsqtEg[piece][index]);}
void PsqtB(const int piece, const int index) {MixScoreB(e_pst_mg_b[piece][index], e_pst_eg_b[piece][index]);}
void MobilityW(const uint64_t moves, const int mg, const int eg) {ScoreW(Popcount(moves & (~e_white)), mg, eg);}
void MobilityB(const uint64_t moves, const int mg, const int eg) {ScoreB(Popcount(moves & (~e_black)), mg, eg);}
int EvalClose(const int pos_a, const int pos_b) {return std::pow(7 - std::max(std::abs(Xcoord(pos_a) - Xcoord(pos_b)), std::abs(Ycoord(pos_a) - Ycoord(pos_b))), 2);}

void EvalPawnsW(const int sq)   {MaterialW(0); PsqtW(0, sq);}
void EvalPawnsB(const int sq)   {MaterialB(0); PsqtB(0, sq);}
void EvalKnightsW(const int sq) {MaterialW(1); PsqtW(1, sq); MobilityW(m_knight[sq], 12, 18);}
void EvalKnightsB(const int sq) {MaterialB(1); PsqtB(1, sq); MobilityB(m_knight[sq], 12, 18);}
void EvalBishopsW(const int sq) {MaterialW(2); PsqtW(2, sq); MobilityW(BishopMagicMoves(sq, e_both), 19, 21);}
void EvalBishopsB(const int sq) {MaterialB(2); PsqtB(2, sq); MobilityB(BishopMagicMoves(sq, e_both), 19, 21);}
void EvalRooksW(const int sq)   {MaterialW(3); PsqtW(3, sq); MobilityW(RookMagicMoves(sq, e_both), 16, 24);}
void EvalRooksB(const int sq)   {MaterialB(3); PsqtB(3, sq); MobilityB(RookMagicMoves(sq, e_both), 16, 24);}
void EvalQueensW(const int sq)  {MaterialW(4); PsqtW(4, sq); MobilityW(BishopMagicMoves(sq, e_both) | RookMagicMoves(sq, e_both), 11, 25);}
void EvalQueensB(const int sq)  {MaterialB(4); PsqtB(4, sq); MobilityB(BishopMagicMoves(sq, e_both) | RookMagicMoves(sq, e_both), 11, 25);}
void EvalKingsW(const int sq)   {PsqtW(5, sq); MobilityW(m_king[sq], 7, 25);}
void EvalKingsB(const int sq)   {PsqtB(5, sq); MobilityB(m_king[sq], 7, 25);}
void MatingW() {ScoreW(-kCenter[e_black_king_sq], 5, 5); ScoreW(EvalClose(e_white_king_sq, e_black_king_sq), 17, 17);}
void MatingB() {ScoreB(-kCenter[e_white_king_sq], 5, 5); ScoreB(EvalClose(e_black_king_sq, e_white_king_sq), 17, 17);}

void EvalSetup() {
  e_white_material = e_black_material = e_pos_mg = e_pos_eg = e_mat_mg = e_mat_eg = 0;
  e_white_king_sq = Lsb(m_board->white[5]);
  e_black_king_sq = Lsb(m_board->black[5]);
  e_white         = White();
  e_black         = Black();
  e_both          = e_white | e_black;
  e_empty         = ~e_both;
  e_both_n        = Popcount(e_both);
}

float EvalCalculateScale() {
  float scale = Between<float>(0, (e_both_n - 2.0) * (1.0 / ((2.0 * 16.0) - 2.0)), 1.0);
  scale = 0.5 * (1.0 + scale);
  return scale * scale;
}

void EvalEndgame() {
  if (e_both_n > 6) return;
  if (e_white_material >= e_black_material + 3 || !e_black_material) MatingW();
  else if (e_black_material >= e_white_material + 3 || !e_white_material) MatingB();
}

void EvalPieces() {
  for (uint64_t both = e_both; both; both = ClearBit(both)) {
    const int sq = Lsb(both);
    switch (m_board->pieces[sq]) {
    case +1: EvalPawnsW(sq);   e_white_material += 1; break;
    case +2: EvalKnightsW(sq); e_white_material += 3; break;
    case +3: EvalBishopsW(sq); e_white_material += 3; break;
    case +4: EvalRooksW(sq);   e_white_material += 5; break;
    case +5: EvalQueensW(sq);  e_white_material += 9; break;
    case +6: EvalKingsW(sq); break;
    case -1: EvalPawnsB(sq);   e_black_material += 1; break;
    case -2: EvalKnightsB(sq); e_black_material += 3; break;
    case -3: EvalBishopsB(sq); e_black_material += 3; break;
    case -4: EvalRooksB(sq);   e_black_material += 5; break;
    case -5: EvalQueensB(sq);  e_black_material += 9; break;
    case -6: EvalKingsB(sq); break;}
  }
}

int EvalCalculateScore() {
  const float scale = EvalCalculateScale();
  return (int) ((scale * (0.82 * (float) (e_pos_mg / 10 + e_mat_mg))) + ((1.0 - scale) * (0.82 * (float) (e_pos_eg / 10 + e_mat_eg))));
}

int EvalAll() {
  EvalSetup();
  EvalPieces();
  EvalEndgame();
  return EvalCalculateScore();
}

int EvaluationClassical(const bool wtm) {
  return EvalAll() + (wtm ? kTempoBonus : -kTempoBonus);
}

// NNUE Evaluation

int EvaluationNNUE(const bool wtm) {
  int pieces[33], squares[33], index = 2;
  for (auto i = 0; i < 64; i++) {
    switch (m_board->pieces[i]) {
    case +1: case +2: case +3: case +4: case +5: pieces[index] = 7 - (int) m_board->pieces[i];           squares[index++] = i; break;
    case -1: case -2: case -3: case -4: case -5: pieces[index] = 6 + (7 - (- (int) m_board->pieces[i])); squares[index++] = i; break;
    case +6: pieces[0] = 1; squares[0] = i; break;
    case -6: pieces[1] = 7; squares[1] = i; break;}
  }
  pieces[index] = squares[index] = 0;
  return (wtm ? 1 : -1) * nnue_evaluate(!wtm, pieces, squares);
}

int EvaluationHashNNUE(const bool wtm) {
  const uint64_t hash = GenHash(wtm);
  Hash *eval_entry = &h_eval[(uint32_t) (hash & kEvalHashKey)];
  if (eval_entry->hash == hash) return eval_entry->score;
  eval_entry->hash = hash;
  return eval_entry->score = EvaluationNNUE(wtm);
}

// Evaluation

uint64_t DrawKey(const int n_knights_w, const int n_bishops_w, const int n_knights_b, const int n_bishops_b) {
  return z_board[0][n_knights_w] ^ z_board[1][n_bishops_w] ^ z_board[2][n_knights_b] ^ z_board[3][n_bishops_b];
}

bool DrawMaterial() {
  if (m_board->white[0] | m_board->black[0] | m_board->white[3] | m_board->white[4] | m_board->black[3] | m_board->black[4]) return 0;
  const uint64_t hash = DrawKey(Popcount(m_board->white[1]), Popcount(m_board->white[2]), Popcount(m_board->black[1]), Popcount(m_board->black[2]));
  for (auto i = 0; i < 15; i++) if (s_sure_draws[i] == hash) return 1;
  return 0;
}

int Eval(const bool wtm) {
  if (DrawMaterial()) return 0;
  const float scale_factor = m_board->rule50 < 20 ? 1.0 : 1.0 - (((float) m_board->rule50) / 100.0);
  return (int) (scale_factor * (g_use_classical ? EvaluationClassical(wtm) : EvaluationHashNNUE(wtm)));
}

// Search

void Speak(const int score, const uint64_t search_time) {
  std::cout << "info depth " << std::min(s_max_depth, s_depth + 1) << " nodes " << s_nodes << " time " << search_time << " nps " << Nps(s_nodes, search_time)
            << " score cp " << ((m_wtm ? 1 : -1) * (int) ((std::abs(score) >= kInf ? 0.01 : 1.0) * score)) << " pv " << MoveName(m_root) << std::endl;
}

bool Draw() {
  if (m_board->rule50 > 99) return 1;
  const uint64_t hash = s_r50_positions[m_board->rule50];
  for (auto i = m_board->rule50 - 2, reps = 0; i >= 0; i -= 2) {if (s_r50_positions[i] == hash && ++reps == 2) return 1;}
  return 0;
}

#if defined WINDOWS
#include <conio.h>
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
  if (!InputAvailable()) return 0;
  Input();
  if (Token("stop")) return 1;
  return 0;
}

bool StopSearch() {return Now() >= s_stop_time;}

bool TimeCheckSearch() {
  static uint64_t ticks = 0;
  if ((++ticks & 0xFFULL) != 0) return 0;
  if (StopSearch() || ((!(ticks & 0xFFFFFULL)) && UserStop())) s_stop = 1;
  return s_stop;
}

int QSearchW(int alpha, const int beta, const int depth) {
  s_nodes++;
  if (s_stop || TimeCheckSearch()) return 0;
  alpha = std::max(alpha, Eval(1));
  if (depth <= 0 || alpha >= beta) return alpha;
  Board moves[64];
  const auto moves_n = MgenTacticalW(moves);
  SortAll();
  for (auto i = 0; i < moves_n; i++) {
    m_board = moves + i;
    alpha = std::max(alpha, QSearchB(alpha, beta, depth - 1));
    if (alpha >= beta) return alpha;
  }
  return alpha;
}

int QSearchB(const int alpha, int beta, const int depth) {
  s_nodes++;
  if (s_stop) return 0;
  beta = std::min(beta, Eval(0));
  if (depth <= 0 || alpha >= beta) return beta;
  Board moves[64];
  const auto moves_n = MgenTacticalB(moves);
  SortAll();
  for (auto i = 0; i < moves_n; i++) {
    m_board = moves + i;
    beta = std::min(beta, QSearchW(alpha, beta, depth - 1));
    if (alpha >= beta) return beta;
  }
  return beta;
}

void UpdateGoodmove(const uint64_t hash, const uint8_t index) {Sort *goodmove = &h_goodmoves[(uint32_t) (hash & kSortKey)]; goodmove->hash = hash; goodmove->index = index;}
void UpdateKiller(const uint64_t hash, const uint8_t index) {Sort *killer = &h_killers[(uint32_t) (hash & kSortKey)]; killer->hash = hash; killer->index = index;}

int SearchMovesW(int alpha, const int beta, int depth, const int ply) {
  const uint64_t hash = s_r50_positions[m_board->rule50];
  const bool checks = ChecksB();
  Board moves[kMaxMoves];
  const auto moves_n = MgenW(moves);
  if (!moves_n) return checks ? -kInf : 0; else if (moves_n == 1 || (ply < 5 && checks)) depth++;
  bool ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  SortByHash(hash);
  for (auto i = 0; i < moves_n; i++) {
    m_board = moves + i;
    if (ok_lmr && i >= 2 && (!m_board->score) && !ChecksW()) {
      const int score = SearchB(alpha, beta, depth - 2 - std::min(1, i / 23), ply + 1);
      if (score <= alpha) continue;
      m_board = moves + i;
    }
    const int score = SearchB(alpha, beta, depth - 1, ply + 1);
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
  s_nodes++;
  if (s_stop || TimeCheckSearch()) return 0;
  if (depth <= 0 || ply >= kDepthLimit) return QSearchW(alpha, beta, s_qs_depth);
  const auto rule50 = m_board->rule50;
  const uint64_t tmp = s_r50_positions[rule50];
  s_r50_positions[rule50] = GenHash(1);
  alpha = Draw() ? 0 : SearchMovesW(alpha, beta, depth, ply);
  s_r50_positions[rule50] = tmp;
  return alpha;
}

int SearchMovesB(const int alpha, int beta, int depth, const int ply) {
  const uint64_t hash = s_r50_positions[m_board->rule50];
  const bool checks = ChecksW();
  Board moves[kMaxMoves];
  const auto moves_n = MgenB(moves);
  if (!moves_n) return checks ? kInf : 0; else if (moves_n == 1 || (ply < 5 && checks)) depth++;
  bool ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  SortByHash(hash);
  for (auto i = 0; i < moves_n; i++) {
    m_board = moves + i;
    if (ok_lmr && i >= 2 && !m_board->score && !ChecksB()) {
      const int score = SearchW(alpha, beta, depth - 2 - std::min(1, i / 23), ply + 1);
      if (score >= beta) continue;
      m_board = moves + i;
    }
    const int score = SearchW(alpha, beta, depth - 1, ply + 1);
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
  s_nodes++;
  if (s_stop) return 0;
  if (depth <= 0 || ply >= kDepthLimit) return QSearchB(alpha, beta, s_qs_depth);
  const auto rule50 = m_board->rule50;
  const uint64_t tmp = s_r50_positions[rule50];
  s_r50_positions[rule50] = GenHash(0);
  beta = Draw() ? 0 : SearchMovesB(alpha, beta, depth, ply);
  s_r50_positions[rule50] = tmp;
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
        score = SearchB(alpha, kInf, s_depth, 0);
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
        score = SearchW(-kInf, beta, s_depth, 0);
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

void Think(const int think_time) {
  Board *tmp     = m_board;
  uint64_t start = Now();
  ThinkSetup(think_time);
  MgenRoot();
  if (ThinkRandomMove()) return;
  if (m_root_n <= 1) {Speak(0, 0); return;}
  g_use_classical = Popcount(Both()) < 10 && std::abs(EvaluationHashNNUE(m_wtm)) > 500; // Up/down a rook and EG activate simple fast Eval. So we can checkmate/run
  s_underpromos = 0;
  for (; std::abs(s_best_score) < 0.5 * kInf && s_depth < s_max_depth && !s_stop; s_depth++) {
    s_best_score = m_wtm ? BestW() : BestB();
    Speak(s_best_score, Now() - start);
    s_qs_depth = std::min(s_qs_depth + 2, 12);
  }
  s_underpromos = 1;
  g_use_classical = 0;
  m_board = tmp;
  Speak(s_best_score, Now() - start);
}

// UCI

void UciFen() {
  if (Token("startpos")) return;
  TokenPop(1);
  std::string posfen = "";
  for (; TokenOk(0) && !Token("moves", 0); TokenPop(1)) posfen += TokenCurrent(0) + " ";
  Fen(posfen);
}

void UciMoves() {
  while (TokenOk(0)) {MakeMove(); TokenPop(1);}
}

void UciPosition() {
  Fen(kStartpos);
  UciFen();
  if (Token("moves")) UciMoves();
}

void UciSetoption() {
  if (TokenPeek("name") && TokenPeek("UCI_Chess960", 1) && TokenPeek("value", 2)) {g_uci_chess960 = TokenPeek("true", 3); TokenPop(4);}
  else if (TokenPeek("name") && TokenPeek("Level", 1) && TokenPeek("value", 2)) {g_level = Between<int>(0, TokenInt(3), 100); TokenPop(4);}
  else if (TokenPeek("name") && TokenPeek("MoveOverhead", 1) && TokenPeek("value", 2)) {g_move_overhead = Between<int>(0, TokenInt(3), 5000); TokenPop(4);}
  else if (TokenPeek("name") && TokenPeek("EvalFile", 1) && TokenPeek("value", 2)) {g_eval_file = TokenCurrent(3); nnue_init(g_eval_file.c_str()); TokenPop(4);}
}

void UciGo() {
  int wtime = 0, btime = 0, winc = 0, binc = 0, mtg = 30;
  for (; TokenOk(0); TokenPop(1)) {
    if (     Token("infinite"))  {Think(kInf); goto out;}
    else if (Token("wtime"))     {wtime = TokenInt(0);}
    else if (Token("btime"))     {btime = TokenInt(0);}
    else if (Token("winc"))      {winc  = TokenInt(0);}
    else if (Token("binc"))      {binc  = TokenInt(0);}
    else if (Token("movestogo")) {mtg   = Between<int>(1, TokenInt(0), 30);}
    else if (Token("movetime"))  {Think(TokenInt(0)); TokenPop(1); goto out;}
    else if (Token("depth"))     {s_max_depth = Between<int>(1, TokenInt(0), kDepthLimit); Think(kInf); s_max_depth = kDepthLimit; TokenPop(1); goto out;}
  }
  Think(std::max(0, m_wtm ? (wtime - g_move_overhead) / mtg + winc : (btime - g_move_overhead) / mtg + binc));
out:
  std::cout << "bestmove " << (m_root_n <= 0 ? "0000" : MoveName(m_root)) << std::endl;
}

void UciUci() {
  std::cout << "id name " << kName << std::endl;
  std::cout << "id author Toni Helminen" << std::endl;
  std::cout << "option name UCI_Chess960 type check default false" << std::endl;
  std::cout << "option name Level type spin default 100 min 0 max 100" << std::endl;
  std::cout << "option name MoveOverhead type spin default 10 min 0 max 5000" << std::endl;
  std::cout << "option name EvalFile type string default " << g_eval_file << std::endl;
  std::cout << "uciok" << std::endl;
}

bool UciCommands() {
  if (TokenOk(0)) {
    if (Token("position"))       UciPosition();
    else if (Token("go"))        UciGo();
    else if (Token("isready"))   std::cout << "readyok" << std::endl;
    else if (Token("setoption")) UciSetoption();
    else if (Token("uci"))       UciUci();
    else if (Token("quit"))      return 0;
  }
  while (TokenOk(0)) TokenPop();
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
  const std::string move = TokenCurrent(0);
  MgenRoot();
  for (auto i = 0; i < m_root_n; i++) {if (move == MoveName(m_root + i)) {Make(i); return;}}
  Assert(0, "Error #4: Bad move");
}

// Init

uint64_t PermutateBb(const uint64_t moves, const int index) {
  int total = 0, good[64] = {0};
  uint64_t permutations = 0;
  for (auto i = 0; i < 64; i++) if (moves & Bit(i)) good[total++] = i;
  const int popn = Popcount(moves);
  for (auto i = 0; i < popn; i++) if ((1 << i) & index) permutations |= Bit(good[i]);
  return permutations & moves;
}

uint64_t MakeSliderMagicMoves(const int *slider_vectors, const int sq, const uint64_t moves) {
  uint64_t possible_moves = 0;
  const int x_pos = Xcoord(sq), y_pos = Ycoord(sq);
  for (auto i = 0; i < 4; i++)
    for (auto j = 1; j < 8; j++) {
      const int x = x_pos + j * slider_vectors[2 * i], y = y_pos + j * slider_vectors[2 * i + 1];
      if (!OnBoard(x, y)) break;
      const uint64_t tmp = Bit(8 * y + x);
      possible_moves |= tmp;
      if (tmp & moves) break;
    }
  return possible_moves & (~Bit(sq));
}

void InitBishopMagics() {
  for (auto i = 0; i < 64; i++) {
    const uint64_t magics = kBishopMoveMagics[i] & (~Bit(i));
    for (auto j = 0; j < 512; j++) {
      const uint64_t allmoves = PermutateBb(magics, j);
      m_bishop_magic_moves[i][BishopMagicIndex(i, allmoves)] = MakeSliderMagicMoves(kBishopVectors, i, allmoves);
    }
  }
}

void InitRookMagics() {
  for (auto i = 0; i < 64; i++) {
    const uint64_t magics = kRookMoveMagic[i] & (~Bit(i));
    for (auto j = 0; j < 4096; j++) {
      const uint64_t allmoves = PermutateBb(magics, j);
      m_rook_magic_moves[i][RookMagicIndex(i, allmoves)] = MakeSliderMagicMoves(kRookVectors, i, allmoves);
    }
  }
}

uint64_t MakeSliderMoves(const int sq, const int *slider_vectors) {
  uint64_t moves  = 0;
  const int x_pos = Xcoord(sq), y_pos = Ycoord(sq);
  for (auto i = 0; i < 4; i++) {
    const int dx = slider_vectors[2 * i], dy = slider_vectors[2 * i + 1];
    uint64_t tmp = 0;
    for (auto j = 1; j < 8; j++) {
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

uint64_t MakeJumpMoves(const int sq, const int len, const int dy, const int *jump_vectors) {
  uint64_t moves  = 0;
  const int x_pos = Xcoord(sq), y_pos = Ycoord(sq);
  for (auto i = 0; i < len; i++) {
    const int x = x_pos + jump_vectors[2 * i], y = y_pos + dy * jump_vectors[2 * i + 1];
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

void InitDraws() {
  constexpr int draws[6 * 4] = {1,0,0,0 ,0,1,0,0, 2,0,0,0, 1,0,0,1, 2,0,1,0, 2,0,0,1}; // KNK / KBK / KNNK / KNKB / KNNKN / KNNKB
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

void InitPsqt() {
  for (auto i = 0; i < 6; i++)
    for (auto j = 0; j < 64; j++) {
      e_pst_mg_b[i][Mirror(j)] = kPsqtMg[i][j];
      e_pst_eg_b[i][Mirror(j)] = kPsqtEg[i][j];
    }
}

void Init() {
  g_seed += std::time(nullptr);
  InitBishopMagics();
  InitRookMagics();
  InitZobrist();
  InitDraws();
  InitSliderMoves();
  InitJumpMoves();
  InitPsqt();
  nnue_init(g_eval_file.c_str());
  Fen(kStartpos);
}

void Bench() {
  uint64_t nodes = 0, start = Now();
  int nth = 0;
  const std::vector<std::string> suite = {
    "qnnbrkbr/pppppppp/8/8/8/8/PPPPPPPP/QNNBRKBR w EHeh - 0 1",
    "qnnrkbbr/pppppppp/8/8/8/8/PPPPPPPP/QNNRKBBR w DHdh - 0 1",
    "qnnrkrbb/pppppppp/8/8/8/8/PPPPPPPP/QNNRKRBB w DFdf - 0 1",
    "bbnqnrkr/pppppppp/8/8/8/8/PPPPPPPP/BBNQNRKR w FHfh - 0 1",
    "bnqbnrkr/pppppppp/8/8/8/8/PPPPPPPP/BNQBNRKR w FHfh - 0 1",
    "bnqnrbkr/pppppppp/8/8/8/8/PPPPPPPP/BNQNRBKR w EHeh - 0 1",
    "bnqnrkrb/pppppppp/8/8/8/8/PPPPPPPP/BNQNRKRB w EGeg - 0 1",
    "nbbqnrkr/pppppppp/8/8/8/8/PPPPPPPP/NBBQNRKR w FHfh - 0 1",
    "nqbbnrkr/pppppppp/8/8/8/8/PPPPPPPP/NQBBNRKR w FHfh - 0 1",
    "nqbnrbkr/pppppppp/8/8/8/8/PPPPPPPP/NQBNRBKR w EHeh - 0 1"
  };
  std::cout << ":: Benchmarks ::\n" << std::endl;
  for (auto fen : suite) {
    std::cout << ++nth << " / " << suite.size() << ":" << std::endl;
    Fen(fen);
    Think(2000);
    nodes += s_nodes;
    std::cout << std::endl;
  }
  std::cout << "Nps: " << Nps(nodes, Now() - start) << std::endl;
}

void PrintHelp() {
  std::cout << ":: Help ::" << std::endl;
  std::cout << "> mayhem  # Enter UCI mode" << std::endl;
  std::cout << "--help    This help" << std::endl;
  std::cout << "--version Print version" << std::endl;
  std::cout << "--bench   Run benchmarks" << std::endl;
  std::cout << "--list    Show root list" << std::endl;
}

void Loop() {while (Uci());}

void PrintRoot() {
  for (auto i = 0; i < m_root_n; i++)
    std::cout << i << ": " << MoveName(m_root + i) << " : " << m_root[i].score << std::endl;
}

void Args(int argc, char **argv) {
  if (argc == 1) {Loop(); return;}
  if (argc == 2 && std::string(argv[1]) == "--version") {std::cout << kName << std::endl; return;}
  if (argc == 2 && std::string(argv[1]) == "--help")    {PrintHelp(); return;}
  if (argc == 2 && std::string(argv[1]) == "--bench")   {Bench(); return;}
  if (argc == 2 && std::string(argv[1]) == "--list")    {MgenRoot(); PrintRoot(); return;}
  std::cout << "> mayhem --help" << std::endl;
}}

// "The Spartans do not ask how many are the enemy but where are they." -- Plutarch
int main(int argc, char **argv) {
  mayhem::Init();
  mayhem::Args(argc, argv);
  //mayhem::Think(10000);
  return EXIT_SUCCESS;
}
