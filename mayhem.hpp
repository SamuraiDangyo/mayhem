/*
Mayhem. Linux UCI Chess960 engine. Written in C++17 language
Copyright (C) 2020-2022 Toni Helminen

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

// Just for InputAvailable() ...
extern "C" {
#ifdef WINDOWS
#include <conio.h>
#else
#include <unistd.h>
#endif
} // extern "C"

// Just extra strength
#include "nnue.hpp"
#include "polyglotbook.hpp"
#include "eucalyptus.hpp"

// Namespace

namespace mayhem {

// Macros

#define VERSION       "Mayhem 7.2" // Version
#define STARTPOS      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" // uci startpos
#define MAX_MOVES     256      // Max chess moves
#define MAX_DEPTH     64       // Max search depth (stack frame problems ...)
#define MAX_Q_DEPTH   12       // Max Qsearch depth
#define BOOK_MS       100      // At least 100ms+ for the book lookup
#define INF           1048576  // System max number
#define MAX_ARR       102      // Enough space for arrays
#define HASH          256      // MB
#define NOISE         2        // Noise for opening moves
#define MOVEOVERHEAD  100      // ms
#define BOOK_BEST     false    // Nondeterministic opening play
#define MAX_PIECES    32       // Max pieces on board (32 normally ...)
#define READ_CLOCK    0x1FFULL // Read clock every 512 ticks (white / 2 x both)
#define EVAL_FILE     "nn-cb80fb9393af.nnue" // "x" to disable NNUE
#define BOOK_FILE     "performance.bin"      // "x" to disable book

// Constants

// Tactical fens to pressure search
const std::array<const std::string, 15> kBench =
{ "R7/P4k2/8/8/8/8/r7/6K1 w - - 0 1 ; bm Rh8",
  "2kr3r/pp1q1ppp/5n2/1Nb5/2Pp1B2/7Q/P4PPP/1R3RK1 w - - 0 1 ; bm Nxa7+",
  "2R5/2R4p/5p1k/6n1/8/1P2QPPq/r7/6K1 w - - 0 1 ; bm Rxh7+",
  "5r1k/1b4p1/p6p/4Pp1q/2pNnP2/7N/PPQ3PP/5R1K b - - 0 1 ; bm Qxh3",
  "6k1/3r4/2R5/P5P1/1P4p1/8/4rB2/6K1 b - - 0 1 ; bm g3",
  "5n2/pRrk2p1/P4p1p/4p3/3N4/5P2/6PP/6K1 w - - 0 1 ; bm Nb5",
  "8/6pp/4p3/1p1n4/1NbkN1P1/P4P1P/1PR3K1/r7 w - - 0 1 ; bm Rxc4+",
  "2r5/2rk2pp/1pn1pb2/pN1p4/P2P4/1N2B3/nPR1KPPP/3R4 b - - 0 1 ; bm Nxd4+",
  "nrq4r/2k1p3/1p1pPnp1/pRpP1p2/P1P2P2/2P1BB2/1R2Q1P1/6K1 w - - 0 1 ; bm Bxc5",
  "3r2k1/5p2/6p1/4b3/1P2P3/1R2P2p/P1K1N3/8 b - - 0 1 ; bm Rd1",
  "1k1r4/pp1r1pp1/4n1p1/2R5/2Pp1qP1/3P2QP/P4PB1/1R4K1 w - - 0 1 ; bm Bxb7",
  "2r1k3/6pr/p1nBP3/1p3p1p/2q5/2P5/P1R4P/K2Q2R1 w - - 0 1 ; bm Rxg7",
  "2b4k/p1b2p2/2p2q2/3p1PNp/3P2R1/3B4/P1Q2PKP/4r3 w - - 0 1 ; bm Qxc6",
  "5bk1/1rQ4p/5pp1/2pP4/3n1PP1/7P/1q3BB1/4R1K1 w - - 0 1 ; bm d6",
  "rnbqkb1r/pppp1ppp/8/4P3/6n1/7P/PPPNPPP1/R1BQKBNR b KQkq - 0 1 ; bm Ne3" };

// [Attacker][Captured] / [PNBRQK][pnbrqk]
constexpr int kMvv[6][6] = { {10, 15, 15, 20, 25, 99}, {9, 14, 14, 19, 24, 99}, {9, 14, 14, 19, 24, 99},
                             { 8, 13, 13, 18, 23, 99}, {7, 12, 12, 17, 22, 99}, {6, 11, 11, 16, 21, 99} };

// Piece-Square Tables
// Material baked in
// MG / EG -> P / N / B / R / Q / K
constexpr int kPesto[6][2][64] =
{ {{82,   82,   82,   82,   82,   82,   82,   82,   47,   81,   62,   59,   67,   106,  120,  60,
    56,   78,   78,   72,   85,   85,   115,  70,   55,   80,   77,   94,   99,   88,   92,   57,
    68,   95,   88,   103,  105,  94,   99,   59,   76,   89,   108,  113,  147,  138,  107,  62,
    180,  216,  143,  177,  150,  208,  116,  71,   82,   82,   82,   82,   82,   82,   82,   82},
   {94,   94,   94,   94,   94,   94,   94,   94,   107,  102,  102,  104,  107,  94,   96,   87,
    98,   101,  88,   95,   94,   89,   93,   86,   107,  103,  91,   87,   87,   86,   97,   93,
    126,  118,  107,  99,   92,   98,   111,  111,  188,  194,  179,  161,  150,  147,  176,  178,
    272,  267,  252,  228,  241,  226,  259,  281,  94,   94,   94,   94,   94,   94,   94,   94}},
  {{232,  316,  279,  304,  320,  309,  318,  314,  308,  284,  325,  334,  336,  355,  323,  318,
    314,  328,  349,  347,  356,  354,  362,  321,  324,  341,  353,  350,  365,  356,  358,  329,
    328,  354,  356,  390,  374,  406,  355,  359,  290,  397,  374,  402,  421,  466,  410,  381,
    264,  296,  409,  373,  360,  399,  344,  320,  170,  248,  303,  288,  398,  240,  322,  230},
   {252,  230,  258,  266,  259,  263,  231,  217,  239,  261,  271,  276,  279,  261,  258,  237,
    258,  278,  280,  296,  291,  278,  261,  259,  263,  275,  297,  306,  297,  298,  285,  263,
    264,  284,  303,  303,  303,  292,  289,  263,  257,  261,  291,  290,  280,  272,  262,  240,
    256,  273,  256,  279,  272,  256,  257,  229,  223,  243,  268,  253,  250,  254,  218,  182}},
  {{332,  362,  351,  344,  352,  353,  326,  344,  369,  380,  381,  365,  372,  386,  398,  366,
    365,  380,  380,  380,  379,  392,  383,  375,  359,  378,  378,  391,  399,  377,  375,  369,
    361,  370,  384,  415,  402,  402,  372,  363,  349,  402,  408,  405,  400,  415,  402,  363,
    339,  381,  347,  352,  395,  424,  383,  318,  336,  369,  283,  328,  340,  323,  372,  357},
   {274,  288,  274,  292,  288,  281,  292,  280,  283,  279,  290,  296,  301,  288,  282,  270,
    285,  294,  305,  307,  310,  300,  290,  282,  291,  300,  310,  316,  304,  307,  294,  288,
    294,  306,  309,  306,  311,  307,  300,  299,  299,  289,  297,  296,  295,  303,  297,  301,
    289,  293,  304,  285,  294,  284,  293,  283,  283,  276,  286,  289,  290,  288,  280,  273}},
  {{458,  464,  478,  494,  493,  484,  440,  451,  433,  461,  457,  468,  476,  488,  471,  406,
    432,  452,  461,  460,  480,  477,  472,  444,  441,  451,  465,  476,  486,  470,  483,  454,
    453,  466,  484,  503,  501,  512,  469,  457,  472,  496,  503,  513,  494,  522,  538,  493,
    504,  509,  535,  539,  557,  544,  503,  521,  509,  519,  509,  528,  540,  486,  508,  520},
   {503,  514,  515,  511,  507,  499,  516,  492,  506,  506,  512,  514,  503,  503,  501,  509,
    508,  512,  507,  511,  505,  500,  504,  496,  515,  517,  520,  516,  507,  506,  504,  501,
    516,  515,  525,  513,  514,  513,  511,  514,  519,  519,  519,  517,  516,  509,  507,  509,
    523,  525,  525,  523,  509,  515,  520,  515,  525,  522,  530,  527,  524,  524,  520,  517}},
  {{1024, 1007, 1016, 1035, 1010, 1000, 994,  975,  990,  1017, 1036, 1027, 1033, 1040, 1022, 1026,
    1011, 1027, 1014, 1023, 1020, 1027, 1039, 1030, 1016, 999,  1016, 1015, 1023, 1021, 1028, 1022,
    998,  998,  1009, 1009, 1024, 1042, 1023, 1026, 1012, 1008, 1032, 1033, 1054, 1081, 1072, 1082,
    1001, 986,  1020, 1026, 1009, 1082, 1053, 1079, 997,  1025, 1054, 1037, 1084, 1069, 1068, 1070},
   {903,  908,  914,  893,  931,  904,  916,  895,  914,  913,  906,  920,  920,  913,  900,  904,
    920,  909,  951,  942,  945,  953,  946,  941,  918,  964,  955,  983,  967,  970,  975,  959,
    939,  958,  960,  981,  993,  976,  993,  972,  916,  942,  945,  985,  983,  971,  955,  945,
    919,  956,  968,  977,  994,  961,  966,  936,  927,  958,  958,  963,  963,  955,  946,  956}},
  {{-15,  36,   12,   -54,  8,    -28,  24,   14,   1,    7,    -8,   -64,  -43,  -16,  9,    8,
    -14,  -14,  -22,  -46,  -44,  -30,  -15,  -27,  -49,  -1,   -27,  -39,  -46,  -44,  -33,  -51,
    -17,  -20,  -12,  -27,  -30,  -25,  -14,  -36,  -9,   24,   2,    -16,  -20,  6,    22,   -22,
    29,   -1,   -20,  -7,   -8,   -4,   -38,  -29,  -65,  23,   16,   -15,  -56,  -34,  2,    13},
   {-53,  -34,  -21,  -11,  -28,  -14,  -24,  -43,  -27,  -11,  4,    13,   14,   4,    -5,   -17,
    -19,  -3,   11,   21,   23,   16,   7,    -9,   -18,  -4,   21,   24,   27,   23,   9,    -11,
    -8,   22,   24,   27,   26,   33,   26,   3,    10,   17,   23,   15,   20,   45,   44,   13,
    -12,  17,   14,   17,   17,   38,   23,   11,   -74,  -35,  -18,  -18,  -11,  15,   4,    -17}} };

constexpr std::uint64_t kRookMagics[3][64] =
{ { 0x548001400080106cULL, 0x900184000110820ULL,  0x428004200a81080ULL,  0x140088082000c40ULL, // Magics
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
    0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL } };

constexpr std::uint64_t kBishopMagics[3][64] =
{ { 0x2890208600480830ULL, 0x324148050f087ULL,    0x1402488a86402004ULL, 0xc2210a1100044bULL, // Magics
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
    0x28440200000000ULL,   0x50080402000000ULL,   0x20100804020000ULL,   0x40201008040200ULL } };

// Enums

enum class MoveType { kKiller, kGood };

// Structs

struct Board { // 171B
  std::uint64_t white[6]{};   // White bitboards
  std::uint64_t black[6]{};   // Black bitboards
  std::int32_t  score{0};     // Sorting score
  std::int8_t   pieces[64]{}; // Pieces white and black
  std::int8_t   epsq{-1};     // En passant square
  std::uint8_t  index{0};     // Sorting index
  std::uint8_t  from{0};      // From square
  std::uint8_t  to{0};        // To square
  std::uint8_t  type{0};      // Move type (0: Normal, 1: OOw, 2: OOOw, 3: OOb, 4: OOOb, 5: =n, 6: =b, 7: =r, 8: =q)
  std::uint8_t  castle{0};    // Castling rights (0x1: K, 0x2: Q, 0x4: k, 0x8: q)
  std::uint8_t  fifty{0};     // Rule 50 counter

  Board() = default; // Default constructor (Just this)
  const std::string movename() const;
  bool is_underpromo() const;
  const std::string to_fen() const;
  const std::string to_s() const;
};

struct HashEntry { // 10B
  std::uint32_t killer_hash{0}, good_hash{0}; // Hash
  std::uint8_t killer{0}, good{0};            // Index

  HashEntry() = default;
  template <MoveType>
  void update(const std::uint64_t, const std::uint8_t);
  void put_hash_value_to_moves(const std::uint64_t, Board*) const;
};

// Variables

std::uint64_t g_black = 0, g_white = 0, g_both = 0, g_empty = 0, g_good = 0, g_stop_search_time = 0,
  g_nodes = 0, g_pawn_sq = 0, g_pawn_1_moves_w[64]{}, g_pawn_1_moves_b[64]{}, g_pawn_2_moves_w[64]{},
  g_pawn_2_moves_b[64]{}, g_knight_moves[64]{}, g_king_moves[64]{}, g_pawn_checks_w[64]{}, g_pawn_checks_b[64]{},
  g_castle_w[2]{}, g_castle_b[2]{}, g_castle_empty_w[2]{}, g_castle_empty_b[2]{}, g_bishop_magic_moves[64][512]{},
  g_rook_magic_moves[64][4096]{}, g_zobrist_ep[64]{}, g_zobrist_castle[16]{}, g_zobrist_wtm[2]{},
  g_r50_positions[MAX_ARR]{}, g_zobrist_board[13][64]{};

int g_move_overhead = MOVEOVERHEAD, g_level = 100, g_root_n = 0, g_king_w = 0, g_king_b = 0,
  g_moves_n = 0, g_max_depth = MAX_DEPTH, g_q_depth = 0, g_depth = 0, g_best_score = 0, g_noise = NOISE,
  g_last_eval = 0, g_rook_w[2]{}, g_rook_b[2]{}, g_lmr[MAX_DEPTH][MAX_MOVES]{}, g_fullmoves = 1,
  g_nnue_pieces[64]{}, g_nnue_squares[64]{};

bool g_chess960 = false, g_wtm = false, g_underpromos = true, g_nullmove_active = false,
  g_stop_search = false, g_is_pv = false, g_book_exist = false, g_nnue_exist = false,
  g_classical = false, g_game_on = true, g_analyzing = false;

Board g_board_empty{}, *g_board = &g_board_empty, *g_moves = nullptr, *g_board_orig = nullptr,
  g_boards[MAX_DEPTH + MAX_Q_DEPTH + 4][MAX_MOVES]{};

std::uint32_t g_hash_entries = 0, g_tokens_nth = 0;
std::vector<std::string> g_tokens(300); // 300 plys init
polyglotbook::PolyglotBook g_book{};
std::unique_ptr<HashEntry[]> g_hash{};
float g_scale[MAX_ARR]{};

// Prototypes

int SearchW(int, const int, const int, const int);
int SearchB(const int, int, const int, const int);
int QSearchB(const int, int, const int, const int);
int Evaluate(const bool);
bool ChecksW();
bool ChecksB();
std::uint64_t RookMagicMoves(const int, const std::uint64_t);
std::uint64_t BishopMagicMoves(const int, const std::uint64_t);

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

// Count first 0's
inline int Ctz(const std::uint64_t bb) {
  return __builtin_ctzll(bb);
}

// Count (return) zeros AND then pop (arg) BitBoard
inline int CtzPop(std::uint64_t *bb) {
  const auto ret = Ctz(*bb);
  *bb = *bb & (*bb - 0x1ULL);
  return ret;
}

// Count 1's in 64b
inline int PopCount(const std::uint64_t bb) {
  return __builtin_popcountll(bb);
}

// sq % 8 - X axle of board
inline int Xaxl(const int sq) {
  return sq & 0x7;
}

// sq / 8 - Y axle of board
inline int Yaxl(const int sq) {
  return sq >> 3;
}

// Set bit in 1 -> 64
inline std::uint64_t Bit(const int nth) {
  return 0x1ULL << nth;
}

// Nodes Per Second
std::uint64_t Nps(const std::uint64_t nodes, const std::uint64_t ms) {
  return (1000 * nodes) / std::max<std::uint64_t>(1, ms);
}

// Is (x, y) on board ? Slow, but only for init
bool OnBoard(const int x, const int y) {
  return x >= 0 && x <= 7 && y >= 0 && y <= 7;
}

// X-coord to char
char File2Char(const int f) {
  return 'a' + f;
}

// Y-coord to char
char Rank2Char(const int r) {
  return '1' + r;
}

// Convert int coords to string
const std::string Move2Str(const int from, const int to) {
  return std::string{File2Char(Xaxl(from)), Rank2Char(Yaxl(from)), File2Char(Xaxl(to)), Rank2Char(Yaxl(to))};
}

// =n / =b / =r / =q -> Char
char PromoLetter(const std::int8_t piece) {
  return "nbrq"[std::abs(piece) - 2];
}

extern "C" {
// See if cin has smt
bool InputAvailable() {
#ifdef WINDOWS
  return _kbhit();
#else
  fd_set fd;
  struct timeval tv = {0, 0};
  FD_ZERO(&fd);
  FD_SET(STDIN_FILENO, &fd);
  select(STDIN_FILENO + 1, &fd, nullptr, nullptr, &tv);
  return FD_ISSET(STDIN_FILENO, &fd) > 0;
#endif
}
} // extern "C"

// ms since 1970
inline std::uint64_t Now() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()) .count();
}

// Deterministic Rand()
std::uint64_t Random64() {
  static std::uint64_t a = 0X12311227ULL, b = 0X1931311ULL, c = 0X13138141ULL;
#define MIXER(num) (((num) << 7) ^ ((num) >> 5))
  a ^= b + c;
  b ^= b * c + 0x1717711ULL;
  c  = (3 * c) + 0x1ULL;
  return MIXER(a) ^ MIXER(b) ^ MIXER(c);
}

// Really random for zobrist
std::uint64_t Random8x64() {
  std::uint64_t ret = 0;
  for (auto i = 0; i < 8; ++i) ret ^= Random64() << (8 * i);
  return ret;
}

// Nondeterministic Rand()
int Random(const int min, const int max) {
  static std::uint64_t seed = 0x202c7ULL + static_cast<std::uint64_t>(std::time(nullptr));
  return min + ((seed = (seed << 5) ^ (seed + 1) ^ (seed >> 3)) % std::max<std::uint64_t>(1, std::abs(max - min) + 1));
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

// Read input from cin
void ReadInput() {
  std::string line{};
  std::getline(std::cin, line);
  g_tokens_nth = 0;
  g_tokens.clear();
  SplitString< std::vector<std::string> >(line, g_tokens);
}

// PolyGlot Book lib

void SetBook(const std::string &book_file) { // x.nnue
  g_book_exist = book_file.length() <= 1 ? false : g_book.open_book(book_file);
}

// NNUE lib

void SetNNUE(const std::string &eval_file) { // x.bin
  g_nnue_exist = eval_file.length() <= 1 ? false : nnue::nnue_init(eval_file.c_str());
}

// Hashtable

void SetHashtable(int hash_mb) {
  hash_mb = std::clamp(hash_mb, 1, 1048576); // Limits 1MB -> 1TB
  // Hash in B / block in B
  g_hash_entries = static_cast<std::uint32_t>((1 << 20) * hash_mb) / (sizeof(HashEntry));
  g_hash.reset(new HashEntry[g_hash_entries]); // Claim space
}

// Hash

inline std::uint64_t Hash(const bool wtm) {
  auto ret = g_zobrist_ep[g_board->epsq + 1] ^ g_zobrist_wtm[wtm] ^ g_zobrist_castle[g_board->castle];
  for (auto both = Both(); both; ) {
    const auto sq = CtzPop(&both);
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
  } else if constexpr (type == MoveType::kGood) {
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

inline bool Board::is_underpromo() const { // =n / =b / =r
  return this->type == 5 || this->type == 6 || this->type == 7;
}

const std::string Board::movename() const {
  auto from2 = this->from, to2 = this->to;

  switch (this->type) {
    case 1: from2 = g_king_w, to2 = g_chess960 ? g_rook_w[0] : 6;      break;
    case 2: from2 = g_king_w, to2 = g_chess960 ? g_rook_w[1] : 2;      break;
    case 3: from2 = g_king_b, to2 = g_chess960 ? g_rook_b[0] : 56 + 6; break;
    case 4: from2 = g_king_b, to2 = g_chess960 ? g_rook_b[1] : 56 + 2; break;
    case 5: case 6: case 7: case 8:
            return Move2Str(from2, to2) + PromoLetter(this->pieces[to2]);
  }

  return Move2Str(from2, to2);
}

// Board presentation in FEN ( Forsyth–Edwards Notation )
const std::string Board::to_fen() const {
  std::stringstream s{};
  for (auto r = 7; r >= 0; --r) {
    auto empty = 0;
    for (auto f = 0; f <= 7; ++f)
      if (const auto p = "kqrbnp.PNBRQK"[this->pieces[8 * r + f] + 6]; p == '.') {
        ++empty;
      } else {
        if (empty) s << empty, empty = 0;
        s << p;
      }
    if (empty)  s << empty;
    if (r != 0) s << "/";
  }
  s << (g_wtm ? " w " : " b ");
  if (this->castle & 0x1) s << static_cast<char>('A' + g_rook_w[0]);
  if (this->castle & 0x2) s << static_cast<char>('A' + g_rook_w[1]);
  if (this->castle & 0x4) s << static_cast<char>('a' + g_rook_b[0] - 56);
  if (this->castle & 0x8) s << static_cast<char>('a' + g_rook_b[1] - 56);
  s << (this->castle ? " " : "- ");
  this->epsq == -1 ? s << "-" : s << static_cast<char>('a' + Xaxl(this->epsq)) << static_cast<char>('1' + Yaxl(this->epsq));
  s << " " << static_cast<int>(this->fifty) << " " << static_cast<int>(std::max(1, g_fullmoves));
  return s.str();
}

// String presentation of board
const std::string Board::to_s() const {
  std::stringstream s{};
  s << " +---+---+---+---+---+---+---+---+\n";
  for (auto r = 7; r >= 0; --r) {
    for (auto f = 0; f <= 7; ++f)
      s << " | " << "kqrbnp PNBRQK"[this->pieces[8 * r + f] + 6];
    s << " | " << (1 + r) << "\n +---+---+---+---+---+---+---+---+\n";
  }
  s << "   a   b   c   d   e   f   g   h\n\n" <<
    "> " << this->to_fen() << "\n" <<
    "> NNUE: " << (g_nnue_exist ? "OK" : "FAIL") << " / " <<
    "Book: " << (g_book_exist ? "OK" : "FAIL") << " / " <<
    "Eval: " << Evaluate(g_wtm) << " / " <<
    "Hash: " << g_hash_entries;
  return s.str();
}

// Tokenizer

bool TokenOk(const std::uint32_t nth = 0) {
  return g_tokens_nth + nth < g_tokens.size(); // O(1)
}

const std::string TokenNth(const std::uint32_t nth = 0) {
  return TokenOk(nth) ? g_tokens[g_tokens_nth + nth] : "";
}

void TokenPop(const std::uint32_t nth = 1) {
  g_tokens_nth += nth;
}

bool TokenPeek(const std::string &token, const std::uint32_t nth = 0) {
  return TokenOk(nth) && token == g_tokens[g_tokens_nth + nth];
}

// If true then pop n
bool Token(const std::string &token, const std::uint32_t pop_n = 1) {
  if (TokenPeek(token)) {
    TokenPop(pop_n);
    return true;
  }
  return false;
}

int TokenNumber(const std::uint32_t nth = 0) {
  return TokenOk(nth) ? std::stoi(g_tokens[g_tokens_nth + nth]) : 0;
}

// Fen handling

std::uint64_t Fill(int from, const int to) { // from / to -> Always good
  auto ret = Bit(from); // Build filled bitboard
  if (from == to) return ret;
  const auto diff = from > to ? -1 : +1;
  do { from += diff; ret |= Bit(from); } while (from != to);
  return ret;
}

void BuildCastlingBitboard1W() {
  if (g_board->castle & 0x1) { // White: O-O
    g_castle_w[0]       = Fill(g_king_w, 6);
    g_castle_empty_w[0] = (g_castle_w[0] | Fill(g_rook_w[0], 5)) ^ (Bit(g_king_w) | Bit(g_rook_w[0]));
  }

  if (g_board->castle & 0x2) { // White: O-O-O
    g_castle_w[1]       = Fill(g_king_w, 2);
    g_castle_empty_w[1] = (g_castle_w[1] | Fill(g_rook_w[1], 3)) ^ (Bit(g_king_w) | Bit(g_rook_w[1]));
  }
}

void BuildCastlingBitboard1B() {
  if (g_board->castle & 0x4) { // Black: O-O
    g_castle_b[0]       = Fill(g_king_b, 56 + 6);
    g_castle_empty_b[0] = (g_castle_b[0] | Fill(g_rook_b[0], 56 + 5)) ^ (Bit(g_king_b) | Bit(g_rook_b[0]));
  }

  if (g_board->castle & 0x8) { // Black: O-O-O
    g_castle_b[1]       = Fill(g_king_b, 56 + 2);
    g_castle_empty_b[1] = (g_castle_b[1] | Fill(g_rook_b[1], 56 + 3)) ^ (Bit(g_king_b) | Bit(g_rook_b[1]));
  }
}

void BuildCastlingBitboard2() {
  for (const auto i : {0, 1}) {
    g_castle_empty_w[i] &= 0xFFULL;
    g_castle_empty_b[i] &= 0xFF00000000000000ULL;
    g_castle_w[i]       &= 0xFFULL;
    g_castle_b[i]       &= 0xFF00000000000000ULL;
  }
}

void BuildCastlingBitboards() {
  BuildCastlingBitboard1W();
  BuildCastlingBitboard1B();
  BuildCastlingBitboard2();
}

void PutPiece(const int sq, const int p) {
  // Find kings too
  if      (p == +6) g_king_w = sq; // K
  else if (p == -6) g_king_b = sq; // k

  // Put piece on board
  g_board->pieces[sq] = p;

  // Create bitboards
  if      (p > 0) g_board->white[+p - 1] |= Bit(sq);
  else if (p < 0) g_board->black[-p - 1] |= Bit(sq);
}

int Piece2Num(const char p) { // Convert piece (Char) -> Int
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
    default:  return  0; // Impossible
  }
}

int Empty2Num(const char e) { // Empty cells (Char) -> Int
  return e - '0';
}

int File2Num(const char f) { // X-coord (Char) -> Int
  return f - 'a';
}

int Rank2Num(const char r) { // Ep Y-coord (Char) -> Int
  return r == '3' ? 2 : 5; // '3' / '6'
}

void FenBoard(const std::string &board) {
  auto sq = 56;
  for (std::size_t i = 0; i < board.length() && sq >= 0; ++i) // O(n)
    if (const auto c = board[i]; c == '/') sq -= 16;
    else if (std::isdigit(c))              sq += Empty2Num(c);
    else                                   PutPiece(sq++, Piece2Num(c));
}

void FenAddCastle(int *rooks, const int sq, const int castle) {
  *rooks           = sq;
  g_board->castle |= castle;
}

void FenAddChess960Castling(const char file) {
  if (file >= 'A' && file <= 'H') {
    if (const auto tmp = file - 'A'; tmp > g_king_w)
      FenAddCastle(g_rook_w + 0, tmp, 0x1);
    else if (tmp < g_king_w)
      FenAddCastle(g_rook_w + 1, tmp, 0x2);
  } else if (file >= 'a' && file <= 'h') {
    if (const auto tmp = (file - 'a') + 56; tmp > g_king_b)
      FenAddCastle(g_rook_b + 0, tmp, 0x4);
    else if (tmp < g_king_b)
      FenAddCastle(g_rook_b + 1, tmp, 0x8);
  }
}

void FenKQkq(const std::string &KQkq) {
  for (std::size_t i = 0; i < KQkq.length(); ++i)
    switch (const auto f = KQkq[i]) {
      case 'K': FenAddCastle(g_rook_w + 0, 7,      0x1); break;
      case 'Q': FenAddCastle(g_rook_w + 1, 0,      0x2); break;
      case 'k': FenAddCastle(g_rook_b + 0, 56 + 7, 0x4); break;
      case 'q': FenAddCastle(g_rook_b + 1, 56 + 0, 0x8); break;
      default:  FenAddChess960Castling(f);               break;
    }
}

void FenEp(const std::string &ep) {
  if (ep.length() == 2) g_board->epsq = 8 * Rank2Num(ep[1]) + File2Num(ep[0]);
}

void FenRule50(const std::string &fifty) {
  if (fifty.length() != 0 && fifty[0] != '-') g_board->fifty = std::clamp(std::stoi(fifty), 0, 100);
}

void FenFullMoves(const std::string &fullmoves) {
  if (fullmoves.length() != 0) g_fullmoves = std::max(std::stoi(fullmoves), 1);
}

// Fully legal FEN expected
void FenGen(const std::string &fen) {
  std::vector<std::string> tokens{};
  SplitString< std::vector<std::string> >(fen, tokens);
  if (fen.length() < 23 || // "8/8/8/8/8/8/8/8 w - - 0 1"
      tokens.size() < 6 ||
      tokens[0].find('K') == std::string::npos ||
      tokens[0].find('k') == std::string::npos)
    throw std::runtime_error("info string Bad fen: " + fen);

  FenBoard(tokens[0]);
  g_wtm = tokens[1][0] == 'w';
  FenKQkq(tokens[2]);
  FenEp(tokens[3]);
  FenRule50(tokens[4]);
  FenFullMoves(tokens[5]);
  BuildCastlingBitboards();
}

// Reset board
void FenReset() {
  g_board_empty = {};
  g_board       = &g_board_empty;
  g_wtm         = true;
  g_board->epsq = -1;
  g_king_w      = g_king_b = 0;
  g_fullmoves   = 1;

  for (const auto i : {0, 1}) {
    g_castle_w[i] = g_castle_empty_w[i] = g_castle_b[i] = g_castle_empty_b[i] = 0;
    g_rook_w[i]   = g_rook_b[i] = 0;
  }

  for (auto i = 0; i < 6; ++i)
    g_board->white[i] = g_board->black[i] = 0;
}

void Fen(const std::string &fen) {
  FenReset();
  FenGen(fen);
}

// Checks

inline bool ChecksHereW(const int sq) {
  const auto both = Both();
  return (g_pawn_checks_b[sq]        &  g_board->white[0]) |
         (g_knight_moves[sq]         &  g_board->white[1]) |
         (BishopMagicMoves(sq, both) & (g_board->white[2] | g_board->white[4])) |
         (RookMagicMoves(sq, both)   & (g_board->white[3] | g_board->white[4])) |
         (g_king_moves[sq]           &  g_board->white[5]);
}

inline bool ChecksHereB(const int sq) {
  const auto both = Both();
  return (g_pawn_checks_w[sq]        &  g_board->black[0]) |
         (g_knight_moves[sq]         &  g_board->black[1]) |
         (BishopMagicMoves(sq, both) & (g_board->black[2] | g_board->black[4])) |
         (RookMagicMoves(sq, both)   & (g_board->black[3] | g_board->black[4])) |
         (g_king_moves[sq]           &  g_board->black[5]);
}

bool ChecksCastleW(std::uint64_t squares) {
  while (squares)
    if (ChecksHereW(CtzPop(&squares)))
      return true;
  return false;
}

bool ChecksCastleB(std::uint64_t squares) {
  while (squares)
    if (ChecksHereB(CtzPop(&squares)))
      return true;
  return false;
}

inline bool ChecksW() {
  return ChecksHereW(Ctz(g_board->black[5]));
}

inline bool ChecksB() {
  return ChecksHereB(Ctz(g_board->white[5]));
}

// Sorting

// Only sort when necessary (See: lazy-sorting-algorithm paper)
// Sort only node-by-node (Avoid costly n! / tons of operations)
// Swap every nodes for simplicity
inline void LazySort(const int ply, const int nth, const int total_moves) {
  for (auto i = nth + 1; i < total_moves; ++i)
    if (g_boards[ply][i].score > g_boards[ply][nth].score)
      std::swap(g_boards[ply][nth], g_boards[ply][i]);
}

// 1. Evaluate all root moves
void EvalRootMoves() {
  for (auto i = 0; i < g_root_n; ++i)
    g_board         = g_boards[0] + i, // Pointer to this board
    g_board->score += (g_board->type == 8 ? 1000 : 0) + // =q
                      (g_board->type >= 1 && g_board->type <= 4 ? 100 : 0) + // OO|OOO
                      (g_board->is_underpromo() ? -5000 : 0) + // =r / =b / =n
                      (g_noise ? Random(-g_noise, +g_noise) : 0) + // Add noise -> Make unpredictable
                      (g_wtm ? +1 : -1) * Evaluate(g_wtm); // Full eval
}

// 2. Then sort root moves
struct RootCompFunctor { bool operator()(const Board &a, const Board &b) const { return a.score > b.score; } };
void SortRootMoves() {
  std::sort(g_boards[0] + 0, g_boards[0] + g_root_n, RootCompFunctor()); // 9 -> 0
}

void SortRoot(const int index) {
  if (index) {
    const auto tmp = g_boards[0][index];
    for (auto i = index; i > 0; --i)
      g_boards[0][i] = g_boards[0][i - 1];
    g_boards[0][0] = tmp;
  }
}

void SwapMoveInRootList(const int index) {
  if (index)
    std::swap(g_boards[0][0], g_boards[0][index]);
}

// Move generator

inline std::uint64_t BishopMagicIndex(const int sq, const std::uint64_t mask) {
  return ((mask & kBishopMagics[1][sq]) * kBishopMagics[0][sq]) >> 55;
}

inline std::uint64_t RookMagicIndex(const int sq, const std::uint64_t mask) {
  return ((mask & kRookMagics[1][sq]) * kRookMagics[0][sq]) >> 52;
}

inline std::uint64_t BishopMagicMoves(const int sq, const std::uint64_t mask) {
  return g_bishop_magic_moves[sq][BishopMagicIndex(sq, mask)];
}

inline std::uint64_t RookMagicMoves(const int sq, const std::uint64_t mask) {
  return g_rook_magic_moves[sq][RookMagicIndex(sq, mask)];
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

  if (!ChecksB()) g_board->index = g_moves_n++;
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

  if (!ChecksW()) g_board->index = g_moves_n++;
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

  if (!ChecksB()) g_board->index = g_moves_n++;
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

  if (!ChecksW()) g_board->index = g_moves_n++;
}

void AddOOW() {
  if ((g_board->castle & 0x1) && !(g_castle_empty_w[0] & g_both))
    AddCastleOOW(), g_board = g_board_orig;
}

void AddOOOW() {
  if ((g_board->castle & 0x2) && !(g_castle_empty_w[1] & g_both))
    AddCastleOOOW(), g_board = g_board_orig;
}

void MgenCastlingMovesW() {
  AddOOW();
  AddOOOW();
}

void AddOOB() {
  if ((g_board->castle & 0x4) && !(g_castle_empty_b[0] & g_both))
    AddCastleOOB(), g_board = g_board_orig;
}

void AddOOOB() {
  if ((g_board->castle & 0x8) && !(g_castle_empty_b[1] & g_both))
    AddCastleOOOB(), g_board = g_board_orig;
}

void MgenCastlingMovesB() {
  AddOOB();
  AddOOOB();
}

void CheckCastlingRightsW() {
  if (g_board->pieces[g_king_w]    != +6) {g_board->castle &= 0x4 | 0x8; return;}
  if (g_board->pieces[g_rook_w[0]] != +4) {g_board->castle &= 0x2 | 0x4 | 0x8;}
  if (g_board->pieces[g_rook_w[1]] != +4) {g_board->castle &= 0x1 | 0x4 | 0x8;}
}

void CheckCastlingRightsB() {
  if (g_board->pieces[g_king_b]    != -6) {g_board->castle &= 0x1 | 0x2; return;}
  if (g_board->pieces[g_rook_b[0]] != -4) {g_board->castle &= 0x1 | 0x2 | 0x8;}
  if (g_board->pieces[g_rook_b[1]] != -4) {g_board->castle &= 0x1 | 0x2 | 0x4;}
}

void HandleCastlingRights() {
  if (g_board->castle) {
    CheckCastlingRightsW();
    CheckCastlingRightsB();
  }
}

void ModifyPawnStuffW(const int from, const int to) {
  if (g_board->pieces[to] != +1) return;

  g_board->fifty = 0;
  if (to == g_board_orig->epsq) {
    g_board->score          = 10; // PxP
    g_board->pieces[to - 8] = 0;
    g_board->black[0]      ^= Bit(to - 8);
  } else if (Yaxl(from) == 1 && Yaxl(to) == 3) { // e2e4 ...
    g_board->epsq = to - 8;
  } else if (Yaxl(to) == 6) { // Bonus for 7th ranks
    g_board->score = 85 + Yaxl(to);
  }
}

void ModifyPawnStuffB(const int from, const int to) {
  if (g_board->pieces[to] != -1) return;

  g_board->fifty = 0;
  if (to == g_board_orig->epsq) {
    g_board->score          = 10;
    g_board->pieces[to + 8] = 0;
    g_board->white[0]      ^= Bit(to + 8);
  } else if (Yaxl(from) == 6 && Yaxl(to) == 4) {
    g_board->epsq = to + 8;
  } else if (Yaxl(to) == 1) {
    g_board->score = 85 + 7 - Yaxl(to);
  }
}

void AddPromotionW(const int from, const int to, const int piece) {
  const auto eat = g_board->pieces[to];

  g_moves[g_moves_n]        = *g_board;
  g_board                   = &g_moves[g_moves_n];
  g_board->from             = from;
  g_board->to               = to;
  g_board->score            = 110 + piece; // Highest priority
  g_board->type             = 3 + piece;
  g_board->epsq             = -1;
  g_board->fifty            = 0;
  g_board->pieces[to]       = piece;
  g_board->pieces[from]     = 0;
  g_board->white[0]         ^= Bit(from);
  g_board->white[piece - 1] |= Bit(to);

  if (eat <= -1)  g_board->black[-eat - 1] ^= Bit(to);
  if (!ChecksB()) HandleCastlingRights(), g_board->index = g_moves_n++;
}

void AddPromotionB(const int from, const int to, const int piece) {
  const auto eat = g_board->pieces[to];

  g_moves[g_moves_n]    = *g_board;
  g_board               = &g_moves[g_moves_n];
  g_board->from         = from;
  g_board->to           = to;
  g_board->score        = 110 + (-piece);
  g_board->type         = 3 + (-piece);
  g_board->epsq         = -1;
  g_board->fifty        = 0;
  g_board->pieces[from] = 0;
  g_board->pieces[to]   = piece;
  g_board->black[0]    ^= Bit(from);
  g_board->black[-piece - 1] |= Bit(to);

  if (eat >= +1)  g_board->white[eat - 1] ^= Bit(to);
  if (!ChecksW()) HandleCastlingRights(), g_board->index = g_moves_n++;
}

void AddPromotionStuffW(const int from, const int to) {
  if (g_underpromos) { for (const auto p : {+5, +4, +3, +2}) AddPromotionW(from, to, p), g_board = g_board_orig; } // QRBN
  else {               for (const auto p : {+5, +2})         AddPromotionW(from, to, p), g_board = g_board_orig; } // QN
}

void AddPromotionStuffB(const int from, const int to) {
  if (g_underpromos) { for (const auto p : {-5, -4, -3, -2}) AddPromotionB(from, to, p), g_board = g_board_orig; }
  else {               for (const auto p : {-5, -2})         AddPromotionB(from, to, p), g_board = g_board_orig; }
}

inline void CheckNormalCapturesW(const int me, const int eat, const int to) {
  if (eat <= -1) {
    g_board->black[-eat - 1] ^= Bit(to);
    g_board->score            = kMvv[me - 1][-eat - 1];
    g_board->fifty            = 0;
  }
}

inline void CheckNormalCapturesB(const int me, const int eat, const int to) {
  if (eat >= +1) {
    g_board->white[eat - 1] ^= Bit(to);
    g_board->score           = kMvv[-me - 1][eat - 1];
    g_board->fifty           = 0;
  }
}

// If not under checks -> Handle castling rights -> Add move
inline void AddMoveIfOkW() {
  if (!ChecksB())
    HandleCastlingRights(), g_board->index = g_moves_n++;
}

inline void AddMoveIfOkB() {
  if (!ChecksW())
    HandleCastlingRights(), g_board->index = g_moves_n++;
}

void AddNormalStuffW(const int from, const int to) {
  const auto me = g_board->pieces[from], eat = g_board->pieces[to];

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
  ++g_board->fifty; // Rule50 counter increased after non-decisive move

  CheckNormalCapturesW(me, eat, to);
  ModifyPawnStuffW(from, to);
  AddMoveIfOkW();
  g_board = g_board_orig; // Back to old board
}

void AddNormalStuffB(const int from, const int to) {
  const auto me = g_board->pieces[from], eat = g_board->pieces[to];

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
  ++g_board->fifty;

  CheckNormalCapturesB(me, eat, to);
  ModifyPawnStuffB(from, to);
  AddMoveIfOkB();
  g_board = g_board_orig;
}

void AddW(const int from, const int to) {
  g_board->pieces[from] == +1 && Yaxl(from) == 6 ? AddPromotionStuffW(from, to) : AddNormalStuffW(from, to);
}

void AddB(const int from, const int to) {
  g_board->pieces[from] == -1 && Yaxl(from) == 1 ? AddPromotionStuffB(from, to) : AddNormalStuffB(from, to);
}

void AddMovesW(const int from, std::uint64_t moves) {
  while (moves) AddW(from, CtzPop(&moves));
}

void AddMovesB(const int from, std::uint64_t moves) {
  while (moves) AddB(from, CtzPop(&moves));
}

void MgenPawnsW() {
  for (auto p = g_board->white[0]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesW(sq, g_pawn_checks_w[sq] & g_pawn_sq);
    if (Yaxl(sq) == 1) {
      if (g_pawn_1_moves_w[sq] & g_empty)
        AddMovesW(sq, g_pawn_2_moves_w[sq] & g_empty);
    } else {
      AddMovesW(sq, g_pawn_1_moves_w[sq] & g_empty);
    }
  }
}

void MgenPawnsB() {
  for (auto p = g_board->black[0]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesB(sq, g_pawn_checks_b[sq] & g_pawn_sq);
    if (Yaxl(sq) == 6) {
      if (g_pawn_1_moves_b[sq] & g_empty)
        AddMovesB(sq, g_pawn_2_moves_b[sq] & g_empty);
    } else {
      AddMovesB(sq, g_pawn_1_moves_b[sq] & g_empty);
    }
  }
}

void MgenPawnsOnlyCapturesW() {
  for (auto p = g_board->white[0]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesW(sq, Yaxl(sq) == 6 ? g_pawn_1_moves_w[sq] & (~g_both) : g_pawn_checks_w[sq] & g_pawn_sq);
  }
}

void MgenPawnsOnlyCapturesB() {
  for (auto p = g_board->black[0]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesB(sq, Yaxl(sq) == 1 ? g_pawn_1_moves_b[sq] & (~g_both) : g_pawn_checks_b[sq] & g_pawn_sq);
  }
}

void MgenKnightsW() {
  for (auto p = g_board->white[1]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesW(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenKnightsB() {
  for (auto p = g_board->black[1]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesB(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenBishopsPlusQueensW() {
  for (auto p = g_board->white[2] | g_board->white[4]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesW(sq, BishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenBishopsPlusQueensB() {
  for (auto p = g_board->black[2] | g_board->black[4]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesB(sq, BishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensW() {
  for (auto p = g_board->white[3] | g_board->white[4]; p; ) {
    const auto sq = CtzPop(&p);
    AddMovesW(sq, RookMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensB() {
  for (auto p = g_board->black[3] | g_board->black[4]; p; ) {
    const auto sq = CtzPop(&p);
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

inline void MgenSetupBoth() {
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

inline void MgenReset(Board *moves) {
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
int MgenRoot() {
  return g_wtm ? MgenW(g_boards[0]) : MgenB(g_boards[0]);
}

// Evaluation

#define FRC_PENALTY        400
#define TEMPO_BONUS        25
#define BISHOP_PAIR_BONUS  20
#define CHECKS_BONUS       17

// Probe Eucalyptus KPK bitbases -> true: draw -> false: not draw
inline bool ProbeKPK(const bool wtm) {
  return g_board->white[0] ?
    eucalyptus::IsDraw(     Ctz(g_board->white[5]),      Ctz(g_board->white[0]),      Ctz(g_board->black[5]),  wtm) :
    eucalyptus::IsDraw(63 - Ctz(g_board->black[5]), 63 - Ctz(g_board->black[0]), 63 - Ctz(g_board->white[5]), !wtm);
}

// Detect trivial draws really fast
bool EasyDraw(const bool wtm) {
  if (g_board->white[3] | g_board->white[4] | g_board->black[3] | g_board->black[4]) // R/Q/r/q -> No draw
    return false;

  if (g_board->white[1] | g_board->white[2] | g_board->black[1] | g_board->black[2]) // N/B/n/b -> Drawish ?
    return (g_board->white[0] | g_board->black[0]) ?
      false : // Pawns ? -> No draw
      PopCount(g_board->white[1] | g_board->white[2]) <= 1 && // Max 1 N/B per side -> Draw
      PopCount(g_board->black[1] | g_board->black[2]) <= 1;

  const auto pawns_n = PopCount(g_board->white[0] | g_board->black[0]); // No N/B/R/Q/n/b/r/q -> Pawns ?
  return pawns_n == 1 ? ProbeKPK(wtm) : (pawns_n == 0); // Check KPK ? / Bare kings ? -> Draw
}

// Cornered bishop penalty in FRC
// Bishop on a1/h1/a8/h8 blocked by own pawn
int FixFRC() {
  // No bishop in corner -> Speedup
  static const std::uint64_t corners = Bit(0) | Bit(7) | Bit(56) | Bit(63);
  if (!((g_board->white[2] | g_board->black[2]) & corners))
    return 0;

  auto s = 0;
  if (g_board->pieces[0]  == +3 && g_board->pieces[9]  == +1) s -= FRC_PENALTY;
  if (g_board->pieces[7]  == +3 && g_board->pieces[14] == +1) s -= FRC_PENALTY;
  if (g_board->pieces[56] == -3 && g_board->pieces[49] == -1) s += FRC_PENALTY;
  if (g_board->pieces[63] == -3 && g_board->pieces[54] == -1) s += FRC_PENALTY;
  return s;
}

// Classical evaluation (HCE)

struct ClassicalEval { // Finish the game or no NNUE
  const std::uint64_t white, black, both;
  const bool m_wtm; // Avoid collision w/ template wtm
  int w_pieces[5]{}, b_pieces[5]{}, white_total{0}, black_total{0}, both_total{0},
      wk{0}, bk{0}, score{0}, mg{0}, eg{0}, scale_factor{1};

  explicit ClassicalEval(const bool wtm) : white{White()}, black{Black()}, both{this->white | this->black}, m_wtm{wtm} { }

  inline int flip_y(const int sq) const { return sq ^ 56; } // Mirror horizontal
  int square(const int x) const { return x * x; }

  int close_bonus(const int a, const int b) const {
    return this->square(7 - std::abs(Xaxl(a) - Xaxl(b))) + this->square(7 - std::abs(Yaxl(a) - Yaxl(b)));
  }

  int close_any_corner_bonus(const int sq) const {
    return std::max({this->close_bonus(sq, 0),  this->close_bonus(sq, 7), this->close_bonus(sq, 56), this->close_bonus(sq, 63)});
  }

  template<bool wtm>
  void check_blind_bishop() {
    if constexpr (wtm) {
      const auto wpx   = Xaxl(Ctz(g_board->white[0]));
      const auto color = g_board->white[2] & 0x55aa55aa55aa55aaULL;
      if ((color && wpx == 7) || (!color && wpx == 0)) this->scale_factor = 2;
    } else {
      const auto bpx   = Xaxl(Ctz(g_board->black[0]));
      const auto color = g_board->black[2] & 0x55aa55aa55aa55aaULL;
      if ((!color && bpx == 7) || (color && bpx == 0)) this->scale_factor = 2;
    }
  }

  template<bool wtm, int p>
  inline void pesto(const int sq) {
    if constexpr (wtm) {
      this->mg += kPesto[p][0][sq];
      this->eg += kPesto[p][1][sq];
    } else {
      this->mg -= kPesto[p][0][this->flip_y(sq)];
      this->eg -= kPesto[p][1][this->flip_y(sq)];
    }
  }

  template<bool wtm, int piece>
  inline void inc_piece_count() {
    if constexpr (wtm) ++this->w_pieces[piece];
    else               ++this->b_pieces[piece];
  }

  // Squares not having own pieces are reachable
  template<bool wtm>
  inline std::uint64_t reachable() const {
    if constexpr (wtm) return ~this->white;
    else               return ~this->black;
  }

  template<bool wtm, int k>
  inline void mobility(const std::uint64_t m) {
    if constexpr (wtm) this->score += k * PopCount(m);
    else               this->score -= k * PopCount(m);
  }

  template<bool wtm, int piece, int k>
  inline void eval_score(const int sq, const std::uint64_t m) {
    this->inc_piece_count<wtm, piece>();
    this->pesto<wtm, piece>(sq);
    this->mobility<wtm, k>(m);
  }

  template<bool wtm>
  void pawn(const int sq) {
    this->inc_piece_count<wtm, 0>();
    this->pesto<wtm, 0>(sq);
  }

  template<bool wtm>
  void knight(const int sq) {
    this->eval_score<wtm, 1, 2>(sq, g_knight_moves[sq] & this->reachable<wtm>());
  }

  template<bool wtm>
  void bishop(const int sq) {
    this->eval_score<wtm, 2, 3>(sq, BishopMagicMoves(sq, this->both) & this->reachable<wtm>());
  }

  template<bool wtm>
  void rook(const int sq) {
    this->eval_score<wtm, 3, 3>(sq, RookMagicMoves(sq, this->both) & this->reachable<wtm>());
  }

  template<bool wtm>
  void queen(const int sq) {
    this->eval_score<wtm, 4, 2>(sq, (BishopMagicMoves(sq, this->both) | RookMagicMoves(sq, this->both)) & this->reachable<wtm>());
  }

  template<bool wtm>
  void king(const int sq) {
    if constexpr (wtm) this->wk = sq;
    else               this->bk = sq;
    this->pesto<wtm, 5>(sq);
    this->mobility<wtm, 1>(g_king_moves[sq] & this->reachable<wtm>());
  }

  void eval_piece(const int sq) {
    switch (g_board->pieces[sq]) {
      case +1: this->pawn<true>(sq);    break;
      case +2: this->knight<true>(sq);  break;
      case +3: this->bishop<true>(sq);  break;
      case +4: this->rook<true>(sq);    break;
      case +5: this->queen<true>(sq);   break;
      case +6: this->king<true>(sq);    break;
      case -1: this->pawn<false>(sq);   break;
      case -2: this->knight<false>(sq); break;
      case -3: this->bishop<false>(sq); break;
      case -4: this->rook<false>(sq);   break;
      case -5: this->queen<false>(sq);  break;
      case -6: this->king<false>(sq);   break;
    }
  }

  void evaluate_pieces() {
    for (auto b = this->both; b; ) this->eval_piece(CtzPop(&b));
    this->white_total = std::accumulate(this->w_pieces + 0, this->w_pieces + 5, 1); // Start from 1 (king)
    this->black_total = std::accumulate(this->b_pieces + 0, this->b_pieces + 5, 1);
    this->both_total  = this->white_total + this->black_total;
  }

  template<bool wtm>
  void bonus_knbk() {
    if constexpr (wtm) this->score += (2 * this->close_bonus(this->wk, this->bk)) +
      (10 * ((g_board->white[2] & 0xaa55aa55aa55aa55ULL) ? std::max(this->close_bonus(0, this->bk), this->close_bonus(63, this->bk)) :
                                                           std::max(this->close_bonus(7, this->bk), this->close_bonus(56, this->bk))));
    else               this->score -= (2 * this->close_bonus(this->wk, this->bk)) +
      (10 * ((g_board->black[2] & 0xaa55aa55aa55aa55ULL) ? std::max(this->close_bonus(0, this->wk), this->close_bonus(63, this->wk)) :
                                                           std::max(this->close_bonus(7, this->wk), this->close_bonus(56, this->wk))));
  }

  void bonus_tempo() {
    this->score += this->m_wtm ? +TEMPO_BONUS : -TEMPO_BONUS;
  }

  void bonus_checks() {
    if (     ChecksW()) this->score += CHECKS_BONUS;
    else if (ChecksB()) this->score -= CHECKS_BONUS;
  }

  // Force black king in the corner and get closer
  template<bool wtm>
  void bonus_mating() {
    if constexpr (wtm) this->score += 6 * this->close_any_corner_bonus(this->bk) + 4 * this->close_bonus(this->wk, this->bk);
    else               this->score -= 6 * this->close_any_corner_bonus(this->wk) + 4 * this->close_bonus(this->bk, this->wk);
  }

#define WP(x) this->w_pieces[(x)]
#define BP(x) this->b_pieces[(x)]

  void bonus_bishop_pair() {
    if (WP(2) >= 2) this->score += BISHOP_PAIR_BONUS;
    if (BP(2) >= 2) this->score -= BISHOP_PAIR_BONUS;
  }

  // 1. KQvK(PNBR) -> White Checkmate
  // 2. K(PNBR)vKQ -> Black Checkmate
  // 3. KRvK(NB)   -> Drawish (Try to checkmate)
  // 4. K(NB)vKR   -> Drawish (Try to checkmate)
  void bonus_special_4men() {
    if (     WP(4) && (!BP(4)))         { this->bonus_mating<true>(); }
    else if (BP(4) && (!WP(4)))         { this->bonus_mating<false>(); }
    else if (WP(3) && (BP(1) || BP(2))) { this->scale_factor = 4; this->bonus_mating<true>(); }
    else if (BP(3) && (WP(1) || WP(2))) { this->scale_factor = 4; this->bonus_mating<false>(); }
  }

  // 1. KRRvKR / KR(NB)vK(NB)               -> White Checkmate
  // 2. KRvKRR / K(NB)vKR(NB)               -> Black Checkmate
  // 3. K(RQ)(PNB)vK(RQ) / K(RQ)vK(RQ)(PNB) -> Drawish
  void bonus_special_5men() {
    if (     (WP(3) == 2 && BP(3)) || (WP(3) && (WP(2) || WP(1)) && (BP(2) || BP(1)))) this->bonus_mating<true>();
    else if ((BP(3) == 2 && WP(3)) || (BP(3) && (BP(2) || BP(1)) && (WP(2) || WP(1)))) this->bonus_mating<false>();
    else if (((WP(3) && BP(3)) || (WP(4) && BP(4))) && ((WP(0) || WP(1) || WP(2)) || (BP(0) || BP(1) || BP(2))))
      this->scale_factor = 4;
  }

  // 1. Special mating pattern (knbk)
  // 2. Don't force king to corner    -> Try to promote
  // 3. Can't force mate w/ 2 knights -> Drawish
  void white_is_mating() {
    if (this->white_total == 3) {
      if (WP(2) && WP(1)) { this->bonus_knbk<true>();         return; }
      if (WP(2) && WP(0)) { this->check_blind_bishop<true>(); return; }
      if (WP(1) == 2)     { this->scale_factor = 4; }
    }
    this->bonus_mating<true>();
  }

  void black_is_mating() {
    if (this->black_total == 3) {
      if (BP(2) && BP(1)) { this->bonus_knbk<false>();         return; }
      if (BP(2) && BP(0)) { this->check_blind_bishop<false>(); return; }
      if (BP(1) == 2)     { this->scale_factor = 4; }
    }
    this->bonus_mating<false>();
  }

  // Special EG functions. To avoid always doing "Tabula rasa"
  void bonus_endgame() {
    if (     this->black_total == 1) this->white_is_mating();
    else if (this->white_total == 1) this->black_is_mating();
    else if (this->both_total  == 4) this->bonus_special_4men();
    else if (this->both_total  == 5) this->bonus_special_5men();
  }

  int calculate_score() const {
    // 30 phases for HCE (assume kings exist)
    const float n = std::max<float>(this->both_total - 2, 0) / static_cast<float>(MAX_PIECES - 2);
    const int s   = static_cast<int>(n * static_cast<float>(this->mg) + (1.0f - n) * static_cast<float>(this->eg));
    return (this->score + s) / this->scale_factor;
  }

  int evaluate() {
    this->evaluate_pieces();
    this->bonus_tempo();
    this->bonus_checks();
    this->bonus_bishop_pair();
    this->bonus_endgame();
    return this->calculate_score() + (FixFRC() / 4);
  }
};

// NNUE Eval

struct NnueEval {
  const bool wtm;

  explicit NnueEval(const bool wtm2) : wtm{wtm2} { } // explicit -> Force curly init

  int probe() const {
    auto i = 2;

    for (auto both = Both(); both; )
      switch (const auto sq = CtzPop(&both); g_board->pieces[sq]) {
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
    return this->probe() + FixFRC();
  }
};

int EvaluateClassical(const bool wtm) {
  ClassicalEval e{wtm};
  return e.evaluate();
}

int EvaluateNNUE(const bool wtm) {
  NnueEval e{wtm};
  return e.evaluate() / 4; // NNUE evals are 4x
}

// Add noise to eval for different playing levels
// 0    (Random Mover)
// 1-99 (Levels)
// 100  (Full Strength)
int LevelNoise() { // -5 -> +5 pawns
  return g_level == 100 ? 0 : Random(-5 * (100 - g_level), +5 * (100 - g_level));
}

int Evaluate(const bool wtm) {
  return LevelNoise() + (EasyDraw(wtm) ? 0 :
    (g_scale[g_board->fifty] * (static_cast<float>(g_classical ? EvaluateClassical(wtm) : EvaluateNNUE(wtm)))));
}

// Search

void SpeakUci(const int score, const std::uint64_t ms) {
  std::cout << "info depth " << std::min(g_max_depth, g_depth + 1) <<
    " nodes " << g_nodes <<
    " time " << ms <<
    " nps " << Nps(g_nodes, ms) <<
    " score cp " << ((g_wtm ? +1 : -1) * (std::abs(score) == INF ? score / 100 : score)) <<
    " pv " << g_boards[0][0].movename() << std::endl; // flush
}

bool Draw(const bool wtm) {
  if (g_board->fifty > 100 || EasyDraw(wtm)) return true;

  const auto hash = g_r50_positions[g_board->fifty]; // g_r50_positions.pop() must contain hash !
  for (auto i = g_board->fifty - 2; i >= 0; i -= 2)
    if (g_r50_positions[i] == hash) // Only 2 rep
      return true;

  return false;
}

// Responding to "quit" / "stop" / "isready" signals
bool UserStop() {
  if (!InputAvailable()) return false;

  ReadInput();
  if (Token("isready")) {
    std::cout << "readyok" << std::endl;
    return false;
  }

  return Token("quit") ? !(g_game_on = false) : Token("stop");
}

inline bool CheckTime() { // Time checking
  static std::uint64_t ticks = 0;
  return ((++ticks) & READ_CLOCK) ? false : ((Now() >= g_stop_search_time) || UserStop());
}

// 1. Check against standpat to see whether we are better -> Done
// 2. Iterate deeper
int QSearchW(int alpha, const int beta, const int depth, const int ply) {
  ++g_nodes; // Increase visited nodes count

  if (g_stop_search || (g_stop_search = CheckTime())) return 0; // Search is stopped. Return ASAP
  if (((alpha = std::max(alpha, Evaluate(true))) >= beta) || depth <= 0) return alpha; // Better / terminal node -> Done

  const auto moves_n = MgenTacticalW(g_boards[ply]);
  for (auto i = 0; i < moves_n; ++i) {
    LazySort(ply, i, moves_n); // Very few moves, sort them all
    g_board = g_boards[ply] + i;
    if ((alpha = std::max(alpha, QSearchB(alpha, beta, depth - 1, ply + 1))) >= beta) return alpha;
  }

  return alpha;
}

int QSearchB(const int alpha, int beta, const int depth, const int ply) {
  ++g_nodes;

  if (g_stop_search) return 0;
  if ((alpha >= (beta = std::min(beta, Evaluate(false)))) || depth <= 0) return beta;

  const auto moves_n = MgenTacticalB(g_boards[ply]);
  for (auto i = 0; i < moves_n; ++i) {
    LazySort(ply, i, moves_n);
    g_board = g_boards[ply] + i;
    if (alpha >= (beta = std::min(beta, QSearchW(alpha, beta, depth - 1, ply + 1)))) return beta;
  }

  return beta;
}

// a >= b -> Minimizer won't pick any better move anyway.
//           So searching beyond is a waste of time.
int SearchMovesW(int alpha, const int beta, int depth, const int ply) {
  const auto hash    = g_r50_positions[g_board->fifty];
  const auto checks  = ChecksB();
  const auto moves_n = MgenW(g_boards[ply]);

  if (!moves_n) return checks ? -INF : 0; // Checkmate or stalemate
  if (moves_n == 1 || (depth == 1 && (checks || g_board->type == 8))) ++depth; // Extend interesting path (SRE / CE / PPE)

  const auto ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  auto *entry       = &g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)];
  entry->put_hash_value_to_moves(hash, g_boards[ply]);

  // Tiny speedup since not all moves are scored (lots of pointless shuffling ...)
  // So avoid sorting useless moves
  auto sort = true;
  for (auto i = 0; i < moves_n; ++i) {
    if (sort) LazySort(ply, i, moves_n), sort = g_boards[ply][i].score != 0;
    g_board = g_boards[ply] + i;
    g_is_pv = i <= 1 && !g_boards[ply][i].score;
    if (ok_lmr && i >= 1 && !g_board->score && !ChecksW()) {
      if (SearchB(alpha, beta, depth - 2 - g_lmr[depth][i], ply + 1) <= alpha) continue;
      g_board = g_boards[ply] + i;
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
  if (moves_n == 1 || (depth == 1 && (checks || g_board->type == 8))) ++depth;

  const auto ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  auto *entry       = &g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)];
  entry->put_hash_value_to_moves(hash, g_boards[ply]);

  auto sort = true;
  for (auto i = 0; i < moves_n; ++i) {
    if (sort) LazySort(ply, i, moves_n), sort = g_boards[ply][i].score != 0;
    g_board = g_boards[ply] + i;
    g_is_pv = i <= 1 && !g_boards[ply][i].score;
    if (ok_lmr && i >= 1 && !g_board->score && !ChecksB()) {
      if (SearchW(alpha, beta, depth - 2 - g_lmr[depth][i], ply + 1) >= beta) continue;
      g_board = g_boards[ply] + i;
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
      (depth >= 3) && // Enough depth ( 2 blunders too much. 3 sweet spot ... ) ?
      ((g_board->white[1] | g_board->white[2] | g_board->white[3] | g_board->white[4]) ||
       (PopCount(g_board->white[0]) >= 2)) && // Non pawn material or at least 2 pawns ( Zugzwang ... ) ?
      (!ChecksB()) && // Not under checks ?
      (Evaluate(true) >= beta)) { // Looks good ?
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
      (depth >= 3) &&
      ((g_board->black[1] | g_board->black[2] | g_board->black[3] | g_board->black[4]) ||
       (PopCount(g_board->black[0]) >= 2)) &&
      (!ChecksW()) &&
      (alpha >= Evaluate(false))) {
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
  ++g_nodes;

  if (g_stop_search || (g_stop_search = CheckTime())) return 0; // Search is stopped. Return ASAP
  if (depth <= 0 || ply >= MAX_DEPTH) return QSearchW(alpha, beta, g_q_depth, ply);

  const auto fifty = g_board->fifty;
  const auto tmp   = g_r50_positions[fifty];

  if (TryNullMoveW(&alpha, beta, depth, ply)) return alpha;

  g_r50_positions[fifty] = Hash(true);
  alpha                  = Draw(true) ? 0 : SearchMovesW(alpha, beta, depth, ply);
  g_r50_positions[fifty] = tmp;

  return alpha;
}

int SearchB(const int alpha, int beta, const int depth, const int ply) {
  ++g_nodes;

  if (g_stop_search) return 0;
  if (depth <= 0 || ply >= MAX_DEPTH) return QSearchB(alpha, beta, g_q_depth, ply);

  const auto fifty = g_board->fifty;
  const auto tmp   = g_r50_positions[fifty];

  if (TryNullMoveB(alpha, &beta, depth, ply)) return beta;

  g_r50_positions[fifty] = Hash(false);
  beta                   = Draw(false) ? 0 : SearchMovesB(alpha, beta, depth, ply);
  g_r50_positions[fifty] = tmp;

  return beta;
}

// Root search
int BestW() {
  auto best_i = 0, alpha = -INF;

  for (auto i = 0; i < g_root_n; ++i) {
    g_board = g_boards[0] + i;
    g_is_pv = i <= 1 && !g_boards[0][i].score; // 1 / 2 moves too good and not tactical -> pv
    int score;
    if (g_depth >= 1 && i >= 1) { // Null window search for bad moves
      if ((score = SearchB(alpha, alpha + 1, g_depth, 1)) > alpha)
        g_board = g_boards[0] + i, score = SearchB(alpha, +INF, g_depth, 1); // Search w/ full window
    } else {
      score = SearchB(alpha, +INF, g_depth, 1);
    }
    if (g_stop_search) return g_best_score;
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

int BestB() {
  auto best_i = 0, beta = +INF;

  for (auto i = 0; i < g_root_n; ++i) {
    g_board = g_boards[0] + i;
    g_is_pv = i <= 1 && !g_boards[0][i].score;
    int score;
    if (g_depth >= 1 && i >= 1) {
      if ((score = SearchW(beta - 1, beta, g_depth, 1)) < beta)
        g_board = g_boards[0] + i, score = SearchW(-INF, beta, g_depth, 1);
    } else {
      score = SearchW(-INF, beta, g_depth, 1);
    }
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
  const int white_n, black_n, both_n;

  Material() : white_n{PopCount(White())}, black_n{PopCount(Black())}, both_n{this->white_n + this->black_n} { }

  bool is_rook_ending() const { // KRRvKR / KRvKRR / KRRRvK / KvKRRR ?
    return this->both_n == 5 && (PopCount(g_board->white[3] | g_board->black[3]) == 3);
  }

  bool is_easy() const { // Vs king + (PNBRQ) ?
    return g_wtm ? this->black_n <= 2 : this->white_n <= 2;
  }

  bool is_endgame() const { // 5 pieces or less both side -> Endgame
    return g_wtm ? this->black_n <= 5 : this->white_n <= 5;
  }
};

bool HCEActivation(const Material &m) { // Activate HCE when ...
  return (!g_nnue_exist)    || // No NNUE
         m.is_easy()        || // Easy
         m.is_rook_ending() || // Rook ending
         m.both_n >= 33;       // Disable NNUE for 33+ pieces
}

// Play the book move from root list
bool FindBookMove(const int from, const int to, const int type) {
  if (type) {
    for (auto i = 0; i < g_root_n; ++i)
      if (g_boards[0][i].type == type) {
        SwapMoveInRootList(i);
        return true;
      }
  } else {
    for (auto i = 0; i < g_root_n; ++i)
      if (g_boards[0][i].from == from && g_boards[0][i].to == to) {
        SwapMoveInRootList(i);
        return true;
      }
  }
  return false;
}

int BookSolveType(const int from, const int to, const int move) {
  // PolyGlot promos (=n / =b / =r / =q)
  auto is_promo = [&move](const int v){ return move & (0x1 << (12 + v)); };
  constexpr std::array<int, 4> v = {0, 1, 2, 3};
  if (const auto res = std::find_if(v.begin(), v.end(), is_promo); res != v.end())
    return 5 + *res;

  // White: O-O / O-O-O
  if (g_board->pieces[from] == +6 && g_board->pieces[to] == +4)
    return to > from ? 1 : 2;

  // Black: O-O / O-O-O
  if (g_board->pieces[from] == -6 && g_board->pieces[to] == -4)
    return to > from ? 3 : 4;

  // Normal
  return 0;
}

bool ProbeBook() { // Probe PolyGlot book
  if (const auto move = g_book.setup(g_board->pieces, Both(), g_board->castle, g_board->epsq, g_wtm)
                              .probe(BOOK_BEST)) {
    const auto from = 8 * ((move >> 9) & 0x7) + ((move >> 6) & 0x7);
    const auto to   = 8 * ((move >> 3) & 0x7) + ((move >> 0) & 0x7);
    return FindBookMove(from, to, BookSolveType(from, to, move));
  }

  return false;
}

bool RandomMove() { // At level 0 we simply play a random move
  if (g_level == 0) {
    SwapMoveInRootList(Random(0, g_root_n - 1));
    return true;
  }
  return false;
}

bool FastMove(const int ms) {
  if ((g_root_n <= 1) || // Only move
      (ms <= 1)       || // Hurry up !
      (RandomMove())  || // Random mover
      (g_book_exist && ms > BOOK_MS && ProbeBook())) { // Try book
    SpeakUci(g_last_eval, 0);
    return true;
  }
  return false;
}

void SearchRootMoves(const bool is_eg) {
  auto good      = 0; // Good score in a row for HCE activation
  const auto now = Now();

  for ( ; std::abs(g_best_score) != INF && g_depth < g_max_depth && !g_stop_search; ++g_depth) {
    g_best_score = g_wtm ? BestW() : BestB();
    // Switch to classical only when the game is decided ( 4+ pawns ) !
    g_classical = g_classical || (is_eg && std::abs(g_best_score) > (4 * 100) && ((++good) >= 7));
    SpeakUci(g_best_score, Now() - now);
    g_q_depth = std::min(g_q_depth + 2, MAX_Q_DEPTH);
  }

  g_last_eval = g_best_score;
  if (!g_q_depth) // Nothing searched -> Print smt for UCI
    SpeakUci(g_last_eval, Now() - now);
}

void ThinkReset() { // Reset search status
  g_stop_search = g_nullmove_active = g_is_pv = false;
  g_q_depth = g_best_score = g_nodes = g_depth = 0;
}

void Think(const int ms) {
  g_stop_search_time = Now() + static_cast<std::uint64_t>(ms); // Start clock early
  ThinkReset();
  g_root_n = MgenRoot();
  if (FastMove(ms)) return;

  const auto tmp = g_board;
  const Material m{};
  g_classical = HCEActivation(m);
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
  for (auto i = 0; i < moves_n; ++i)
    g_board = g_boards[ply] + i, nodes += Perft(!wtm, depth - 1, ply + 1);
  return nodes;
}

// UCI

void UciMake(const int root_i) {
  if (!g_wtm) ++g_fullmoves; // Increase fullmoves only after black move
  g_r50_positions[g_board->fifty] = Hash(g_wtm); // Set hash
  g_board_empty                   = g_boards[0][root_i]; // Copy current board
  g_board                         = &g_board_empty; // Set pointer
  g_wtm                           = !g_wtm; // Flip the board
}

void UciMakeMove() {
  const auto move = TokenNth();
  g_root_n = MgenRoot();
  for (auto i = 0; i < g_root_n; ++i)
    if (move == g_boards[0][i].movename()) {
      UciMake(i);
      return;
    }
  throw std::runtime_error("info string Bad move: " + move); // No move found -> Quit
}

void UciTakeSpecialFen() {
  TokenPop(); // pop "fen"
  std::string fen{};
  for ( ; TokenOk() && !TokenPeek("moves"); TokenPop())
    fen += TokenNth() + " ";
  Fen(fen);
}

void UciFen() {
  Token("startpos") ? Fen(STARTPOS) : UciTakeSpecialFen();
}

void UciMoves() {
  for ( ; TokenOk(); TokenPop()) UciMakeMove();
}

void UciPosition() {
  UciFen();
  if (Token("moves")) UciMoves();
}

void UciSetoption() {
  if (TokenPeek("name") && TokenPeek("value", 2)) {
    if (     TokenPeek("UCI_Chess960", 1)) { g_chess960 = TokenPeek("true", 3); }
    else if (TokenPeek("Hash", 1))         { SetHashtable(TokenNumber(3)); }
    else if (TokenPeek("Level", 1))        { g_level = std::clamp(TokenNumber(3), 0, 100); }
    else if (TokenPeek("MoveOverhead", 1)) { g_move_overhead = std::clamp(TokenNumber(3), 0, 10000); }
    else if (TokenPeek("EvalFile", 1))     { SetNNUE(TokenNth(3)); }
    else if (TokenPeek("BookFile", 1))     { SetBook(TokenNth(3)); }
  }
}

void PrintBestMove() {
  std::cout << "bestmove " << (g_root_n <= 0 ? "0000" : g_boards[0][0].movename()) << std::endl;
}

void UciGoInfinite() {
  g_analyzing = true; // Allow: =n / =b / =r / =q
  Think(INF);
  g_analyzing = false;
  PrintBestMove();
}

void UciGoMovetime() {
  Think(std::max(0, TokenNumber()));
  TokenPop();
  PrintBestMove();
}

void UciGoDepth() {
  g_max_depth = std::clamp(TokenNumber(), 1, MAX_DEPTH);
  Think(INF);
  g_max_depth = MAX_DEPTH;
  TokenPop();
  PrintBestMove();
}

// Calculate needed time then think
// Make sure we never lose on time
// Thus small overhead buffer to prevent time losses
void UciGo() {
  auto wtime = 0, btime = 0, winc = 0, binc = 0, mtg = 26;

  for ( ; TokenOk(); TokenPop())
    if (     Token("wtime"))     {wtime = std::max(0, TokenNumber() - g_move_overhead);}
    else if (Token("btime"))     {btime = std::max(0, TokenNumber() - g_move_overhead);}
    else if (Token("winc"))      {winc  = std::max(0, TokenNumber());}
    else if (Token("binc"))      {binc  = std::max(0, TokenNumber());}
    else if (Token("movestogo")) {mtg   = std::max(1, TokenNumber());}
    else if (Token("movetime"))  {UciGoMovetime(); return;}
    else if (Token("infinite"))  {UciGoInfinite(); return;}
    else if (Token("depth"))     {UciGoDepth();    return;}

  g_wtm ? Think(std::min(wtime, wtime / mtg + winc)) :
          Think(std::min(btime, btime / mtg + binc));

  PrintBestMove();
}

void UciUci() {
  std::cout << "id name " << VERSION << "\n" <<
    "id author Toni Helminen" << "\n" <<
    "option name UCI_Chess960 type check default false" << "\n" <<
    "option name Level type spin default 100 min 0 max 100" << "\n" <<
    "option name MoveOverhead type spin default " << MOVEOVERHEAD << " min 0 max 10000" << "\n" <<
    "option name Hash type spin default " << HASH << " min 1 max 1048576" << "\n" <<
    "option name EvalFile type string default " << EVAL_FILE << "\n" <<
    "option name BookFile type string default " << BOOK_FILE << "\n" <<
    "uciok" << std::endl;
}

// Save state (just in case) if multiple commands in a row
struct Save {
  const bool nnue, book;
  const std::string fen;

  // Save stuff in constructor
  Save() : nnue{g_nnue_exist}, book{g_book_exist}, fen{g_board->to_fen()} { }

  ~Save() { // Restore stuff in destructor
    g_nnue_exist = this->nnue;
    g_book_exist = this->book;
    Fen(this->fen);
  }
};

// Print ASCII art board
// > p [fen = startpos]
// > p 2R5/2R4p/5p1k/6n1/8/1P2QPPq/r7/6K1_w_-_-_0_1
// Print board + some info (NNUE, Book, Eval (cp), Hash (entries))
// Also used for debug in UCI mode
void UciPrintBoard(std::string s = "") {
  const Save save{};
  if (s.length()) std::replace(s.begin(), s.end(), '_', ' '), Fen(s);
  std::cout << "\n" << g_board->to_s() << std::endl;
}

// Calculate perft split numbers
// > perft [depth = 6] [fen = startpos]
// > perft -> 119060324
// > perft 7 R7/P4k2/8/8/8/8/r7/6K1_w_-_-_0_1 -> 245764549
void UciPerft(const std::string &d, const std::string &f) {
  const Save save{};
  const auto depth = d.length() ? std::max(0, std::stoi(d)) : 6;
  std::string fen  = f.length() ? f : STARTPOS;
  std::replace(fen.begin(), fen.end(), '_', ' '); // Hack !
  std::uint64_t nodes = depth >= 1 ? 0 : 1, total_ms = 0;
  Fen(fen);
  g_root_n = MgenRoot();
  for (auto i = 0; i < g_root_n; ++i) {
    g_board           = g_boards[0] + i;
    const auto now    = Now();
    const auto nodes2 = depth >= 0 ? Perft(!g_wtm, depth - 1, 1) : 0;
    const auto ms     = Now() - now;
    std::cout << (i + 1) << ". " << g_boards[0][i].movename() << " -> " << nodes2 << " (" << ms << " ms)" << std::endl;
    nodes    += nodes2;
    total_ms += ms;
  }
  std::cout << "\n===========================\n\n" <<
    "Nodes:    " << nodes << "\n" <<
    "Time(ms): " << total_ms << "\n" <<
    "NPS:      " << Nps(nodes, total_ms) << std::endl;
}

// Bench signature and speed of the program
// > bench [depth = 11] [time = inf] [hash = 256] [nnue = 1]
// Speed:     bench inf 5000
// Signature: bench 11 inf
// > bench              -> 15932209
// > bench 11 inf 256 0 -> 15157798
void UciBench(const std::string &d, const std::string &t, const std::string &h, const std::string &nnue) {
  const Save save{};
  SetHashtable(h.length() ? std::stoi(h) : 256); // Set hash and reset
  g_max_depth         = !d.length() ? 11 : (d == "inf" ? MAX_DEPTH : std::clamp(std::stoi(d), 0, MAX_DEPTH)); // Set depth limits
  g_noise             = 0; // Make search deterministic
  g_book_exist        = false; // Disable book
  g_nnue_exist        = g_nnue_exist && nnue != "0"; // Use nnue ?
  std::uint64_t nodes = 0;
  const auto time     = !t.length() || t == "inf" ? INF : std::max(0, std::stoi(t)); // Set time limits
  auto n = 0, total_ms = 0;
  for (const auto &fen : kBench) {
    std::cout << "[ " << (++n) << "/" << kBench.size() << " ; "  << fen << " ]" << std::endl;
    Fen(fen);
    const auto now = Now();
    Think(time);
    total_ms += Now() - now;
    nodes    += g_nodes;
    std::cout << std::endl;
  }
  g_noise     = NOISE;
  g_max_depth = MAX_DEPTH;
  std::cout << "===========================\n\n" <<
    "Nodes:    " << nodes << "\n" <<
    "Time(ms): " << total_ms << "\n" <<
    "NPS:      " << Nps(nodes, total_ms) << "\n" <<
    "Mode:     " << (g_nnue_exist ? "NNUE" : "HCE") << std::endl;
}

bool UciCommands() {
  if (!TokenOk()) return true;

  if (     Token("position"))   UciPosition();
  else if (Token("go"))         UciGo();
  else if (Token("ucinewgame")) g_last_eval = 0;
  else if (Token("isready"))    std::cout << "readyok" << std::endl;
  else if (Token("setoption"))  UciSetoption();
  else if (Token("uci"))        UciUci();
  else if (Token("quit"))       return false;
  // Extra ...
  else if (Token("bench"))      UciBench(TokenNth(0), TokenNth(1), TokenNth(2), TokenNth(3));
  else if (Token("perft"))      UciPerft(TokenNth(0), TokenNth(1));
  else if (Token("p"))          UciPrintBoard(TokenNth(0));

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

  for (auto i = 0; i < 64; ++i)
    if (moves & Bit(i))
      good[total++] = i; // post inc

  const auto popn = PopCount(moves);
  for (auto i = 0; i < popn; ++i)
    if ((0x1 << i) & index)
      permutations |= Bit(good[i]);

  return permutations & moves;
}

std::uint64_t MakeSliderMagicMoves(const std::array<int, 8> &slider_vectors, const int sq, const std::uint64_t moves) {
  std::uint64_t possible_moves = 0;
  const auto x_pos = Xaxl(sq), y_pos = Yaxl(sq);
  for (auto i = 0; i < 4; ++i)
    for (auto j = 1; j < 8; ++j) {
      const auto x = x_pos + j * slider_vectors[2 * i], y = y_pos + j * slider_vectors[2 * i + 1];
      if (!OnBoard(x, y)) break;
      const auto tmp  = Bit(8 * y + x);
      possible_moves |= tmp;
      if (tmp & moves) break;
    }
  return possible_moves & (~Bit(sq));
}

void InitBishopMagics() {
  constexpr std::array<int, 8> bishop_vectors = {+1, +1, -1, -1, +1, -1, -1, +1};
  for (auto i = 0; i < 64; ++i) {
    const auto magics = kBishopMagics[2][i] & (~Bit(i));
    for (auto j = 0; j < 512; ++j) {
      const auto allmoves = PermutateBb(magics, j);
      g_bishop_magic_moves[i][BishopMagicIndex(i, allmoves)] = MakeSliderMagicMoves(bishop_vectors, i, allmoves);
    }
  }
}

void InitRookMagics() {
  constexpr std::array<int, 8> rook_vectors = {+1,  0,  0, +1,  0, -1, -1,  0};
  for (auto i = 0; i < 64; ++i) {
    const auto magics = kRookMagics[2][i] & (~Bit(i));
    for (auto j = 0; j < 4096; ++j) {
      const auto allmoves = PermutateBb(magics, j);
      g_rook_magic_moves[i][RookMagicIndex(i, allmoves)] = MakeSliderMagicMoves(rook_vectors, i, allmoves);
    }
  }
}

std::uint64_t MakeSliderMoves(const int sq, const std::array<int, 8> &slider_vectors) {
  std::uint64_t moves = 0;
  const auto x_pos = Xaxl(sq), y_pos = Yaxl(sq);
  for (auto i = 0; i < 4; ++i) {
    const auto dx = slider_vectors[2 * i], dy = slider_vectors[2 * i + 1];
    std::uint64_t tmp = 0;
    for (auto j = 1; j < 8; ++j)
      if (const auto x = x_pos + j * dx, y = y_pos + j * dy; OnBoard(x, y)) tmp |= Bit(8 * y + x);
      else                                                                  break;
    moves |= tmp;
  }
  return moves;
}

template <int k>
std::uint64_t MakeJumpMoves(const int sq, const int dy, const std::array<int, k> &jump_vectors) {
  std::uint64_t moves = 0;
  const auto x_pos = Xaxl(sq), y_pos = Yaxl(sq);
  for (std::size_t i = 0; i < jump_vectors.size() / 2; ++i)
    if (const auto x = x_pos + jump_vectors[2 * i], y = y_pos + dy * jump_vectors[2 * i + 1]; OnBoard(x, y))
      moves |= Bit(8 * y + x);
  return moves;
}

void InitJumpMoves() {
  constexpr std::array<int, 16> king_vectors      = {+1,  0,  0, +1,  0, -1, -1,  0, +1, +1, -1, -1, +1, -1, -1, +1};
  constexpr std::array<int, 16> knight_vectors    = {+2, +1, -2, +1, +2, -1, -2, -1, +1, +2, -1, +2, +1, -2, -1, -2};
  constexpr std::array<int, 4> pawn_check_vectors = {-1, +1, +1, +1};
  constexpr std::array<int, 2> pawn_1_vectors     = { 0, +1};

  for (auto i = 0; i < 64; ++i) {
    g_king_moves[i]     = MakeJumpMoves<16>(i, +1, king_vectors);
    g_knight_moves[i]   = MakeJumpMoves<16>(i, +1, knight_vectors);
    g_pawn_checks_w[i]  = MakeJumpMoves<4>(i, +1, pawn_check_vectors);
    g_pawn_checks_b[i]  = MakeJumpMoves<4>(i, -1, pawn_check_vectors);
    g_pawn_1_moves_w[i] = MakeJumpMoves<2>(i, +1, pawn_1_vectors);
    g_pawn_1_moves_b[i] = MakeJumpMoves<2>(i, -1, pawn_1_vectors);
  }

  for (auto i = 0; i < 8; ++i) {
    g_pawn_2_moves_w[ 8 + i] = MakeJumpMoves<2>( 8 + i, +1, pawn_1_vectors) | MakeJumpMoves<2>( 8 + i, +2, pawn_1_vectors);
    g_pawn_2_moves_b[48 + i] = MakeJumpMoves<2>(48 + i, -1, pawn_1_vectors) | MakeJumpMoves<2>(48 + i, -2, pawn_1_vectors);
  }
}

void InitZobrist() {
  for (auto i = 0; i < 13; ++i) for (auto j = 0; j < 64; ++j) g_zobrist_board[i][j] = Random8x64();
  for (auto i = 0; i < 64; ++i) g_zobrist_ep[i]     = Random8x64();
  for (auto i = 0; i < 16; ++i) g_zobrist_castle[i] = Random8x64();
  for (auto i = 0; i <  2; ++i) g_zobrist_wtm[i]    = Random8x64();
}

// Shuffle period 30 plies then scale
void InitScale() {
  for (auto i = 0; i < MAX_ARR; ++i)
    g_scale[i] = i < 30 ? 1.0f : (1.0f - ((static_cast<float>(i - 30)) / 110.0f));
}

void InitLMR() {
  for (auto d = 0; d < MAX_DEPTH; ++d)
    for (auto m = 0; m < MAX_MOVES; ++m)
      g_lmr[d][m] = std::clamp<int>(0.25 * std::log(d) * std::log(m), 1, 6);
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
  InitScale();
  InitLMR();
  SetHashtable(HASH);
  SetNNUE(EVAL_FILE);
  SetBook(BOOK_FILE);
  Fen(STARTPOS);
}

void UciLoop() {
  while (Uci());
}

} // namespace mayhem
