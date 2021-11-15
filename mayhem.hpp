/*
Mayhem. Linux UCI Chess960 engine. Written in C++17 language
Copyright (C) 2020-2021 Toni Helminen

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
#include <cmath>
#include <memory>
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <ctime>
#include <array>
#include <string>
#include <chrono>

extern "C" {
#include <unistd.h>
#ifdef WINDOWS
#include <conio.h>
#endif
}

namespace nnue { // No clashes
#include "lib/nnue.hpp"
}
#include "lib/eucalyptus.hpp"
#include "lib/polyglotbook.hpp"
#include "lib/poseidon.hpp"

// Namespace

namespace mayhem {

// Macros

#define VERSION       "Mayhem 6.2"
#define STARTPOS      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0"
#define MAX_MOVES     256     // Max chess moves
#define MAX_DEPTH     64      // Max search depth (stack frame problems ...)
#define MAX_Q_DEPTH   12      // Max Qsearch depth
#define BOOK_MS       100     // At least 100ms+ for the book lookup
#define INF           1048576 // System max number
#define MAX_POS       101     // Rule 50 + 1 ply for arrays
#define HASH          256     // MB
#define MOVEOVERHEAD  100     // ms
#define BOOK_BEST     false   // Nondeterministic opening play
#define MAX_PIECES    32      // Max pieces on board (32 for Standard ...)
#define EVAL_FILE     "nn-cb80fb9393af.nnue" // "x" to disable NNUE
#define BOOK_FILE     "performance.bin"      // "x" to disable book

// Constants

// Tactical fens to pressure search
const std::array<const std::string, 15> kBench{
  "R7/P4k2/8/8/8/8/r7/6K1 w - - 0 ; 1/15 ; Rh8",
  "2kr3r/pp1q1ppp/5n2/1Nb5/2Pp1B2/7Q/P4PPP/1R3RK1 w - - 0 ; 2/15 ; Nxa7+",
  "2R5/2R4p/5p1k/6n1/8/1P2QPPq/r7/6K1 w - - 0 ; 3/15 ; Rxh7+",
  "5r1k/1b4p1/p6p/4Pp1q/2pNnP2/7N/PPQ3PP/5R1K b - - 0 ; 4/15 ; Qxh3",
  "6k1/3r4/2R5/P5P1/1P4p1/8/4rB2/6K1 b - - 0 ; 5/15 ; g3",
  "5n2/pRrk2p1/P4p1p/4p3/3N4/5P2/6PP/6K1 w - - 0 ; 6/15 ; Nb5",
  "8/6pp/4p3/1p1n4/1NbkN1P1/P4P1P/1PR3K1/r7 w - - 0 ; 7/15 ; Rxc4+",
  "2r5/2rk2pp/1pn1pb2/pN1p4/P2P4/1N2B3/nPR1KPPP/3R4 b - - 0 ; 8/15 ; Nxd4+",
  "nrq4r/2k1p3/1p1pPnp1/pRpP1p2/P1P2P2/2P1BB2/1R2Q1P1/6K1 w - - 0 ; 9/15 ; Bxc5",
  "3r2k1/5p2/6p1/4b3/1P2P3/1R2P2p/P1K1N3/8 b - - 0 ; 10/15 ; Rd1",
  "1k1r4/pp1r1pp1/4n1p1/2R5/2Pp1qP1/3P2QP/P4PB1/1R4K1 w - - 0 ; 11/15 ; Bxb7",
  "2r1k3/6pr/p1nBP3/1p3p1p/2q5/2P5/P1R4P/K2Q2R1 w - - 0 ; 12/15 ; Rxg7",
  "2b4k/p1b2p2/2p2q2/3p1PNp/3P2R1/3B4/P1Q2PKP/4r3 w - - 0 ; 13/15 ; Qxc6",
  "5bk1/1rQ4p/5pp1/2pP4/3n1PP1/7P/1q3BB1/4R1K1 w - - 0 ; 14/15 ; d6",
  "rnbqkb1r/pppp1ppp/8/4P3/6n1/7P/PPPNPPP1/R1BQKBNR b KQkq - 0 ; 15/15 ; Ne3"
};

// [Attacker][Captured] / [PNBRQK][pnbrqk]
constexpr int kMvv[6][6]{
  {10, 15, 15, 20, 25, 99}, {9, 14, 14, 19, 24, 99}, {9, 14, 14, 19, 24, 99},
  {8,  13, 13, 18, 23, 99}, {7, 12, 12, 17, 22, 99}, {6, 11, 11, 16, 21, 99}
};

// Material baked in
// MG / EG -> P / N / B / R / Q / K
constexpr int kPesto[6][2][64]{
  {{82,   82,   82,   82,   82,   82,   82,   82,   47,   81,   62,   59,   67,   106,  120,  60,
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
    -12,  17,   14,   17,   17,   38,   23,   11,   -74,  -35,  -18,  -18,  -11,  15,   4,    -17}}
};

constexpr std::uint64_t kRookMagic[3][64]{
  { // Magics
    0x548001400080106cULL, 0x900184000110820ULL,  0x428004200a81080ULL,  0x140088082000c40ULL,
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
    0x280120020049902ULL,  0x1200412602009402ULL, 0x914900048020884ULL,  0x104824281002402ULL
  },
  { // Mask
    0x101010101017eULL,    0x202020202027cULL,    0x404040404047aULL,    0x8080808080876ULL,
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
    0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL
  },
  { // Moves
    0x101010101017eULL,    0x202020202027cULL,    0x404040404047aULL,    0x8080808080876ULL,
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
    0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL
  }
};

constexpr std::uint64_t kBishopMagic[3][64]{
  { // Magics
    0x2890208600480830ULL, 0x324148050f087ULL,    0x1402488a86402004ULL, 0xc2210a1100044bULL,
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
    0x180450800222a011ULL, 0x600014600490202ULL,  0x21040100d903ULL,     0x10404821000420ULL
  },
  { // Mask
    0x40201008040200ULL,   0x402010080400ULL,     0x4020100a00ULL,       0x40221400ULL,
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
    0x28440200000000ULL,   0x50080402000000ULL,   0x20100804020000ULL,   0x40201008040200ULL
  },
  { // Moves
    0x40201008040200ULL,   0x402010080400ULL,     0x4020100a00ULL,       0x40221400ULL,
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
    0x28440200000000ULL,   0x50080402000000ULL,   0x20100804020000ULL,   0x40201008040200ULL
  }
};

// Structs

struct Board {
  std::uint64_t
    white[6],   // White bitboards
    black[6];   // Black bitboards
  std::int32_t
    score;      // Sorting score
  std::int8_t
    pieces[64], // Pieces white and black
    epsq;       // En passant square
  std::uint8_t
    index,      // Sorting index
    from,       // From square
    to,         // To square
    type,       // Move type (0: Normal, 1: OOw, 2: OOOw, 3: OOb, 4: OOOb,
                //            5: =n, 6: =b, 7: =r, 8: =q)
    castle,     // Castling rights (0x1: K, 0x2: Q, 0x4: k, 0x8: q)
    fifty;      // Rule 50 counter
};

struct HashEntry {
  // Hashes for eval and sort
  std::uint64_t eval_hash{0x0ULL}, sort_hash{0x0ULL};
  // Score for NNUE only
  std::int32_t score{0};
  // Indexes for sorting
  std::uint8_t killer{0}, good{0}, quiet{0};
};

// Enums

enum class MoveType { kKiller, kGood, kQuiet };

// Variables

std::uint64_t g_black{0x0ULL}, g_white{0x0ULL}, g_both{0x0ULL}, g_empty{0x0ULL}, g_pawn_sq{0x0ULL},
  g_pawn_1_moves_w[64]{}, g_pawn_1_moves_b[64]{}, g_pawn_2_moves_w[64]{},
  g_pawn_2_moves_b[64]{}, g_bishop_moves[64]{}, g_rook_moves[64]{}, g_queen_moves[64]{},
  g_knight_moves[64]{}, g_king_moves[64]{}, g_pawn_checks_w[64]{}, g_pawn_checks_b[64]{},
  g_castle_w[2]{}, g_castle_b[2]{}, g_castle_empty_w[2]{}, g_castle_empty_b[2]{},
  g_bishop_magic_moves[64][512]{}, g_rook_magic_moves[64][4096]{}, g_zobrist_ep[64]{},
  g_zobrist_castle[16]{}, g_zobrist_wtm[2]{}, g_zobrist_board[13][64]{},
  g_stop_search_time{0x0ULL}, g_r50_positions[MAX_POS]{}, g_nodes{0x0ULL}, g_good{0x0ULL};

int g_move_overhead{MOVEOVERHEAD}, g_rook_w[2]{}, g_rook_b[2]{}, g_root_n{0},
  g_king_w{0}, g_king_b{0}, g_moves_n{0}, g_max_depth{MAX_DEPTH}, g_q_depth{0},
  g_depth{0}, g_best_score{0}, g_last_eval{0}, g_lmr[MAX_DEPTH][MAX_MOVES]{},
  g_noise{2}, g_nnue_pieces[64]{}, g_nnue_squares[64]{}, g_level{100};

bool g_chess960{false}, g_wtm{false}, g_underpromos{true}, g_nullmove_active{false},
  g_stop_search{false}, g_is_pv{false}, g_book_exist{false}, g_nnue_exist{false},
  g_classical{false}, g_game_on{true}, g_frc_problems{false};

Board g_board_tmp{}, *g_board{&g_board_tmp}, *g_moves{nullptr}, *g_board_orig{nullptr},
  g_boards[MAX_DEPTH + MAX_Q_DEPTH + 4][MAX_MOVES]{};

std::uint32_t g_hash_entries{0}, g_tokens_nth{0};

std::vector<std::string> g_tokens{};

polyglotbook::PolyglotBook g_book{};

std::unique_ptr<HashEntry[]> g_hash{};

float g_scale[MAX_POS]{};

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

inline std::uint64_t White() {
  return g_board->white[0] | g_board->white[1] | g_board->white[2] |
         g_board->white[3] | g_board->white[4] | g_board->white[5];
}

inline std::uint64_t Black() {
  return g_board->black[0] | g_board->black[1] | g_board->black[2] |
         g_board->black[3] | g_board->black[4] | g_board->black[5];
}

inline std::uint64_t Both() {
  return White() | Black();
}

inline int Ctz(const std::uint64_t bb) {
  return __builtin_ctzll(bb);
}

// Count (return) zeros AND then pop (arg) BitBoard
inline int CtzPop(std::uint64_t *bb) {
  const auto ret{Ctz(*bb)};
  *bb = *bb & (*bb - 0x1ULL);
  return ret;
}

inline int PopCount(const std::uint64_t bb) {
  return __builtin_popcountll(bb);
}

inline int Xcoord(const int sq) {
  return sq & 0x7;
}

inline int Ycoord(const int sq) {
  return sq >> 3;
}

inline std::uint64_t Bit(const int nbits) {
  return 0x1ULL << nbits;
}

std::uint64_t Nps(const std::uint64_t nodes, const std::uint64_t ms) {
  return ms ? (1000 * nodes) / ms : 0;
}

bool OnBoard(const int x, const int y) { // Slow, but only for init
  return x >= 0 && x <= 7 && y >= 0 && y <= 7;
}

inline bool IsUnderpromo(const Board *b) {
  return b->type >= 5 && b->type <= 7;
}

void Ok(const bool test, const std::string &msg) {
  if (!test) {
    std::cout << "info string " << msg << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

extern "C" {

#ifdef WINDOWS
bool InputAvailable() {
  return _kbhit();
}
#else
bool InputAvailable() {
  fd_set fd;
  struct timeval tv;
  FD_ZERO(&fd);
  FD_SET(STDIN_FILENO, &fd);
  tv.tv_sec = tv.tv_usec = 0;
  select(STDIN_FILENO + 1, &fd, nullptr, nullptr, &tv);
  return FD_ISSET(STDIN_FILENO, &fd) > 0;
}
#endif

} // extern "C"

inline std::uint64_t Now() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::system_clock::now().time_since_epoch())
           .count();
}

std::uint64_t Random64() {
  static std::uint64_t a{0X12311227ULL}, b{0X1931311ULL}, c{0X13138141ULL};
#define MIXER(num) (((num) << 7) ^ ((num) >> 5))
  a ^= b + c;
  b ^= b * c + 0x1717711ULL;
  c  = (3 * c) + 0x1ULL;
  return MIXER(a) ^ MIXER(b) ^ MIXER(c);
}

std::uint64_t Random8x64() {
  std::uint64_t ret{0x0ULL};
  for (auto i{0}; i < 8; ++i)
    ret ^= Random64() << (8 * i);
  return ret;
}

int Random(const int min, const int max) {
  static std::uint64_t seed{0x202c7ULL + static_cast<std::uint64_t>(std::time(nullptr))};
  return min +
    (seed = (seed << 5) ^ (seed + 1) ^ (seed >> 3)) %
      static_cast<std::uint64_t>(std::max(1, max - min + 1));
}

template <class T>
void SplitString(const std::string &str, T &cont, const std::string &delims = " ") {
  std::size_t cur{str.find_first_of(delims)}, prev{0};
  while (cur != std::string::npos) {
    cont.push_back(str.substr(prev, cur - prev));
    prev = cur + 1;
    cur  = str.find_first_of(delims, prev);
  }
  cont.push_back(str.substr(prev, cur - prev));
}

void ReadInput() {
  std::string line;
  std::getline(std::cin, line);
  g_tokens_nth = 0;
  g_tokens.clear();
  SplitString<std::vector<std::string>>(line, g_tokens);
}

// PolyGlot Book lib

void SetBook(const std::string &book_file, const bool print = false) {
  g_book_exist = book_file.length() <= 1 ? false : g_book.open_book(book_file);
  if (print)
    std::cout << "info string Book " << (g_book_exist ? "enabled" : "disabled") << std::endl;
}

// NNUE lib

void SetNNUE(const std::string &eval_file, const bool print = false) {
  g_nnue_exist = eval_file.length() <= 1 ? false : nnue::nnue_init(eval_file.c_str());
  if (print)
    std::cout << "info string NNUE " << (g_nnue_exist ? "enabled" : "disabled") << std::endl;
}

// Hashtable

void SetHashtable(int hash_mb) {
  // Limits 4MB -> 512GB
  hash_mb = std::clamp(hash_mb, 4, 1048576);
  // Hash in B / block in B
  g_hash_entries = static_cast<std::uint32_t>((1 << 20) * hash_mb) / (sizeof(HashEntry));
  // Claim space
  g_hash.reset(new HashEntry[g_hash_entries]);
}

// Hash

inline std::uint64_t Hash(const bool wtm) {
  auto ret{g_zobrist_ep[g_board->epsq + 1] ^
           g_zobrist_wtm[int(wtm)]         ^
           g_zobrist_castle[g_board->castle]};

  for (auto both{Both()}; both; ) {
    const auto sq{CtzPop(&both)};
    ret ^= g_zobrist_board[g_board->pieces[sq] + 6][sq];
  }

  return ret;
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

// Board

void BuildBitboards() {
  for (auto i{0}; i < 64; ++i)
    if      (g_board->pieces[i] > 0) g_board->white[+g_board->pieces[i] - 1] |= Bit(i);
    else if (g_board->pieces[i] < 0) g_board->black[-g_board->pieces[i] - 1] |= Bit(i);
}

template<int to>
std::uint64_t Fill(int from) {
  // static_assert(to >= 0 && to <= 63, "Bad to square");
  if (from < 0 || from > 63)
    return 0x0ULL;

  auto ret{Bit(from)};
  if (from == to)
    return ret;

  const auto diff{from > to ? -1 : +1};
  do {
    from += diff;
    ret  |= Bit(from);
  } while (from != to);

  return ret;
}

void BuildCastlingBitboard1W() {
  // White: O-O
  if (g_board->castle & 0x1) {
    g_castle_w[0]       = Fill<6>(g_king_w);
    g_castle_empty_w[0] = (g_castle_w[0] | Fill<5>(g_rook_w[0])) ^
                            (Bit(g_king_w) | Bit(g_rook_w[0]));
  }

  // White: O-O-O
  if (g_board->castle & 0x2) {
    g_castle_w[1]       = Fill<2>(g_king_w);
    g_castle_empty_w[1] = (g_castle_w[1] | Fill<3>(g_rook_w[1])) ^
                            (Bit(g_king_w) | Bit(g_rook_w[1]));
  }
}

void BuildCastlingBitboard1B() {
  // Black: O-O
  if (g_board->castle & 0x4) {
    g_castle_b[0]       = Fill<56 + 6>(g_king_b);
    g_castle_empty_b[0] = (g_castle_b[0] | Fill<56 + 5>(g_rook_b[0])) ^
                            (Bit(g_king_b) | Bit(g_rook_b[0]));
  }

  // Black: O-O-O
  if (g_board->castle & 0x8) {
    g_castle_b[1]       = Fill<56 + 2>(g_king_b);
    g_castle_empty_b[1] = (g_castle_b[1] | Fill<56 + 3>(g_rook_b[1])) ^
                            (Bit(g_king_b) | Bit(g_rook_b[1]));
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

// Fen handling

void PutPiece(const int sq, const int p) {
  // Find kings too
  if      (p == +6) g_king_w = sq; // K
  else if (p == -6) g_king_b = sq; // k
  g_board->pieces[sq] = p;
}

int Piece2Num(const char p) {
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
    default:  return -6; // k
  }
}

int Empty2Num(const char e) {
  return e - '0';
}

int File2Num(const char f) {
  return f - 'a';
}

int Rank2Num(const char r) {
  return r == '3' ? 2 : 5; // '3' / '6'
}

void FenBoard(const std::string &board) {
  auto sq{56};
  for (std::size_t i{0}; i < board.length() /* O(n) */ && sq >= 0; ++i)
    if (const auto c{board[i]}; c == '/') sq -= 16;
    else if (std::isdigit(c))             sq += Empty2Num(c);
    else                                  PutPiece(sq++, Piece2Num(c));
}

void FenAddCastle(int *rooks, const int sq, const int castle) {
  *rooks           = sq;
  g_board->castle |= castle;
}

void FenAddChess960Castling(const char file) {
  if (file >= 'A' && file <= 'H') {
    if (const auto tmp{file - 'A'}; tmp > g_king_w)
      FenAddCastle(g_rook_w + 0, tmp, 0x1);
    else if (tmp < g_king_w)
      FenAddCastle(g_rook_w + 1, tmp, 0x2);
  } else if (file >= 'a' && file <= 'h') {
    if (const auto tmp{(file - 'a') + 56}; tmp > g_king_b)
      FenAddCastle(g_rook_b + 0, tmp, 0x4);
    else if (tmp < g_king_b)
      FenAddCastle(g_rook_b + 1, tmp, 0x8);
  }
}

void FenKQkq(const std::string &KQkq) {
  for (std::size_t i{0}; i < KQkq.length(); ++i)
    switch (const auto f{KQkq[i]}) {
      case 'K': FenAddCastle(g_rook_w + 0, 7,      0x1); break;
      case 'Q': FenAddCastle(g_rook_w + 1, 0,      0x2); break;
      case 'k': FenAddCastle(g_rook_b + 0, 56 + 7, 0x4); break;
      case 'q': FenAddCastle(g_rook_b + 1, 56 + 0, 0x8); break;
      default:  FenAddChess960Castling(f);               break;
    }
}

void FenEp(const std::string &ep) {
  if (ep.length() == 2)
    g_board->epsq = 8 * Rank2Num(ep[1]) + File2Num(ep[0]);
}

void FenRule50(const std::string &fifty) {
  if (fifty.length() != 0 && fifty[0] != '-')
    g_board->fifty = std::clamp(std::stoi(fifty), 0, 100);
}

void FenGen(const std::string &fen) {
  std::vector<std::string> tokens{};

  SplitString<std::vector<std::string>>(fen, tokens);
  Ok(fen.length() >= 10 && tokens.size() >= 5, "Bad fen #1");

  FenBoard(tokens[0]);
  g_wtm = tokens[1][0] == 'w';
  FenKQkq(tokens[2]);
  BuildCastlingBitboards();
  FenEp(tokens[3]);
  FenRule50(tokens[4]);
}

// Reset board
void FenReset() {
  g_board_tmp   = {};
  g_board       = &g_board_tmp;
  g_wtm         = true;
  g_board->epsq = -1;
  g_king_w = g_king_b = 0;

  for (const auto i : {0, 1}) {
    g_castle_w[i] = g_castle_empty_w[i] = g_castle_b[i] = g_castle_empty_b[i] = 0x0ULL;
    g_rook_w[i] = g_rook_b[i] = 0;
  }

  for (auto i{0}; i < 6; ++i)
    g_board->white[i] = g_board->black[i] = 0x0ULL;
}

// Not perfect. Just avoid obvious crashes
bool BoardIsGood() {
         // 1 king / side
  return (  PopCount(g_board->white[5]) == 1 &&
            PopCount(g_board->black[5]) == 1) &&
         // No illegal checks
         (!(g_wtm ? ChecksW() : ChecksB()));
}

void Fen(const std::string &fen) {
  FenReset();
  FenGen(fen);
  BuildBitboards();
  Ok(BoardIsGood(), "Bad fen #2");
}

// Checks

inline bool ChecksHereW(const int sq) {
  const auto both{Both()};
  return (g_pawn_checks_b[sq]        &  g_board->white[0])                      |
         (g_knight_moves[sq]         &  g_board->white[1])                      |
         (BishopMagicMoves(sq, both) & (g_board->white[2] | g_board->white[4])) |
         (RookMagicMoves(sq, both)   & (g_board->white[3] | g_board->white[4])) |
         (g_king_moves[sq]           &  g_board->white[5]);
}

inline bool ChecksHereB(const int sq) {
  const auto both{Both()};
  return (g_pawn_checks_w[sq]        &  g_board->black[0])                      |
         (g_knight_moves[sq]         &  g_board->black[1])                      |
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

// Move printing

char File2Char(const int f) {
  return 'a' + f;
}

char Rank2Char(const int r) {
  return '1' + r;
}

const std::string MoveStr(const int from, const int to) {
  return std::string{File2Char(Xcoord(from)), Rank2Char(Ycoord(from)),
                     File2Char(Xcoord(to)),   Rank2Char(Ycoord(to))};
}

char PromoLetter(const std::int8_t piece) {
  return "nbrq"[std::abs(piece) - 2];
}

const std::string MoveName(const Board *move) {
  auto from{move->from}, to{move->to};

  switch (move->type) {
    case 1: from = g_king_w; to = g_chess960 ? g_rook_w[0] : 6;      break;
    case 2: from = g_king_w; to = g_chess960 ? g_rook_w[1] : 2;      break;
    case 3: from = g_king_b; to = g_chess960 ? g_rook_b[0] : 56 + 6; break;
    case 4: from = g_king_b; to = g_chess960 ? g_rook_b[1] : 56 + 2; break;
    case 5: case 6: case 7: case 8:
            return MoveStr(from, to) + PromoLetter(move->pieces[to]);
  }

  return MoveStr(from, to);
}

// Sorting

template<bool noisy> // Tiny speedup (avoid checks or =q etc)
void SortNthMoves(const int nth) {
  for (auto i{0}; i < nth; ++i) {
    for (auto j{i + 1}; j < g_moves_n; ++j)
      if (g_moves[j].score > g_moves[i].score)
        std::swap(g_moves[j], g_moves[i]);

    // Can't sort since no scores -> quit
    if constexpr (noisy)
      if (!g_moves[i].score)
        return;
  }
}

int EvaluateMoves() {
  auto tactics{0}; // int

  for (auto i{0}; i < g_moves_n; ++i) {
    if (g_moves[i].score)
      ++tactics;
    g_moves[i].index = i;
  }

  return tactics;
}

// Best moves put first for maximum cutoffs
void SortByScore(const HashEntry *entry, const std::uint64_t hash) {
  if (entry->sort_hash == hash) {
    if (entry->killer) g_moves[entry->killer - 1].score += 10000;
    if (entry->good)   g_moves[entry->good   - 1].score += 7000;
    if (entry->quiet)  g_moves[entry->quiet  - 1].score += 3000;
  }
  SortNthMoves<false>(EvaluateMoves());
}

void EvalRootMoves() {
  auto *tmp{g_board}; // Have to call eval()

  for (auto i{0}; i < g_root_n; ++i) {
    g_board         = g_boards[0] + i;
                      // =q
    g_board->score += (g_board->type == 8 ? 1000 : 0) +
                      // OO|OOO
                      (g_board->type >= 1 && g_board->type <= 4 ? 100 : 0) +
                      // =rbn
                      (IsUnderpromo(g_board) ? -5000 : 0) +
                      // Make some noise !!!
                      (g_noise ? Random(-g_noise, +g_noise) : 0) +
                      // Full eval
                      (g_wtm ? +1 : -1) * Evaluate(g_wtm);
  }

  g_board = tmp;
}

void SortRoot(const int index) {
  if (index) {
    const auto tmp{g_boards[0][index]};
    for (auto i{index}; i > 0; --i)
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
  return ((mask & kBishopMagic[1][sq]) * kBishopMagic[0][sq]) >> 55;
}

inline std::uint64_t RookMagicIndex(const int sq, const std::uint64_t mask) {
  return ((mask & kRookMagic[1][sq]) * kRookMagic[0][sq]) >> 52;
}

inline std::uint64_t BishopMagicMoves(const int sq, const std::uint64_t mask) {
  return g_bishop_magic_moves[sq][BishopMagicIndex(sq, mask)];
}

inline std::uint64_t RookMagicMoves(const int sq, const std::uint64_t mask) {
  return g_rook_magic_moves[sq][RookMagicIndex(sq, mask)];
}

void HandleCastlingW(const int mtype, const int from, const int to) {
  g_moves[g_moves_n] = *g_board; // copy
  g_board            = &g_moves[g_moves_n]; // pointer
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
  if (ChecksCastleB(g_castle_w[0]))
    return;

  HandleCastlingW(1, g_king_w, 6);

  g_board->pieces[g_rook_w[0]] = 0;
  g_board->pieces[g_king_w]    = 0;
  g_board->pieces[5]           = +4;
  g_board->pieces[6]           = +6;
  g_board->white[3]            = (g_board->white[3] ^ Bit(g_rook_w[0])) | Bit(5);
  g_board->white[5]            = (g_board->white[5] ^ Bit(g_king_w))    | Bit(6);

  if (!ChecksB())
    ++g_moves_n;
}

void AddCastleOOB() {
  if (ChecksCastleW(g_castle_b[0]))
    return;

  HandleCastlingB(3, g_king_b, 56 + 6);

  g_board->pieces[g_rook_b[0]] = 0;
  g_board->pieces[g_king_b]    = 0;
  g_board->pieces[56 + 5]      = -4;
  g_board->pieces[56 + 6]      = -6;
  g_board->black[3]            = (g_board->black[3] ^ Bit(g_rook_b[0])) | Bit(56 + 5);
  g_board->black[5]            = (g_board->black[5] ^ Bit(g_king_b))    | Bit(56 + 6);

  if (!ChecksW())
    ++g_moves_n;
}

void AddCastleOOOW() {
  if (ChecksCastleB(g_castle_w[1]))
    return;

  HandleCastlingW(2, g_king_w, 2);

  g_board->pieces[g_rook_w[1]] = 0;
  g_board->pieces[g_king_w]    = 0;
  g_board->pieces[3]           = +4;
  g_board->pieces[2]           = +6;
  g_board->white[3]            = (g_board->white[3] ^ Bit(g_rook_w[1])) | Bit(3);
  g_board->white[5]            = (g_board->white[5] ^ Bit(g_king_w))    | Bit(2);

  if (!ChecksB())
    ++g_moves_n;
}

void AddCastleOOOB() {
  if (ChecksCastleW(g_castle_b[1]))
    return;

  HandleCastlingB(4, g_king_b, 56 + 2);

  g_board->pieces[g_rook_b[1]] = 0;
  g_board->pieces[g_king_b]    = 0;
  g_board->pieces[56 + 3]      = -4;
  g_board->pieces[56 + 2]      = -6;
  g_board->black[3]            = (g_board->black[3] ^ Bit(g_rook_b[1])) | Bit(56 + 3);
  g_board->black[5]            = (g_board->black[5] ^ Bit(g_king_b))    | Bit(56 + 2);

  if (!ChecksW())
    ++g_moves_n;
}

void AddOOW() {
  if ((g_board->castle & 0x1) && !(g_castle_empty_w[0] & g_both)) {
    AddCastleOOW();
    g_board = g_board_orig;
  }
}

void AddOOOW() {
  if ((g_board->castle & 0x2) && !(g_castle_empty_w[1] & g_both)) {
    AddCastleOOOW();
    g_board = g_board_orig;
  }
}

void MgenCastlingMovesW() {
  AddOOW();
  AddOOOW();
}

void AddOOB() {
  if ((g_board->castle & 0x4) && !(g_castle_empty_b[0] & g_both)) {
    AddCastleOOB();
    g_board = g_board_orig;
  }
}

void AddOOOB() {
  if ((g_board->castle & 0x8) && !(g_castle_empty_b[1] & g_both)) {
    AddCastleOOOB();
    g_board = g_board_orig;
  }
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
  if (g_board->pieces[to] != +1)
    return;

  g_board->fifty = 0;
  if (to == g_board_orig->epsq) {
    g_board->score          = 10; // PxP
    g_board->pieces[to - 8] = 0;
    g_board->black[0]      ^= Bit(to - 8);
  } else if (Ycoord(from) == 1 && Ycoord(to) == 3) { // e2e4 ...
    g_board->epsq = to - 8;
  } else if (Ycoord(to) == 6) { // Bonus for 7th ranks
    g_board->score = 85 + Ycoord(to);
  }
}

void ModifyPawnStuffB(const int from, const int to) {
  if (g_board->pieces[to] != -1)
    return;

  g_board->fifty = 0;
  if (to == g_board_orig->epsq) {
    g_board->score          = 10;
    g_board->pieces[to + 8] = 0;
    g_board->white[0]      ^= Bit(to + 8);
  } else if (Ycoord(from) == 6 && Ycoord(to) == 4) {
    g_board->epsq = to + 8;
  } else if (Ycoord(to) == 1) {
    g_board->score = 85 + 7 - Ycoord(to);
  }
}

void AddPromotionW(const int from, const int to, const int piece) {
  const auto eat{g_board->pieces[to]};

  g_moves[g_moves_n]    = *g_board;
  g_board               = &g_moves[g_moves_n];
  g_board->from         = from;
  g_board->to           = to;
  g_board->score        = 110; // Highest priority
  g_board->type         = 3 + piece;
  g_board->epsq         = -1;
  g_board->fifty        = 0;
  g_board->pieces[to]   = piece;
  g_board->pieces[from] = 0;
  g_board->white[0]    ^= Bit(from);
  g_board->white[piece - 1] |= Bit(to);

  if (eat <= -1)
    g_board->black[-eat - 1] ^= Bit(to);

  if (!ChecksB()) {
    HandleCastlingRights();
    ++g_moves_n;
  }
}

void AddPromotionB(const int from, const int to, const int piece) {
  const auto eat{g_board->pieces[to]};

  g_moves[g_moves_n]    = *g_board;
  g_board               = &g_moves[g_moves_n];
  g_board->from         = from;
  g_board->to           = to;
  g_board->score        = 110;
  g_board->type         = 3 + (-piece);
  g_board->epsq         = -1;
  g_board->fifty        = 0;
  g_board->pieces[from] = 0;
  g_board->pieces[to]   = piece;
  g_board->black[0]    ^= Bit(from);
  g_board->black[-piece - 1] |= Bit(to);

  if (eat >= +1)
    g_board->white[eat - 1] ^= Bit(to);

  if (!ChecksW()) {
    HandleCastlingRights();
    ++g_moves_n;
  }
}

void AddPromotionStuffW(const int from, const int to) {
  auto *tmp{g_board};

  if (g_underpromos) {
    for (const auto p : {+5, +4, +3, +2}) { // QRBN
      AddPromotionW(from, to, p);
      g_board = tmp;
    }
  } else { // Only Q
    AddPromotionW(from, to, +5);
    g_board = tmp;
  }
}

void AddPromotionStuffB(const int from, const int to) {
  auto *tmp{g_board};

  if (g_underpromos) {
    for (const auto p : {-5, -4, -3, -2}) {
      AddPromotionB(from, to, p);
      g_board = tmp;
    }
  } else {
    AddPromotionB(from, to, -5);
    g_board = tmp;
  }
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

// If not under checks -> handle castling -> add move
inline void AddMoveIfOkW() {
  if (!ChecksB()) {
    HandleCastlingRights();
    ++g_moves_n;
  }
}

inline void AddMoveIfOkB() {
  if (!ChecksW()) {
    HandleCastlingRights();
    ++g_moves_n;
  }
}

void AddNormalStuffW(const int from, const int to) {
  const auto me{g_board->pieces[from]};
  const auto eat{g_board->pieces[to]};

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
  ++g_board->fifty;

  CheckNormalCapturesW(me, eat, to);
  ModifyPawnStuffW(from, to);
  AddMoveIfOkW();
}

void AddNormalStuffB(const int from, const int to) {
  const auto me{g_board->pieces[from]};
  const auto eat{g_board->pieces[to]};

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
}

void AddW(const int from, const int to) {
  g_board->pieces[from] == +1 && Ycoord(from) == 6 ?
    AddPromotionStuffW(from, to) :
    AddNormalStuffW(from, to);

  g_board = g_board_orig;
}

void AddB(const int from, const int to) {
  g_board->pieces[from] == -1 && Ycoord(from) == 1 ?
    AddPromotionStuffB(from, to) :
    AddNormalStuffB(from, to);

  g_board = g_board_orig;
}

void AddMovesW(const int from, std::uint64_t moves) {
  while (moves)
    AddW(from, CtzPop(&moves));
}

void AddMovesB(const int from, std::uint64_t moves) {
  while (moves)
    AddB(from, CtzPop(&moves));
}

void MgenSetupW() {
  g_white   = White();
  g_black   = Black();
  g_both    = g_white | g_black;
  g_empty   = ~g_both;
  g_pawn_sq = g_black | (g_board->epsq > 0 ? Bit(g_board->epsq) &
                           0x0000FF0000000000ULL : 0x0ULL);
}

void MgenSetupB() {
  g_white   = White();
  g_black   = Black();
  g_both    = g_white | g_black;
  g_empty   = ~g_both;
  g_pawn_sq = g_white | (g_board->epsq > 0 ? Bit(g_board->epsq) &
                           0x0000000000FF0000ULL : 0x0ULL);
}

void MgenPawnsW() {
  for (auto p{g_board->white[0]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesW(sq, g_pawn_checks_w[sq] & g_pawn_sq);
    if (Ycoord(sq) == 1) {
      if (g_pawn_1_moves_w[sq] & g_empty)
        AddMovesW(sq, g_pawn_2_moves_w[sq] & g_empty);
    } else {
      AddMovesW(sq, g_pawn_1_moves_w[sq] & g_empty);
    }
  }
}

void MgenPawnsB() {
  for (auto p{g_board->black[0]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesB(sq, g_pawn_checks_b[sq] & g_pawn_sq);
    if (Ycoord(sq) == 6) {
      if (g_pawn_1_moves_b[sq] & g_empty)
        AddMovesB(sq, g_pawn_2_moves_b[sq] & g_empty);
    } else {
      AddMovesB(sq, g_pawn_1_moves_b[sq] & g_empty);
    }
  }
}

void MgenPawnsOnlyCapturesW() {
  for (auto p{g_board->white[0]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesW(sq, Ycoord(sq) == 6 ?
      g_pawn_1_moves_w[sq] & (~g_both) :
      g_pawn_checks_w[sq] & g_pawn_sq);
  }
}

void MgenPawnsOnlyCapturesB() {
  for (auto p{g_board->black[0]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesB(sq, Ycoord(sq) == 1 ?
      g_pawn_1_moves_b[sq] & (~g_both) :
      g_pawn_checks_b[sq] & g_pawn_sq);
  }
}

void MgenKnightsW() {
  for (auto p{g_board->white[1]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesW(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenKnightsB() {
  for (auto p{g_board->black[1]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesB(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenBishopsPlusQueensW() {
  for (auto p{g_board->white[2] | g_board->white[4]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesW(sq, BishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenBishopsPlusQueensB() {
  for (auto p{g_board->black[2] | g_board->black[4]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesB(sq, BishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensW() {
  for (auto p{g_board->white[3] | g_board->white[4]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesW(sq, RookMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensB() {
  for (auto p{g_board->black[3] | g_board->black[4]}; p; ) {
    const auto sq{CtzPop(&p)};
    AddMovesB(sq, RookMagicMoves(sq, g_both) & g_good);
  }
}

void MgenKingW() {
  const auto sq{Ctz(g_board->white[5])};
  AddMovesW(sq, g_king_moves[sq] & g_good);
}

void MgenKingB() {
  const auto sq{Ctz(g_board->black[5])};
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

int MgenW(Board *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
  MgenAllW();
  return g_moves_n;
}

int MgenB(Board *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
  MgenAllB();
  return g_moves_n;
}

int MgenCapturesW(Board *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
  MgenAllCapturesW();
  return g_moves_n;
}

int MgenCapturesB(Board *moves) {
  g_moves_n    = 0;
  g_moves      = moves;
  g_board_orig = g_board;
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

void MgenRoot() {
  g_root_n = g_wtm ? MgenW(g_boards[0]) : MgenB(g_boards[0]);
}

// Evaluation

// Eval macros
#define TEMPO_BONUS        25
#define BISHOP_PAIR_BONUS  20
#define CHECKS_BONUS       17
#define FRC_PENALTY        400

// Probe Eucalyptus KPK bitbases
inline bool ProbeKPK(const bool wtm) {
  return g_board->white[0] ?
    eucalyptus::IsDraw(Ctz(g_board->white[5]),
        Ctz(g_board->white[0]), Ctz(g_board->black[5]), int(wtm)) :
    eucalyptus::IsDraw(63 - Ctz(g_board->black[5]),
        63 - Ctz(g_board->black[0]), 63 - Ctz(g_board->white[5]), int(!wtm));
}

// Detect trivial draws really fast
bool EasyDraw(const bool wtm) {
  // R/Q/r/q -> No draw
  if (g_board->white[3] | g_board->white[4] | g_board->black[3] | g_board->black[4])
    return false;

  // N/B/n/b -> Drawish ?
  if (g_board->white[1] | g_board->white[2] | g_board->black[1] | g_board->black[2])
    return (g_board->white[0] | g_board->black[0]) ?
      // Pawns ? -> No draw
      false :
      // Max 1 N/B per side -> Draw
      PopCount(g_board->white[1] | g_board->white[2]) <= 1 &&
      PopCount(g_board->black[1] | g_board->black[2]) <= 1;

  // No N/B/R/Q/n/b/r/q -> Pawns ?
  const auto pawns_n{PopCount(g_board->white[0] | g_board->black[0])};

  // Check KPK ? / Bare kings ? -> Draw
  return pawns_n == 1 ? ProbeKPK(wtm) : (pawns_n == 0);
}

// Only look for bishop penalty when it's possible
bool AnyFRCProblems() {
  return ((g_board->white[2] & Bit(0))  && (g_board->white[0] & Bit(9)))  ||
         ((g_board->white[2] & Bit(7))  && (g_board->white[0] & Bit(14))) ||
         ((g_board->black[2] & Bit(56)) && (g_board->black[0] & Bit(49))) ||
         ((g_board->black[2] & Bit(63)) && (g_board->black[0] & Bit(54)));
}

// Trapped bishop penalty in FRC
// Bishop on a1/h1/a8/h8 blocked by own pawn
int FixFRC() {
  // Small speedup since pawns can't move backwards
  if (!g_frc_problems)
    return 0;

  auto s{0};
  if ((g_board->white[2] & Bit(0))  && (g_board->white[0] & Bit(9)))  s += -FRC_PENALTY;
  if ((g_board->white[2] & Bit(7))  && (g_board->white[0] & Bit(14))) s += -FRC_PENALTY;
  if ((g_board->black[2] & Bit(56)) && (g_board->black[0] & Bit(49))) s -= -FRC_PENALTY;
  if ((g_board->black[2] & Bit(63)) && (g_board->black[0] & Bit(54))) s -= -FRC_PENALTY;
  return s;
}

// More bonus for closeness
int CloseBonus(const int sq1, const int sq2) {
  return std::pow(7 - std::abs(Xcoord(sq1) - Xcoord(sq2)), 2) +
         std::pow(7 - std::abs(Ycoord(sq1) - Ycoord(sq2)), 2);
}

int CloseAnyCornerBonus(const int sq) {
  return std::max({CloseBonus(sq, 0),  CloseBonus(sq, 7),
                   CloseBonus(sq, 56), CloseBonus(sq, 63)});
}

// Mirror horizontal
int FlipY(const int sq) {
  return sq ^ 56;
}

// Classical evaluation. To finish the game or no NNUE
struct ClassicalEval {
  const std::uint64_t white, black, both;
  const bool wtm;
  int white_n, black_n, both_n, wk, bk, wpn, wnn, wbn, wrn, wqn,
      bpn, bnn, bbn, brn, bqn, score, mg, eg, scale_factor;

  // explicit -> force curly init
  explicit ClassicalEval(const bool wtm2) :
    white{White()}, black{Black()}, both{this->white | this->black}, wtm{wtm2}, white_n{0},
    black_n{0}, both_n{0}, wk{0}, bk{0}, wpn{0}, wnn{0}, wbn{0}, wrn{0}, wqn{0}, bpn{0},
    bnn{0}, bbn{0}, brn{0}, bqn{0}, score{0}, mg{0}, eg{0}, scale_factor{1} { }

  void mgeg(const int mg2, const int eg2) {
    this->mg += mg2;
    this->eg += eg2;
  }

  inline void pesto_w(const int p, const int sq) {
    this->mgeg(+kPesto[p][0][sq], +kPesto[p][1][sq]);
  }

  inline void pesto_b(const int p, const int sq) {
    this->mgeg(-kPesto[p][0][FlipY(sq)], -kPesto[p][1][FlipY(sq)]);
  }

  inline void mobility_w(const int k, const std::uint64_t m) {
    this->score += k * PopCount(m);
  }

  inline void mobility_b(const int k, const std::uint64_t m) {
    this->score -= k * PopCount(m);
  }

  void pawn_w(const int sq) {
    ++this->wpn;
    this->pesto_w(0, sq);
  }

  void pawn_b(const int sq) {
    ++this->bpn;
    this->pesto_b(0, sq);
  }

  void knight_w(const int sq) {
    ++this->wnn;
    this->pesto_w(1, sq);
    this->mobility_w(2, g_knight_moves[sq] & (~this->white));
  }

  void knight_b(const int sq) {
    ++this->bnn;
    this->pesto_b(1, sq);
    this->mobility_b(2, g_knight_moves[sq] & (~this->black));
  }

  void bishop_w(const int sq) {
    ++this->wbn;
    this->pesto_w(2, sq);
    this->mobility_w(3, BishopMagicMoves(sq, this->both) & (~this->white));
  }

  void bishop_b(const int sq) {
    ++this->bbn;
    this->pesto_b(2, sq);
    this->mobility_b(3, BishopMagicMoves(sq, this->both) & (~this->black));
  }

  void rook_w(const int sq) {
    ++this->wrn;
    this->pesto_w(3, sq);
    this->mobility_w(3, RookMagicMoves(sq, this->both) & (~this->white));
  }

  void rook_b(const int sq) {
    ++this->brn;
    this->pesto_b(3, sq);
    this->mobility_b(3, RookMagicMoves(sq, this->both) & (~this->black));
  }

  void queen_w(const int sq) {
    ++this->wqn;
    this->pesto_w(4, sq);
    this->mobility_w(2, (BishopMagicMoves(sq, this->both) | RookMagicMoves(sq, this->both)) &
                          (~this->white));
  }

  void queen_b(const int sq) {
    ++this->bqn;
    this->pesto_b(4, sq);
    this->mobility_b(2, (BishopMagicMoves(sq, this->both) | RookMagicMoves(sq, this->both)) &
                          (~this->black));
  }

  void king_w(const int sq) {
    this->wk = sq;
    this->pesto_w(5, sq);
    this->mobility_w(1, g_king_moves[sq] & (~this->white));
  }

  void king_b(const int sq) {
    this->bk = sq;
    this->pesto_b(5, sq);
    this->mobility_b(1, g_king_moves[sq] & (~this->black));
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

  void evaluate_pieces() {
    for (auto b{this->both}; b; )
      this->eval_piece(CtzPop(&b));

    this->white_n = this->wpn + this->wnn + this->wbn + this->wrn + this->wqn + 1;
    this->black_n = this->bpn + this->bnn + this->bbn + this->brn + this->bqn + 1;
    this->both_n  = this->white_n + this->black_n;
  }

  void bonus_knbk_w() {
    this->score += (2 * CloseBonus(this->wk, this->bk)) +
                   ((g_board->white[2] & 0xaa55aa55aa55aa55ULL) ?
                     10 * std::max(CloseBonus(0, this->bk), CloseBonus(63, this->bk)) :
                     10 * std::max(CloseBonus(7, this->bk), CloseBonus(56, this->bk)));
  }

  void bonus_knbk_b() {
    this->score -= (2 * CloseBonus(this->wk, this->bk)) +
                   ((g_board->black[2] & 0xaa55aa55aa55aa55ULL) ?
                     10 * std::max(CloseBonus(0, this->wk), CloseBonus(63, this->wk)) :
                     10 * std::max(CloseBonus(7, this->wk), CloseBonus(56, this->wk)));
  }

  void bonus_tempo() {
    this->score += this->wtm ? +TEMPO_BONUS : -TEMPO_BONUS;
  }

  // Force black king in the corner and get closer
  void bonus_mating_w() {
    this->score += 6 * CloseAnyCornerBonus(this->bk) + 4 * CloseBonus(this->wk, this->bk);
  }

  void bonus_mating_b() {
    this->score -= 6 * CloseAnyCornerBonus(this->wk) + 4 * CloseBonus(this->bk, this->wk);
  }

  void bonus_checks() {
    if (     ChecksW()) this->score += CHECKS_BONUS;
    else if (ChecksB()) this->score -= CHECKS_BONUS;
  }

  void bonus_bishop_pair() {
    if (this->wbn >= 2) this->score += BISHOP_PAIR_BONUS;
    if (this->bbn >= 2) this->score -= BISHOP_PAIR_BONUS;
  }

  void bonus_special_4men() {
    // KQvK(RNB)
    if (this->wqn && (this->brn || this->bnn || this->bbn)) {
      this->bonus_mating_w();
    // KRvK(NB) -> Drawish
    } else if (this->wrn && (this->bnn || this->bbn)) {
      this->scale_factor = 4;
      this->bonus_mating_w();
    // K(RNB)vKQ
    } else if (this->bqn && (this->wrn || this->wnn || this->wbn)) {
      this->bonus_mating_b();
    // K(NB)vKR -> Drawish
    } else if (this->brn && (this->wnn || this->wbn)) {
      this->scale_factor = 4;
      this->bonus_mating_b();
    }
  }

  void bonus_special_5men() {
    // KRRvKR / KR(NB)vK(NB)
    if (     (this->wrn == 2 && this->brn) ||
        (this->wrn && (this->wbn || this->wnn) && (this->bbn || this->bnn)))
      this->bonus_mating_w();
    // KRvKRR / K(NB)vKR(NB)
    else if ((this->brn == 2 && this->wrn) ||
        (this->brn && (this->bbn || this->bnn) && (this->wbn || this->wnn)))
      this->bonus_mating_b();
  }

  // Blind KBPvK ( Not perfect, just a hint ) -> Drawish ?
  void check_blind_bishop_w() {
    if (this->white_n == 3 && this->wbn && this->wpn) {
      const auto wpx{Xcoord(Ctz(g_board->white[0]))};
      const auto color{g_board->white[2] & 0x55aa55aa55aa55aaULL};
      if ((color && wpx == 7) || (!color && wpx == 0))
        this->scale_factor = 2;
    }
  }

  void check_blind_bishop_b() {
    if (this->black_n == 3 && this->bbn && this->bpn) {
      const auto bpx{Xcoord(Ctz(g_board->black[0]))};
      const auto color{g_board->black[2] & 0x55aa55aa55aa55aaULL};
      if ((!color && bpx == 7) || (color && bpx == 0))
        this->scale_factor = 2;
    }
  }

  void white_is_mating() {
    if (this->white_n == 3 && this->wnn && this->wbn) {
      this->bonus_knbk_w();
    } else {
      this->bonus_mating_w();
      this->check_blind_bishop_w();
    }
  }

  void black_is_mating() {
    if (this->black_n == 3 && this->bnn && this->bbn) {
      this->bonus_knbk_b();
    } else {
      this->bonus_mating_b();
      this->check_blind_bishop_b();
    }
  }

  // Special EG functions. To avoid always doing "Tabula rasa"
  void bonus_special() {
    if (     this->black_n == 1) this->white_is_mating();
    else if (this->white_n == 1) this->black_is_mating();
    else if (this->both_n  == 4) this->bonus_special_4men();
    else if (this->both_n  == 5) this->bonus_special_5men();
  }

  int calculate_score() {
    // 64 phases for HCE
    const auto n{std::clamp(2 * this->both_n, 2 * 2, 2 * MAX_PIECES)};
    const auto s{(n * this->mg + (2 * MAX_PIECES - n) * this->eg) /
                   (2 * MAX_PIECES)};
    return (this->score + s) / this->scale_factor;
  }

  int evaluate() {
    this->evaluate_pieces();
    this->bonus_tempo();
    this->bonus_checks();
    this->bonus_bishop_pair();
    this->bonus_special();
    return this->calculate_score() + (FixFRC() / 4);
  }
};

struct NnueEval {
  const bool wtm;

  explicit NnueEval(const bool wtm2) :
    wtm{wtm2} { }

  int probe() {
    auto i{2};

    for (auto both{Both()}; both; )
      switch (const auto sq{CtzPop(&both)}; g_board->pieces[sq]) {
        case +1: case +2: case +3: case +4: case +5: // PNBRK
          g_nnue_pieces[i]    = 7  - g_board->pieces[sq];
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
    const auto hash{Hash(this->wtm)};
    auto *entry{&g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)]};

    if (entry->eval_hash == hash)
      return entry->score;

    entry->eval_hash = hash;
    return (entry->score = (this->probe() + FixFRC()));
  }
};

int EvaluateClassical(const bool wtm) {
  return ClassicalEval(wtm).evaluate();
}

int EvaluateNNUE(const bool wtm) {
  return NnueEval(wtm).evaluate() / 4;
}

// 0    (Random Mover)
// 100  (Full Strength)
// 1-99 (Levels)
int LevelNoise() { // 0 -> 5 pawns
  return g_level == 100 ? 0 : 5 * Random(-g_level, +g_level);
}

int Evaluate(const bool wtm) {
  return LevelNoise() +
    (EasyDraw(wtm) ?
      0 :
      (g_scale[g_board->fifty] * static_cast<float>(
        g_classical ? EvaluateClassical(wtm) : EvaluateNNUE(wtm))));
}

// Search

void Speak(const int score, const std::uint64_t ms) {
  std::cout << "info depth " << std::min(g_max_depth, g_depth + 1);
  std::cout << " nodes " << g_nodes;
  std::cout << " time " << ms;
  std::cout << " nps " << Nps(g_nodes, ms);
  std::cout << " score cp " << ((g_wtm ? +1 : -1) * (std::abs(score) == INF ? score / 100 : score));
  std::cout << " pv " << MoveName(g_boards[0]);
  std::cout << std::endl; // flush
}

// g_r50_positions.pop() must contain hash !
bool Draw(const bool wtm) {
  if (g_board->fifty > 100 || EasyDraw(wtm))
    return true;

  const auto hash{g_r50_positions[g_board->fifty]};

  // Contains rook + pawn endgames only
  if (poseidon::IsDraw(hash))
    return true;

  // Only 2 rep
  for (auto i{g_board->fifty - 2}; i >= 0; i -= 2)
    if (g_r50_positions[i] == hash)
      return true;

  return false;
}

// Responding to "quit" / "stop" / "isready" signals
bool UserStop() {
  if (!InputAvailable())
    return false;

  ReadInput();
  if (Token("isready")) {
    std::cout << "readyok" << std::endl;
    return false;
  }

  return Token("quit") ? !(g_game_on = false) : Token("stop");
}

// Time checking
inline bool CheckTime() {
  static std::uint64_t ticks{0x0ULL};
// Read clock every 512 ticks (white / 2 x both)
#define READ_CLOCK 0x1FFULL
  return (((ticks++) & READ_CLOCK)) ?
    false :
    ((Now() >= g_stop_search_time) || UserStop());
}

// 1. Check against standpat to see whether we are better -> Done
// 2. Iterate deeper
int QSearchW(int alpha, const int beta, const int depth, const int ply) {
  ++g_nodes;

  if (g_stop_search || (g_stop_search = CheckTime()))
    return 0;

  if (((alpha = std::max(alpha, Evaluate(true))) >= beta) || depth <= 0)
    return alpha;

  const auto moves_n{MgenTacticalW(g_boards[ply])};
  SortNthMoves<true>(moves_n); // Very few moves, so sort them all

  for (auto i{0}; i < moves_n; ++i) {
    g_board = g_boards[ply] + i;
    if ((alpha = std::max(alpha, QSearchB(alpha, beta, depth - 1, ply + 1))) >= beta)
      return alpha;
  }

  return alpha;
}

int QSearchB(const int alpha, int beta, const int depth, const int ply) {
  ++g_nodes;

  if (g_stop_search)
    return 0;

  if ((alpha >= (beta = std::min(beta, Evaluate(false)))) || depth <= 0)
    return beta;

  const auto moves_n{MgenTacticalB(g_boards[ply])};
  SortNthMoves<true>(moves_n);

  for (auto i{0}; i < moves_n; ++i) {
    g_board = g_boards[ply] + i;
    if (alpha >= (beta = std::min(beta, QSearchW(alpha, beta, depth - 1, ply + 1))))
      return beta;
  }

  return beta;
}

// Update hashtable sorting algorithm
void UpdateSort(HashEntry *entry, const MoveType type,
    const std::uint64_t hash, const std::uint8_t index) {
  entry->sort_hash = hash;
  switch (type) {
    case MoveType::kKiller: entry->killer = index + 1; break;
    case MoveType::kGood:   entry->good   = index + 1; break;
    case MoveType::kQuiet:  entry->quiet  = index + 1; break;
  }
}

// a >= b -> Minimizer won't pick any better move anyway.
//           So searching beyond is a waste of time.
int SearchMovesW(int alpha, const int beta, int depth, const int ply) {
  const auto hash{g_r50_positions[g_board->fifty]};
  const auto checks{ChecksB()};
  const auto moves_n{MgenW(g_boards[ply])};

  if (!moves_n)
    return checks ? -INF : 0; // Checkmate or stalemate

  if (moves_n == 1 || (depth == 1 && (checks || g_board->type == 8)))
    ++depth; // Extend interesting path (SRE / CE / PPE)

  const auto ok_lmr{moves_n >= 5 && depth >= 2 && !checks};
  auto *entry{&g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)]};
  SortByScore(entry, hash);

  for (auto i{0}; i < moves_n; ++i) {
    g_board = g_boards[ply] + i;
    g_is_pv = i <= 1 && !g_boards[ply][i].score;

    if (ok_lmr && i >= 1 && !g_board->score && !ChecksW()) {
      if (SearchB(alpha, beta, depth - 2 - g_lmr[depth][i], ply + 1) <= alpha)
        continue;
      g_board = g_boards[ply] + i;
    }

    // Improved scope
    if (const auto score{SearchB(alpha, beta, depth - 1, ply + 1)}; score > alpha) {
      if ((alpha = score) >= beta) {
        UpdateSort(entry, MoveType::kKiller, hash, g_boards[ply][i].index);
        return alpha;
      }
      UpdateSort(entry, g_boards[ply][i].score ? MoveType::kGood : MoveType::kQuiet,
                 hash, g_boards[ply][i].index);
    }
  }

  return alpha;
}

int SearchMovesB(const int alpha, int beta, int depth, const int ply) {
  const auto hash{g_r50_positions[g_board->fifty]};
  const auto checks{ChecksW()};
  const auto moves_n{MgenB(g_boards[ply])};

  if (!moves_n)
    return checks ? +INF : 0;

  if (moves_n == 1 || (depth == 1 && (checks || g_board->type == 8)))
    ++depth;

  const auto ok_lmr{moves_n >= 5 && depth >= 2 && !checks};
  auto *entry{&g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)]};
  SortByScore(entry, hash);

  for (auto i{0}; i < moves_n; ++i) {
    g_board = g_boards[ply] + i;
    g_is_pv = i <= 1 && !g_boards[ply][i].score;

    if (ok_lmr && i >= 1 && !g_board->score && !ChecksB()) {
      if (SearchW(alpha, beta, depth - 2 - g_lmr[depth][i], ply + 1) >= beta)
        continue;
      g_board = g_boards[ply] + i;
    }

    if (const auto score{SearchW(alpha, beta, depth - 1, ply + 1)}; score < beta) {
      if (alpha >= (beta = score)) {
        UpdateSort(entry, MoveType::kKiller, hash, g_boards[ply][i].index);
        return beta;
      }
      UpdateSort(entry, g_boards[ply][i].score ? MoveType::kGood : MoveType::kQuiet,
                 hash, g_boards[ply][i].index);
    }
  }

  return beta;
}

// If we do nothing and we are still better -> Done
bool TryNullMoveW(int *alpha, const int beta, const int depth, const int ply) {
  // No nullmove on the path ?
  if ((!g_nullmove_active) &&
      // Not pv ?
      (!g_is_pv) &&
      // Enough depth (2 blunders too much, 3 sweet spot ... ) ?
      (depth >= 3) &&
      // Non pawn material or at least 2 pawns ( Zugzwang ... ) ?
      ((g_board->white[1] | g_board->white[2] | g_board->white[3] | g_board->white[4]) ||
         (PopCount(g_board->white[0]) >= 2)) &&
      // Not under checks ?
      (!ChecksB()) &&
      // Looks good ?
      (Evaluate(true) >= beta)) {
    const auto ep{g_board->epsq};
    auto *tmp{g_board};
    g_board->epsq = -1;

    g_nullmove_active = true;
    const auto score{SearchB(*alpha, beta, depth - int(depth / 4 + 3), ply)};
    g_nullmove_active = false;

    g_board       = tmp;
    g_board->epsq = ep;

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
    const auto ep{g_board->epsq};
    auto *tmp{g_board};
    g_board->epsq = -1;

    g_nullmove_active = true;
    const auto score{SearchW(alpha, *beta, depth - int(depth / 4 + 3), ply)};
    g_nullmove_active = false;

    g_board       = tmp;
    g_board->epsq = ep;

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

  if (g_stop_search || (g_stop_search = CheckTime()))
    return 0;

  if (depth <= 0 || ply >= MAX_DEPTH)
    return QSearchW(alpha, beta, g_q_depth, ply);

  const auto fifty{g_board->fifty};
  const auto tmp{g_r50_positions[fifty]};

  if (TryNullMoveW(&alpha, beta, depth, ply))
    return alpha;

  g_r50_positions[fifty] = Hash(true);
  alpha                  = Draw(true) ? 0 : SearchMovesW(alpha, beta, depth, ply);
  g_r50_positions[fifty] = tmp;

  return alpha;
}

int SearchB(const int alpha, int beta, const int depth, const int ply) {
  ++g_nodes;

  if (g_stop_search)
    return 0;

  if (depth <= 0 || ply >= MAX_DEPTH)
    return QSearchB(alpha, beta, g_q_depth, ply);

  const auto fifty{g_board->fifty};
  const auto tmp{g_r50_positions[fifty]};

  if (TryNullMoveB(alpha, &beta, depth, ply))
    return beta;

  g_r50_positions[fifty] = Hash(false);
  beta                   = Draw(false) ? 0 : SearchMovesB(alpha, beta, depth, ply);
  g_r50_positions[fifty] = tmp;

  return beta;
}

// Root search
int BestW() {
  int score{0}, best_i{0}, alpha{-INF};

  for (auto i{0}; i < g_root_n; ++i) {
    g_board = g_boards[0] + i;
    g_is_pv = i <= 1 && !g_boards[0][i].score; // 1 + 2 moves too good and not tactical -> pv

    if (g_depth >= 1 && i >= 1) { // Null window search for bad moves
      if ((score = SearchB(alpha, alpha + 1, g_depth, 1)) > alpha)
        g_board = g_boards[0] + i, score = SearchB(alpha, +INF, g_depth, 1);
    } else {
      score = SearchB(alpha, INF, g_depth, 1);
    }

    if (g_stop_search)
      return g_best_score;

    if (score > alpha) {
      // Skip underpromos unless really good ( 3+ pawns )
      if (IsUnderpromo(g_boards[0] + i) && ((score + (3 * 100)) < alpha))
        continue;
      alpha  = score;
      best_i = i;
    }
  }

  SortRoot(best_i);
  return alpha;
}

int BestB() {
  int score{0}, best_i{0}, beta{+INF};

  for (auto i{0}; i < g_root_n; ++i) {
    g_board = g_boards[0] + i;
    g_is_pv = i <= 1 && !g_boards[0][i].score;

    if (g_depth >= 1 && i >= 1) {
      if ((score = SearchW(beta - 1, beta, g_depth, 1)) < beta)
        g_board = g_boards[0] + i, score = SearchW(-INF, beta, g_depth, 1);
    } else {
      score = SearchW(-INF, beta, g_depth, 1);
    }

    if (g_stop_search)
      return g_best_score;

    if (score < beta) {
      if (IsUnderpromo(g_boards[0] + i) && ((score - (3 * 100)) > beta))
        continue;
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

  Material() :
    white_n{PopCount(White())},
    black_n{PopCount(Black())},
    both_n{this->white_n + this->black_n} { }

  // KRRvKR / KRvKRR / KRRRvK / KvKRRR ?
  bool is_rook_ending() const {
    return this->both_n == 5 && PopCount(g_board->white[3] | g_board->black[3]) == 3;
  }

  // Vs king + (PNBRQ) ?
  bool is_easy() const {
    return g_wtm ? this->black_n <= 2 : this->white_n <= 2;
  }

  // 5 pieces or less both side -> Endgame
  bool is_endgame() const {
    return g_wtm ? this->black_n <= 5 : this->white_n <= 5;
  }
};

bool HCEActivation(const Material &m) {
  return (!g_nnue_exist)    || // No NNUE
         m.is_easy()        || // Easy
         m.is_rook_ending() || // Rook ending
         m.both_n >= 33;       // Disable NNUE with 32+ pieces
}

// Play the book move from root list
bool FindBookMove(const int from, const int to, const int type) {
  if (type) {
    for (auto i{0}; i < g_root_n; ++i)
      if (g_boards[0][i].type == type) {
        SwapMoveInRootList(i);
        return true;
      }
  } else {
    for (auto i{0}; i < g_root_n; ++i)
      if (g_boards[0][i].from == from && g_boards[0][i].to == to) {
        SwapMoveInRootList(i);
        return true;
      }
  }

  return false;
}

int BookSolveType(const int from, const int to, const int move) {
  // PolyGlot promos (=n / =b / =r / =q)
  auto is_promo{[&move](const int v){ return move & (0x1 << (12 + v)); }};
  constexpr std::array<int, 4> v{0, 1, 2, 3};
  if (const auto res{std::find_if(v.begin(), v.end(), is_promo)}; res != v.end())
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

// Probe PolyGlot book
bool ProbeBook() {
  if (const auto move{g_book.setup(g_board->pieces, Both(),
                      g_board->castle, g_board->epsq, g_wtm)
                            .probe(BOOK_BEST)}) {
    const int from{8 * ((move >> 9) & 0x7) + ((move >> 6) & 0x7)};
    const int to{  8 * ((move >> 3) & 0x7) + ((move >> 0) & 0x7)};
    return FindBookMove(from, to, BookSolveType(from, to, move));
  }

  return false;
}

bool RandomMove() {
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
    Speak(g_last_eval, 0);
    return true;
  }

  return false;
}

void SearchRootMoves(const bool is_eg) {
  auto good{0}; // Good score per iterations for HCE activation
  const auto now{Now()};

  for ( ; std::abs(g_best_score) != INF && g_depth < g_max_depth && !g_stop_search;
      ++g_depth) {
    g_best_score = g_wtm ? BestW() : BestB();

    // Switch to classical only when the game is decided ( 4+ pawns ) !
    g_classical = g_classical ||
                  (is_eg && std::abs(g_best_score) > (4 * 100) && ((++good) >= 7));

    Speak(g_best_score, Now() - now);
    g_q_depth = std::min(g_q_depth + 2, MAX_Q_DEPTH);
  }

  Speak((g_last_eval = g_best_score), Now() - now);
}

void ThinkReset() {
  g_stop_search = g_nullmove_active = g_is_pv = false;
  g_q_depth = g_best_score = g_nodes = g_depth = 0;
  g_frc_problems = AnyFRCProblems();
}

void Think(const int ms) {
  auto *tmp{g_board};
  ThinkReset();
  MgenRoot();
  if (FastMove(ms))
    return;

  const Material m{};
  g_classical        = HCEActivation(m);
  g_stop_search_time = Now() + static_cast<std::uint64_t>(ms);
  EvalRootMoves();
  SortNthMoves<true>(g_root_n);

  // Underpromos are almost useless for gameplay
  // Disable if you need "full" analysis
  g_underpromos = false;
  SearchRootMoves(m.is_endgame());
  g_underpromos = true;
  g_board       = tmp;
}

// UCI

void UciMake(const int root_i) {
  g_r50_positions[g_board->fifty] = Hash(g_wtm);
  g_board_tmp = g_boards[0][root_i];
  g_board     = &g_board_tmp;
  g_wtm       = !g_wtm;
}

void UciMakeMove() {
  const auto move{TokenNth()};

  MgenRoot();
  for (auto i{0}; i < g_root_n; ++i)
    if (move == MoveName(g_boards[0] + i)) {
      UciMake(i);
      return;
    }

  Ok(false, "Bad move");
}

void UciTakeSpecialFen() {
  TokenPop(); // fen

  std::string fen{};
  for ( ; TokenOk() && !TokenPeek("moves"); TokenPop())
    fen += TokenNth() + " ";

  Fen(fen);
}

void UciFen() {
  Token("startpos") ?
    Fen(STARTPOS) :
    UciTakeSpecialFen();
}

void UciMoves() {
  for ( ; TokenOk(); TokenPop())
    UciMakeMove();
}

void UciPosition() {
  UciFen();
  if (Token("moves"))
    UciMoves();
}

void UciSetoption() {
  if (TokenPeek("name") && TokenPeek("value", 2)) {
    if (TokenPeek("UCI_Chess960", 1)) {
      g_chess960 = TokenPeek("true", 3);
      TokenPop(4);
    } else if (TokenPeek("Hash", 1)) {
      SetHashtable(TokenNumber(3));
      TokenPop(4);
    } else if (TokenPeek("Level", 1)) {
      g_level = std::clamp(TokenNumber(3), 0, 100);
      TokenPop(4);
    } else if (TokenPeek("MoveOverhead", 1)) {
      g_move_overhead = std::clamp(TokenNumber(3), 0, 10000);
      TokenPop(4);
    } else if (TokenPeek("EvalFile", 1)) {
      SetNNUE(TokenNth(3), true);
      TokenPop(4);
    } else if (TokenPeek("BookFile", 1)) {
      SetBook(TokenNth(3), true);
      TokenPop(4);
    }
  }
}

void PrintBestMove() {
  std::cout << "bestmove ";
  std::cout << (g_root_n <= 0 ? "0000" : MoveName(g_boards[0]));
  std::cout << std::endl;
}

void UciGoInfinite() {
  Think(INF);
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

// Make sure we never lose on time
// Small overhead buffer to prevent time losses
void UciGo() {
  int wtime{0}, btime{0}, winc{0}, binc{0}, mtg{26};

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
  std::cout << "id name " << VERSION << "\n";
  std::cout << "id author Toni Helminen" << "\n";
  std::cout << "option name UCI_Chess960 type check default false" << "\n";
  std::cout << "option name Level type spin default 100 min 0 max 100" << "\n";
  std::cout << "option name MoveOverhead type spin default " <<
                MOVEOVERHEAD << " min 0 max 10000" << "\n";
  std::cout << "option name Hash type spin default " <<
                HASH << " min 4 max 1048576" << "\n";
  std::cout << "option name EvalFile type string default " <<
                EVAL_FILE << "\n";
  std::cout << "option name BookFile type string default " <<
                BOOK_FILE << "\n";
  std::cout << "uciok";
  std::cout << std::endl;
}

// "myid" is for correctness of the program
// "bench" is for speed of the program
// myid (NNUE): 6800614
// myid (HCE):  7622297
// Don't run anything after these commands !!!
template <bool bench>
void UciBench() {
  const auto now{Now()};
  std::uint64_t nodes{0x0ULL};
  g_max_depth = bench ? MAX_DEPTH : 10;

  SetHashtable(256);    // 256MB
  g_noise      = 0;     // Make search deterministic
  g_book_exist = false; // Disable book

  for (const auto &fen : kBench) {
    std::cout << "[ " << fen << " ]" << "\n";
    Fen(fen);
    Think(bench ? 5000 : INF);
    nodes += g_nodes;
    std::cout << std::endl;
  }

  std::cout << std::string(16, '=') << "\n\n";
  std::cout << "Mode:  " << (bench ? "bench" : "myid") <<
               (g_nnue_exist ? " (NNUE)" : " (HCE)") << "\n";
  std::cout << "Nodes: " << nodes << "\n";
  std::cout << "NPS:   " << Nps(nodes, Now() - now) << "\n";
  std::cout << "Time:  " << (Now() - now);
  std::cout << std::endl;
}

bool UciCommands() {
  if (!TokenOk())
    return true;

  if (Token("position"))        UciPosition();
  else if (Token("go"))         UciGo();
  else if (Token("ucinewgame")) g_last_eval = 0;
  else if (Token("isready"))    std::cout << "readyok" << std::endl;
  else if (Token("setoption"))  UciSetoption();
  else if (Token("uci"))        UciUci();
  else if (Token("myid"))       UciBench<false>();
  else if (Token("bench"))      UciBench<true>();
  else if (Token("quit"))       return false;

  return g_game_on;
}

bool Uci() {
  ReadInput();
  return UciCommands();
}

// Init

std::uint64_t PermutateBb(const std::uint64_t moves, const int index) {
  int total{0}, good[64]{};
  std::uint64_t permutations{0x0ULL};

  for (auto i{0}; i < 64; ++i)
    if (moves & Bit(i))
      good[total++] = i; // post inc

  const auto popn{PopCount(moves)};
  for (auto i{0}; i < popn; ++i)
    if ((0x1 << i) & index)
      permutations |= Bit(good[i]);

  return permutations & moves;
}

std::uint64_t MakeSliderMagicMoves(const int *slider_vectors,
    const int sq, const std::uint64_t moves) {
  std::uint64_t possible_moves{0x0ULL};
  const auto x_pos{Xcoord(sq)};
  const auto y_pos{Ycoord(sq)};

  for (auto i{0}; i < 4; ++i)
    for (auto j{1}; j < 8; ++j) {
      const auto x{x_pos + j * slider_vectors[2 * i]};
      const auto y{y_pos + j * slider_vectors[2 * i + 1]};
      if (!OnBoard(x, y))
        break;
      const auto tmp{Bit(8 * y + x)};
      possible_moves |= tmp;
      if (tmp & moves)
        break;
    }

  return possible_moves & (~Bit(sq));
}

void InitBishopMagics() {
  constexpr int bishop_vectors[8]{+1, +1, -1, -1, +1, -1, -1, +1};

  for (auto i{0}; i < 64; ++i) {
    const auto magics{kBishopMagic[2][i] & (~Bit(i))};
    for (auto j{0}; j < 512; ++j) {
      const auto allmoves{PermutateBb(magics, j)};
      g_bishop_magic_moves[i][BishopMagicIndex(i, allmoves)] =
        MakeSliderMagicMoves(bishop_vectors, i, allmoves);
    }
  }
}

void InitRookMagics() {
  constexpr int rook_vectors[8]{+1,  0,  0, +1,  0, -1, -1,  0};

  for (auto i{0}; i < 64; ++i) {
    const auto magics{kRookMagic[2][i] & (~Bit(i))};
    for (auto j{0}; j < 4096; ++j) {
      const auto allmoves{PermutateBb(magics, j)};
      g_rook_magic_moves[i][RookMagicIndex(i, allmoves)] =
        MakeSliderMagicMoves(rook_vectors, i, allmoves);
    }
  }
}

std::uint64_t MakeSliderMoves(const int sq, const int *slider_vectors) {
  std::uint64_t moves{0x0ULL};
  const auto x_pos{Xcoord(sq)};
  const auto y_pos{Ycoord(sq)};

  for (auto i{0}; i < 4; ++i) {
    const auto dx{slider_vectors[2 * i]};
    const auto dy{slider_vectors[2 * i + 1]};
    std::uint64_t tmp{0x0ULL};

    for (auto j{1}; j < 8; ++j)
      if (const auto x{x_pos + j * dx}, y{y_pos + j * dy}; OnBoard(x, y))
        tmp |= Bit(8 * y + x);
      else
        break;

    moves |= tmp;
  }

  return moves;
}

void InitSliderMoves() {
  constexpr int bishop_vectors[8]{+1, +1, -1, -1, +1, -1, -1, +1};
  constexpr int rook_vectors[8]{  +1,  0,  0, +1,  0, -1, -1,  0};

  for (auto i{0}; i < 64; ++i) {
    g_rook_moves[i]   = MakeSliderMoves(i, rook_vectors);
    g_bishop_moves[i] = MakeSliderMoves(i, bishop_vectors);
    g_queen_moves[i]  = g_rook_moves[i] | g_bishop_moves[i];
  }
}

std::uint64_t MakeJumpMoves(const int sq, const int len,
    const int dy, const int *jump_vectors) {
  std::uint64_t moves{0x0ULL};
  const auto x_pos{Xcoord(sq)};
  const auto y_pos{Ycoord(sq)};

  for (auto i{0}; i < len; ++i)
    if (const auto x{x_pos + jump_vectors[2 * i]},
                   y{y_pos + dy * jump_vectors[2 * i + 1]};
        OnBoard(x, y))
      moves |= Bit(8 * y + x);

  return moves;
}

void InitJumpMoves() {
  constexpr int king_vectors[16]{  +1,  0,  0, +1,  0, -1, -1,  0,
                                   +1, +1, -1, -1, +1, -1, -1, +1};
  constexpr int knight_vectors[16]{+2, +1, -2, +1, +2, -1, -2, -1,
                                   +1, +2, -1, +2, +1, -2, -1, -2};
  constexpr int pawn_check_vectors[2 * 2]{-1, +1, +1, +1};
  constexpr int pawn_1_vectors[1 * 2]{0, +1};

  for (auto i{0}; i < 64; ++i) {
    g_king_moves[i]     = MakeJumpMoves(i, 8, +1, king_vectors);
    g_knight_moves[i]   = MakeJumpMoves(i, 8, +1, knight_vectors);
    g_pawn_checks_w[i]  = MakeJumpMoves(i, 2, +1, pawn_check_vectors);
    g_pawn_checks_b[i]  = MakeJumpMoves(i, 2, -1, pawn_check_vectors);
    g_pawn_1_moves_w[i] = MakeJumpMoves(i, 1, +1, pawn_1_vectors);
    g_pawn_1_moves_b[i] = MakeJumpMoves(i, 1, -1, pawn_1_vectors);
  }

  for (auto i{0}; i < 8; ++i) {
    g_pawn_2_moves_w[ 8 + i] = MakeJumpMoves( 8 + i, 1, +1, pawn_1_vectors) |
                               MakeJumpMoves( 8 + i, 1, +2, pawn_1_vectors);
    g_pawn_2_moves_b[48 + i] = MakeJumpMoves(48 + i, 1, -1, pawn_1_vectors) |
                               MakeJumpMoves(48 + i, 1, -2, pawn_1_vectors);
  }
}

void InitZobrist() {
  for (auto i{0}; i < 13; ++i)
    for (auto j{0}; j < 64; ++j) g_zobrist_board[i][j] = Random8x64();
  for (auto i{0}; i < 64; ++i) g_zobrist_ep[i]     = Random8x64();
  for (auto i{0}; i < 16; ++i) g_zobrist_castle[i] = Random8x64();
  for (auto i{0}; i <  2; ++i) g_zobrist_wtm[i]    = Random8x64();
}

// Shuffle period 30 plies then scale
void InitScale() {
  for (auto i{0}; i < MAX_POS; ++i)
    g_scale[i] = i < 30 ? 1.0f : (1.0f - ((static_cast<float>(i - 30)) / 110.0f));
}

void InitLMR() {
  for (auto d{0}; d < MAX_DEPTH; ++d)
    for (auto m{0}; m < MAX_MOVES; ++m)
      g_lmr[d][m] = std::clamp(int(0.25 * std::log(d) * std::log(m)), 1, 6);
}

void PrintVersion() {
  std::cout << VERSION << " by Toni Helminen" << std::endl;
}

void Init() {
  InitBishopMagics();
  InitRookMagics();
  InitZobrist();
  InitSliderMoves();
  InitJumpMoves();
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
