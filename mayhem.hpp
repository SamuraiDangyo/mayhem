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

#include <thread>
#include <atomic>
#include <functional>
#include <algorithm>
#include <cmath>
#include <memory>
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <ctime>

extern "C" {
#include <sys/time.h>
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

// Constants

const std::string
  kVersion  = "Mayhem 4.1",
  kStartPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0";

const std::vector<std::string>
  kBench = {
    "2n5/kP6/8/K7/4B3/8/8/8 w - - 0",                                   // bxc8=N+
    "5r1k/1b4p1/p6p/4Pp1q/2pNnP2/7N/PPQ3PP/5R1K b - - 0",               // Qxh3
    "8/4kp2/4p1p1/2p1r3/PpP5/3R4/1P1K1PP1/8 w - - 0",                   // g4
    "5n2/pRrk2p1/P4p1p/4p3/3N4/5P2/6PP/6K1 w - - 0",                    // Nb5
    "2kr3r/pp1q1ppp/5n2/1Nb5/2Pp1B2/7Q/P4PPP/1R3RK1 w - - 0",           // Nxa7+
    "8/6pp/4p3/1p1n4/1NbkN1P1/P4P1P/1PR3K1/r7 w - - 0",                 // Rxc4+
    "2r5/2rk2pp/1pn1pb2/pN1p4/P2P4/1N2B3/nPR1KPPP/3R4 b - - 0",         // Nxd4+
    "nrq4r/2k1p3/1p1pPnp1/pRpP1p2/P1P2P2/2P1BB2/1R2Q1P1/6K1 w - - 0",   // Bxc5
    "6k1/3r4/2R5/P5P1/1P4p1/8/4rB2/6K1 b - - 0",                        // g3
    "3r2k1/5p2/6p1/4b3/1P2P3/1R2P2p/P1K1N3/8 b - - 0",                  // Rd1
    "1k1r4/pp1r1pp1/4n1p1/2R5/2Pp1qP1/3P2QP/P4PB1/1R4K1 w - - 0",       // Bxb7
    "2r1k3/6pr/p1nBP3/1p3p1p/2q5/2P5/P1R4P/K2Q2R1 w - - 0",             // Rxg7
    "2b4k/p1b2p2/2p2q2/3p1PNp/3P2R1/3B4/P1Q2PKP/4r3 w - - 0",           // Qxc6
    "R7/P4k2/8/8/8/8/r7/6K1 w - - 0",                                   // Rh8
    "r1b2rk1/ppqn1p1p/2n1p1p1/2b3N1/2N5/PP1BP3/1B3PPP/R2QK2R w KQ - 0", // Qh5
    "8/8/2N4p/p5kP/P1K5/1P6/8/4b3 w - - 0",                             // Nxa5
    "8/8/p1p5/1p5p/1P5p/8/PPP2K1p/4R1rk w - - 0",                       // Rf1
    "8/8/8/4N3/8/7p/8/5K1k w - - 0",                                    // Ng4
    "6Q1/8/8/7k/8/8/3p1pp1/3Kbrrb w - - 0",                             // Qg7
    "r2qkb1r/pppb2pp/2np1n2/5pN1/2BQP3/2N5/PPP2PPP/R1B1K2R w KQkq - 0", // Bf7+
    "r3kr2/1pp4p/1p1p4/7q/4P1n1/2PP2Q1/PP4P1/R1BB2K1 b q - 0",          // Qh1+
    "r3r1k1/pp1q1pp1/4b1p1/3p2B1/3Q1R2/8/PPP3PP/4R1K1 w - - 0",         // Qxg7+
    "r1bq1r1k/1pp1Np1p/p2p2pQ/4R3/n7/8/PPPP1PPP/R1B3K1 w - - 0"         // Rh5
  };

constexpr int
  kMaxMoves          = 218,
  kMaxDepth          = 60,
  kInf               = 1048576,
  kKingVectors[16]   = {+1,  0,  0, +1,  0, -1, -1,  0, +1, +1, -1, -1, +1, -1, -1, +1},
  kKnightVectors[16] = {+2, +1, -2, +1, +2, -1, -2, -1, +1, +2, -1, +2, +1, -2, -1, -2},
  kBishopVectors[8]  = {+1, +1, -1, -1, +1, -1, -1, +1},
  kRookVectors[8]    = {+1,  0,  0, +1,  0, -1, -1,  0},
  kMvv[6][6]         = {{10, 15, 15, 20, 25, 99}, {9, 14, 14, 19, 24, 99},
                        {9,  14, 14, 19, 24, 99}, {8, 13, 13, 18, 23, 99},
                        {7,  12, 12, 17, 22, 99}, {6, 11, 11, 16, 21, 99}},
  kTempo             = 25,
  kBishopPair        = 20,
  kChecks            = 17,
  kPesto[6][2][64]   = // Material baked in
    {{{82, 82, 82, 82, 82, 82, 82, 82, 47, 81, 62, 59, 67, 106, 120, 60, 56, 78, 78,
       72, 85, 85, 115, 70, 55, 80, 77, 94, 99, 88, 92, 57, 68, 95, 88, 103, 105, 94,
       99, 59, 76, 89, 108, 113, 147, 138, 107, 62, 180, 216, 143, 177, 150, 208, 116,
       71, 82, 82, 82, 82, 82, 82, 82, 82},
      {94, 94, 94, 94, 94, 94, 94, 94, 107, 102, 102, 104, 107, 94, 96, 87, 98, 101,
       88, 95, 94, 89, 93, 86, 107, 103, 91, 87, 87, 86, 97, 93, 126, 118, 107, 99,
       92, 98, 111, 111, 188, 194, 179, 161, 150, 147, 176, 178, 272, 267, 252, 228,
       241, 226, 259, 281, 94, 94, 94, 94, 94, 94, 94, 94}},
     {{232, 316, 279, 304, 320, 309, 318, 314, 308, 284, 325, 334, 336, 355, 323, 318,
       314, 328, 349, 347, 356, 354, 362, 321, 324, 341, 353, 350, 365, 356, 358, 329,
       328, 354, 356, 390, 374, 406, 355, 359, 290, 397, 374, 402, 421, 466, 410, 381,
       264, 296, 409, 373, 360, 399, 344, 320, 170, 248, 303, 288, 398, 240, 322, 230},
      {252, 230, 258, 266, 259, 263, 231, 217, 239, 261, 271, 276, 279, 261, 258, 237,
       258, 278, 280, 296, 291, 278, 261, 259, 263, 275, 297, 306, 297, 298, 285, 263,
       264, 284, 303, 303, 303, 292, 289, 263, 257, 261, 291, 290, 280, 272, 262, 240,
       256, 273, 256, 279, 272, 256, 257, 229, 223, 243, 268, 253, 250, 254, 218, 182}},
     {{332, 362, 351, 344, 352, 353, 326, 344, 369, 380, 381, 365, 372, 386, 398, 366,
       365, 380, 380, 380, 379, 392, 383, 375, 359, 378, 378, 391, 399, 377, 375, 369,
       361, 370, 384, 415, 402, 402, 372, 363, 349, 402, 408, 405, 400, 415, 402, 363,
       339, 381, 347, 352, 395, 424, 383, 318, 336, 369, 283, 328, 340, 323, 372, 357},
      {274, 288, 274, 292, 288, 281, 292, 280, 283, 279, 290, 296, 301, 288, 282, 270,
       285, 294, 305, 307, 310, 300, 290, 282, 291, 300, 310, 316, 304, 307, 294, 288,
       294, 306, 309, 306, 311, 307, 300, 299, 299, 289, 297, 296, 295, 303, 297, 301,
       289, 293, 304, 285, 294, 284, 293, 283, 283, 276, 286, 289, 290, 288, 280, 273}},
     {{458, 464, 478, 494, 493, 484, 440, 451, 433, 461, 457, 468, 476, 488, 471, 406,
       432, 452, 461, 460, 480, 477, 472, 444, 441, 451, 465, 476, 486, 470, 483, 454,
       453, 466, 484, 503, 501, 512, 469, 457, 472, 496, 503, 513, 494, 522, 538, 493,
       504, 509, 535, 539, 557, 544, 503, 521, 509, 519, 509, 528, 540, 486, 508, 520},
      {503, 514, 515, 511, 507, 499, 516, 492, 506, 506, 512, 514, 503, 503, 501, 509,
       508, 512, 507, 511, 505, 500, 504, 496, 515, 517, 520, 516, 507, 506, 504, 501,
       516, 515, 525, 513, 514, 513, 511, 514, 519, 519, 519, 517, 516, 509, 507, 509,
       523, 525, 525, 523, 509, 515, 520, 515, 525, 522, 530, 527, 524, 524, 520, 517}},
     {{1024, 1007, 1016, 1035, 1010, 1000, 994, 975, 990, 1017, 1036, 1027, 1033, 1040,
       1022, 1026, 1011, 1027, 1014, 1023, 1020, 1027, 1039, 1030, 1016, 999, 1016, 1015,
       1023, 1021, 1028, 1022, 998, 998, 1009, 1009, 1024, 1042, 1023, 1026, 1012, 1008,
       1032, 1033, 1054, 1081, 1072, 1082, 1001, 986, 1020, 1026, 1009, 1082, 1053, 1079,
       997, 1025, 1054, 1037, 1084, 1069, 1068, 1070},
      {903, 908, 914, 893, 931, 904, 916, 895, 914, 913, 906, 920, 920, 913, 900, 904,
       920, 909, 951, 942, 945, 953, 946, 941, 918, 964, 955, 983, 967, 970, 975, 959,
       939, 958, 960, 981, 993, 976, 993, 972, 916, 942, 945, 985, 983, 971, 955, 945,
       919, 956, 968, 977, 994, 961, 966, 936, 927, 958, 958, 963, 963, 955, 946, 956}},
     {{-15, 36, 12, -54, 8, -28, 24, 14, 1, 7, -8, -64, -43, -16, 9, 8, -14, -14, -22,
       -46, -44, -30, -15, -27, -49, -1, -27, -39, -46, -44, -33, -51, -17, -20, -12,
       -27, -30, -25, -14, -36, -9, 24, 2, -16, -20, 6, 22, -22, 29, -1, -20, -7, -8,
       -4, -38, -29, -65, 23, 16, -15, -56, -34, 2, 13},
      {-53, -34, -21, -11, -28, -14, -24, -43, -27, -11, 4, 13, 14, 4, -5, -17, -19, -3,
       11, 21, 23, 16, 7, -9, -18, -4, 21, 24, 27, 23, 9, -11, -8, 22, 24, 27, 26, 33,
       26, 3, 10, 17, 23, 15, 20, 45, 44, 13, -12, 17, 14, 17, 17, 38, 23, 11, -74, -35,
       -18, -18, -11, 15, 4, -17}}};

constexpr std::uint64_t
  kRookMagic[64] =
    {0x548001400080106cULL, 0x900184000110820ULL,  0x428004200a81080ULL,  0x140088082000c40ULL,
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
     0x280120020049902ULL,  0x1200412602009402ULL, 0x914900048020884ULL,  0x104824281002402ULL},
  kRookMask[64] =
    {0x101010101017eULL,    0x202020202027cULL,    0x404040404047aULL,    0x8080808080876ULL,
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
     0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL},
  kRookMoveMagic[64] =
    {0x101010101017eULL,    0x202020202027cULL,    0x404040404047aULL,    0x8080808080876ULL,
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
     0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL},
  kBishopMagic[64] =
    {0x2890208600480830ULL, 0x324148050f087ULL,    0x1402488a86402004ULL, 0xc2210a1100044bULL,
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
     0x180450800222a011ULL, 0x600014600490202ULL,  0x21040100d903ULL,     0x10404821000420ULL},
  kBishopMask[64] =
    {0x40201008040200ULL,   0x402010080400ULL,     0x4020100a00ULL,       0x40221400ULL,
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
     0x28440200000000ULL,   0x50080402000000ULL,   0x20100804020000ULL,   0x40201008040200ULL},
  kBishopMoveMagics[64] =
    {0x40201008040200ULL,   0x402010080400ULL,     0x4020100a00ULL,       0x40221400ULL,
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
     0x28440200000000ULL,   0x50080402000000ULL,   0x20100804020000ULL,   0x40201008040200ULL};

// Structs

struct Board_t {
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
    type,       // Move type (0: Normal, 1: OOw, 2: OOOw, 3: OOb, 4: OOOb, 5: =n, 6: =b, 7: =r, 8: =q)
    castle,     // Castling rights (0x1: K, 0x2: Q, 0x4: k, 0x8: q)
    rule50;     // Rule 50 counter
};

struct Hash_t {
  std::uint64_t
    eval_hash, sort_hash;
  std::int32_t
    score;
  std::uint8_t
    killer, good, quiet;
};

// Enums

enum class MoveType {
  kKiller,
  kGood,
  kQuiet
};

// Variables

int
  g_level = 10, g_move_overhead = 100, g_rook_w[2] = {}, g_rook_b[2] = {}, g_root_n = 0, g_king_w = 0,
  g_king_b = 0, g_moves_n = 0, g_max_depth = kMaxDepth, g_qs_depth = 0, g_depth = 0, g_best_score = 0,
  g_hash_mb = 256, g_last_eval = 0, g_lmr[kMaxDepth][kMaxMoves] = {};

std::uint32_t
  g_hash_entries = 0, g_tokens_nth = 0;

std::uint64_t
  g_seed = 0x202c7ULL, g_black = 0x0ULL, g_white = 0x0ULL, g_both = 0x0ULL, g_empty = 0x0ULL,
  g_pawn_sq = 0x0ULL,  g_pawn_1_moves_w[64] = {}, g_pawn_1_moves_b[64] = {}, g_pawn_2_moves_w[64] = {},
  g_pawn_2_moves_b[64] = {}, g_bishop_moves[64] = {}, g_rook_moves[64] = {}, g_queen_moves[64] = {},
  g_knight_moves[64] = {}, g_king_moves[64] = {}, g_pawn_checks_w[64] = {}, g_pawn_checks_b[64] = {},
  g_castle_w[2] = {}, g_castle_b[2] = {}, g_castle_empty_w[2] = {}, g_castle_empty_b[2] = {},
  g_bishop_magic_moves[64][512] = {}, g_rook_magic_moves[64][4096] = {}, g_zobrist_ep[64] = {},
  g_zobrist_castle[16] = {}, g_zobrist_wtm[2] = {}, g_zobrist_board[13][64] = {},
  g_stop_search_time = 0x0ULL, g_r50_positions[128] = {}, g_nodes = 0x0ULL, g_good = 0x0ULL;

bool
  g_chess960 = false, g_wtm = false,  g_underpromos = true, g_nullmove_active = false,
  g_is_pv = false, g_analyzing = false, g_book_exist = false, g_nnue_exist = false, g_classical = false;

std::atomic<bool>
  g_stop_search(false);

std::vector<std::string>
  g_tokens = {};

struct Board_t
  g_board_tmp = {}, *g_board = &g_board_tmp, *g_moves = 0, *g_board_orig = 0, g_root[kMaxMoves] = {};

polyglotbook::PolyglotBook
  g_book;

std::unique_ptr<struct Hash_t[]>
  g_hash;

float
  g_scale[128] = {};

std::string
  g_eval_file = "nn-cb80fb9393af.nnue",
  g_book_file = "performance.bin";

// Prototypes

int SearchB(const int, int, const int, const int);
int SearchW(int, const int, const int, const int);
int QSearchB(const int, int, const int);
int Evaluate(const bool);
bool ChecksW();
bool ChecksB();
std::uint64_t RookMagicMoves(const int, const std::uint64_t);
std::uint64_t BishopMagicMoves(const int, const std::uint64_t);

// Utils

inline std::uint64_t White() {
  return g_board->white[0] | g_board->white[1] | g_board->white[2] | g_board->white[3] | g_board->white[4] | g_board->white[5];
}

inline std::uint64_t Black() {
  return g_board->black[0] | g_board->black[1] | g_board->black[2] | g_board->black[3] | g_board->black[4] | g_board->black[5];
}

inline std::uint64_t Both() {
  return g_board->white[0] | g_board->white[1] | g_board->white[2] | g_board->white[3] | g_board->white[4] | g_board->white[5]
       | g_board->black[0] | g_board->black[1] | g_board->black[2] | g_board->black[3] | g_board->black[4] | g_board->black[5];
}

inline int Ctz(const std::uint64_t bb) {
  return __builtin_ctzll(bb);
}

inline std::uint64_t ClearBit(const std::uint64_t bb) {
  return bb & (bb - 0x1ULL);
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

template <class T>
T Between(const T x, const T y, const T z) {
  return std::max(x, std::min(y, z));
}

std::uint32_t Nps(const std::uint64_t nodes, const std::uint32_t ms) {
  return (1000 * nodes) / (ms + 1);
}

bool OnBoard(const int x, const int y) {
  return x >= 0 && x <= 7 && y >= 0 && y <= 7;
}

inline bool IsUnderpromo(const struct Board_t *b) {
  return b->type >= 5 && b->type <= 7;
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
  select(STDIN_FILENO + 1, &fd, 0, 0, &tv);
  return FD_ISSET(STDIN_FILENO, &fd) > 0;
}
#endif
inline std::uint64_t Now() {
  struct timeval tv;
  return gettimeofday(&tv, NULL) ? 0x0ULL : (1000 * tv.tv_sec + tv.tv_usec / 1000);
}

}

void Assert(const bool test, const std::string &msg) {
  if (test) return;
  std::cerr << msg << std::endl;
  std::exit(EXIT_FAILURE);
}

std::uint64_t Mixer(const std::uint64_t num) {
  return (num << 7) ^ (num >> 5);
}

std::uint64_t Random64() {
  static std::uint64_t a = 0X12311227ULL, b = 0X1931311ULL, c = 0X13138141ULL;
  a ^= b + c;
  b ^= b * c + 0x1717711ULL;
  c  = (3 * c) + 0x1ULL;
  return Mixer(a) ^ Mixer(b) ^ Mixer(c);
}

std::uint64_t Random8x64() {
  std::uint64_t ret = 0x0ULL;
  for (auto i = 0; i < 8; i++) ret ^= Random64() << (8 * i);
  return ret;
}

int Random(const int max) {
  const std::uint64_t ret = (g_seed ^ Random64()) & 0xFFFFFFFFULL;
  g_seed = (g_seed << 5) ^ (g_seed + 1) ^ (g_seed >> 3);
  return (ret & 0xFFFULL) % max;
}

int Random(const int x, const int y) {
  return x + Random(y - x + 1);
}

template <class T>
void Split(const std::string &s, T &cont, const std::string &delims = " ") {
  std::size_t cur = s.find_first_of(delims), prev = 0;

  while (cur != std::string::npos) {
    cont.push_back(s.substr(prev, cur - prev));
    prev = cur + 1;
    cur  = s.find_first_of(delims, prev);
  }

  cont.push_back(s.substr(prev, cur - prev));
}

void Input() {
  std::string line;
  std::getline(std::cin, line);
  g_tokens_nth = 0;
  g_tokens.clear();
  Split<std::vector<std::string>>(line, g_tokens);
}

// Lib

void SetupBook() {
  static std::string filename = "???";

  if (filename == g_book_file) return;

  if (g_book_file == "-") {
    g_book_exist = false;
  } else {
    g_book_exist = g_book.open_book(g_book_file);
    filename     = g_book_file;
  }

  if (!g_book_exist) std::cerr << "Warning: Missing PolyGlot BookFile !" << std::endl;
}

void SetupNNUE() {
  static std::string filename = "???";

  if (filename == g_eval_file) return;

  if (g_eval_file == "-") {
    g_nnue_exist = false;
  } else {
    g_nnue_exist = nnue::nnue_init(g_eval_file.c_str());
    filename     = g_eval_file;
  }

  if (!g_nnue_exist) std::cerr << "Warning: Missing NNUE EvalFile !" << std::endl;
}

// Hashtable

int CreateKey(const int max, const int block) {
  int ret = 1;
  for (; ret * block <= max; ret++);
  return ret - 1;
}

void SetupHashtable() {
  g_hash_mb      = Between<int>(4, g_hash_mb, 1048576); // 4 MB -> 1 TB
  g_hash_entries = CreateKey((1 << 20) * g_hash_mb, sizeof(struct Hash_t));
  g_hash.reset(new struct Hash_t[g_hash_entries]);

  for (std::size_t i = 0; i < g_hash_entries; i++)
    g_hash[i].eval_hash = g_hash[i].sort_hash = g_hash[i].score = g_hash[i].killer = g_hash[i].good = g_hash[i].quiet = 0;
}

// Hash

inline std::uint64_t Hash(const bool wtm) {
  auto hash = g_zobrist_ep[g_board->epsq + 1] ^ g_zobrist_wtm[int(wtm)] ^ g_zobrist_castle[g_board->castle];

  for (auto both = Both(); both; both = ClearBit(both)) {
    const auto sq = Ctz(both);
    hash         ^= g_zobrist_board[g_board->pieces[sq] + 6][sq];
  }

  return hash;
}

// Tokenizer

bool TokenOk(const int n = 0) {
  return g_tokens_nth + n < g_tokens.size(); // O(1)
}

const std::string TokenCurrent(const int n = 0) {
  return TokenOk(n) ? g_tokens[g_tokens_nth + n] : "";
}

void TokenPop(const int n = 1) {
  g_tokens_nth += n;
}

bool Token(const std::string &token, const int n = 1) {
  if (TokenOk(0) && token == TokenCurrent()) {
    TokenPop(n);
    return true;
  }
  return false;
}

int TokenNumber(const int n = 0) {
  return TokenOk(n) ? std::stoi(g_tokens[g_tokens_nth + n]) : 0;
}

bool Peek(const std::string &s, const int n = 0) {
  return TokenOk(n) ? s == g_tokens[g_tokens_nth + n] : false;
}

// Board

void BuildBitboards() {
  for (auto i = 0; i < 64; i++)
    if      (g_board->pieces[i] > 0) g_board->white[+g_board->pieces[i] - 1] |= Bit(i);
    else if (g_board->pieces[i] < 0) g_board->black[-g_board->pieces[i] - 1] |= Bit(i);
}

std::uint64_t Fill(int from, const int to) {
  if (from < 0 || to < 0 || from > 63 || to > 63)
    return 0x0ULL;

  auto ret = Bit(from);
  if (from == to)
    return ret;

  const auto diff = from > to ? -1 : +1;
  do {
    from += diff;
    ret  |= Bit(from);
  } while (from != to);

  return ret;
}

void FindKings() {
  for (auto i = 0; i < 64; i++)
    if (     g_board->pieces[i] == +6) g_king_w = i;
    else if (g_board->pieces[i] == -6) g_king_b = i;
}

void BuildCBB1() {
  if (g_board->castle & 0x1) {
    g_castle_w[0]       = Fill(g_king_w, 6);
    g_castle_empty_w[0] = (g_castle_w[0] | Fill(g_rook_w[0], 5     )) ^ (Bit(g_king_w) | Bit(g_rook_w[0]));
  }

  if (g_board->castle & 0x2) {
    g_castle_w[1]       = Fill(g_king_w, 2);
    g_castle_empty_w[1] = (g_castle_w[1] | Fill(g_rook_w[1], 3     )) ^ (Bit(g_king_w) | Bit(g_rook_w[1]));
  }

  if (g_board->castle & 0x4) {
    g_castle_b[0]       = Fill(g_king_b, 56 + 6);
    g_castle_empty_b[0] = (g_castle_b[0] | Fill(g_rook_b[0], 56 + 5)) ^ (Bit(g_king_b) | Bit(g_rook_b[0]));
  }

  if (g_board->castle & 0x8) {
    g_castle_b[1]       = Fill(g_king_b, 56 + 2);
    g_castle_empty_b[1] = (g_castle_b[1] | Fill(g_rook_b[1], 56 + 3)) ^ (Bit(g_king_b) | Bit(g_rook_b[1]));
  }
}

void BuildCBB2() {
  for (const auto i : {0, 1}) {
    g_castle_empty_w[i] &= 0xFFULL;
    g_castle_empty_b[i] &= 0xFF00000000000000ULL;
    g_castle_w[i]       &= 0xFFULL;
    g_castle_b[i]       &= 0xFF00000000000000ULL;
  }
}

void BuildCastlingBitboards() {
  BuildCBB1();
  BuildCBB2();
}

// Fen handling

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
    case 'k': return -6;
    default:  return  0;
  }
}

int Empty2Num(const char e) {
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

int File2Num(const char f) {
  switch (f) {
    case 'a': return 0;
    case 'b': return 1;
    case 'c': return 2;
    case 'd': return 3;
    case 'e': return 4;
    case 'f': return 5;
    case 'g': return 6;
    default:  return 7;
  }
}

int Rank2Num(const char r) {
  switch (r) {
    case '3': return 2;
    case '6': return 5;
    default:  return 0;
  }
}

void FenBoard(const std::string &board) {
  int sq = 56;
  for (std::size_t i = 0, len = board.length(); /* O(n) */ i < len && sq >= 0; i++) {
    const auto c = board[i];
    if (c == '/')             sq -= 16;
    else if (std::isdigit(c)) sq += Empty2Num(c);
    else                      g_board->pieces[sq++] = Piece2Num(c);
  }
}

void FenAddCastle(int *rooks, const int sq, const int castle) {
  *rooks           = sq;
  g_board->castle |= castle;
}

void AddChess960Castling(const char file) {
  if (file >= 'A' && file <= 'H') {
    const auto tmp = file - 'A';
    if (     tmp > g_king_w) FenAddCastle(g_rook_w + 0, tmp, 0x1);
    else if (tmp < g_king_w) FenAddCastle(g_rook_w + 1, tmp, 0x2);
  } else if (file >= 'a' && file <= 'h') {
    const auto tmp = file - 'a';
    if (     tmp > g_king_b) FenAddCastle(g_rook_b + 0, 56 + tmp, 0x4);
    else if (tmp < g_king_b) FenAddCastle(g_rook_b + 1, 56 + tmp, 0x8);
  }
}

void FenKQkq(const std::string &KQkq) {
  for (std::size_t i = 0, len = KQkq.length(); i < len; i++) {
    const auto f = KQkq[i];
    switch (f) {
      case 'K': FenAddCastle(g_rook_w + 0, 7, 0x1);      break;
      case 'Q': FenAddCastle(g_rook_w + 1, 0, 0x2);      break;
      case 'k': FenAddCastle(g_rook_b + 0, 56 + 7, 0x4); break;
      case 'q': FenAddCastle(g_rook_b + 1, 56 + 0, 0x8); break;
      default:  AddChess960Castling(f);                  break;
    }
  }
}

void FenEp(const std::string &ep) {
  if (ep.length() != 2) return;
  g_board->epsq = 8 * Rank2Num(ep[1]) + File2Num(ep[0]);
}

void FenRule50(const std::string &rule50) {
  if (rule50.length() == 0 || rule50[0] == '-') return;
  g_board->rule50 = Between<std::uint8_t>(0, std::stoi(rule50), 100);
}

void FenGen(const std::string &fen) {
  std::vector<std::string> tokens = {};

  Split<std::vector<std::string>>(std::string(fen), tokens);
  Assert(tokens.size() >= 5, "Error #1: Bad fen !");

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
  g_wtm         = true;
  g_board->epsq = -1;
  g_king_w = g_king_b = 0;

  for (const auto i : {0, 1})  g_castle_w[i] = g_castle_empty_w[i] = g_castle_b[i] = g_castle_empty_b[i] = 0x0ULL;
  for (const auto i : {0, 1})  g_rook_w[i] = g_rook_b[i] = 0;
  for (auto i = 0; i < 6; i++) g_board->white[i] = g_board->black[i] = 0x0ULL;
}

// 1 king per side and 32 <= pieces and Not under checks
bool BoardOk() {
  return (PopCount(g_board->white[5]) == 1 && PopCount(g_board->black[5]) == 1)
      && (PopCount(Both()) <= 32)
      && (g_wtm ? !ChecksW() : !ChecksB());
}

void Fen(const std::string &fen) {
  FenReset();
  FenGen(fen);
  BuildBitboards();
  Assert(BoardOk(), "Error #2: Bad board !");
}

// Checks

inline bool ChecksHereW(const int sq) {
  const auto both = Both();
  return ((g_pawn_checks_b[sq]        &  g_board->white[0])
        | (g_knight_moves[sq]         &  g_board->white[1])
        | (BishopMagicMoves(sq, both) & (g_board->white[2] | g_board->white[4]))
        | (RookMagicMoves(sq, both)   & (g_board->white[3] | g_board->white[4]))
        | (g_king_moves[sq]           &  g_board->white[5]));
}

inline bool ChecksHereB(const int sq) {
  const auto both = Both();
  return ((g_pawn_checks_w[sq]        &  g_board->black[0])
        | (g_knight_moves[sq]         &  g_board->black[1])
        | (BishopMagicMoves(sq, both) & (g_board->black[2] | g_board->black[4]))
        | (RookMagicMoves(sq, both)   & (g_board->black[3] | g_board->black[4]))
        | (g_king_moves[sq]           &  g_board->black[5]));
}

bool ChecksCastleW(std::uint64_t squares) {
  for (; squares; squares = ClearBit(squares))
    if (ChecksHereW(Ctz(squares)))
      return true;
  return false;
}

bool ChecksCastleB(std::uint64_t squares) {
  for (; squares; squares = ClearBit(squares))
    if (ChecksHereB(Ctz(squares)))
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

char Rank2Char(const int r) {
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

const std::string MoveStr(const int from, const int to) {
  return std::string{File2Char(Xcoord(from)), Rank2Char(Ycoord(from)),
                     File2Char(Xcoord(to)),   Rank2Char(Ycoord(to))};
}

char PromoLetter(const std::int8_t piece) {
  switch (std::abs(piece)) {
    case 2:  return 'n';
    case 3:  return 'b';
    case 4:  return 'r';
    default: return 'q';
  }
}

const std::string MoveName(const struct Board_t *move) {
  auto from = move->from;
  auto to   = move->to;

  switch (move->type) {
    case 1:
      from = g_king_w;
      to  = g_chess960 ? g_rook_w[0] : 6;
      break;
    case 2:
      from = g_king_w;
      to   = g_chess960 ? g_rook_w[1] : 2;
      break;
    case 3:
      from = g_king_b;
      to   = g_chess960 ? g_rook_b[0] : 56 + 6;
      break;
    case 4:
      from = g_king_b;
      to   = g_chess960 ? g_rook_b[1] : 56 + 2;
      break;
    case 5: case 6: case 7: case 8:
      return MoveStr(from, to) + PromoLetter(move->pieces[to]);
  }

  return MoveStr(from, to);
}

// Sorting

inline void Swap(struct Board_t *a, struct Board_t *b) {
  const auto tmp = *a;
  *a = *b;
  *b = tmp;
}

void SortNthMoves(const int nth) {
  for (auto i = 0; i < nth; i++)
    for (auto j = i + 1; j < g_moves_n; j++)
      if (g_moves[j].score > g_moves[i].score)
        Swap(g_moves + j, g_moves + i);
}

int EvaluateMoves() {
  auto tactics = 0;

  for (auto i = 0; i < g_moves_n; i++) {
    if (g_moves[i].score) tactics++;
    g_moves[i].index = i;
  }

  return tactics;
}

void SortAll() {
  SortNthMoves(g_moves_n);
}

void SortByScore(const struct Hash_t *entry, const std::uint64_t hash) {
  if (entry->sort_hash == hash) {
    if (entry->killer)    g_moves[entry->killer - 1].score += 10000;
    else if (entry->good) g_moves[entry->good   - 1].score += 10000;

    if (entry->quiet) g_moves[entry->quiet - 1].score += 10000;
  }

  SortNthMoves(EvaluateMoves());
}

void EvalRootMoves() {
  auto *tmp = g_board;

  for (auto i = 0; i < g_root_n; i++) {
    g_board         = g_root + i;
    g_board->score += (  g_board->type == 8 ? 1000 : 0)
                      + (g_board->type >= 1 && g_board->type <= 4 ? 100 : 0)
                      + (IsUnderpromo(g_board) ? -5000 : 0)
                      + (g_wtm ? +1 : -1) * Evaluate(g_wtm)
                      +  Random(-1, +1);
  }

  g_board = tmp;
}

void SortRoot(const int index) {
  if (!index) return;

  const auto tmp = g_root[index];
  for (auto i = index; i > 0; i--)
    g_root[i] = g_root[i - 1];

  g_root[0] = tmp;
}

// Move generator

inline std::uint64_t BishopMagicIndex(const int sq, const std::uint64_t mask) {
  return ((mask & kBishopMask[sq]) * kBishopMagic[sq]) >> 55;
}

inline std::uint64_t RookMagicIndex(const int sq, const std::uint64_t mask) {
  return ((mask & kRookMask[sq]) * kRookMagic[sq]) >> 52;
}

inline std::uint64_t BishopMagicMoves(const int sq, const std::uint64_t mask) {
  return g_bishop_magic_moves[sq][BishopMagicIndex(sq, mask)];
}

inline std::uint64_t RookMagicMoves(const int sq, const std::uint64_t mask) {
  return g_rook_magic_moves[sq][RookMagicIndex(sq, mask)];
}

void HandleCastlingW(const int mtype, const int from, const int to) {
  g_moves[g_moves_n] = *g_board;
  g_board            = &g_moves[g_moves_n];
  g_board->score     = 0;
  g_board->epsq      = -1;
  g_board->from      = from;
  g_board->to        = to;
  g_board->type      = mtype;
  g_board->castle   &= 0x4 | 0x8;
  g_board->rule50    = 0;
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
  g_board->rule50    = 0;
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

  g_moves_n++;
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

void Add_OO_W() {
  if ((g_board->castle & 0x1) && !(g_castle_empty_w[0] & g_both)) {
    AddCastleOOW();
    g_board = g_board_orig;
  }
}

void Add_OOO_W() {
  if ((g_board->castle & 0x2) && !(g_castle_empty_w[1] & g_both)) {
    AddCastleOOOW();
    g_board = g_board_orig;
  }
}

void MgenCastlingMovesW() {
  Add_OO_W();
  Add_OOO_W();
}

void Add_OO_B() {
  if ((g_board->castle & 0x4) && !(g_castle_empty_b[0] & g_both)) {
    AddCastleOOB();
    g_board = g_board_orig;
  }
}

void Add_OOO_B() {
  if ((g_board->castle & 0x8) && !(g_castle_empty_b[1] & g_both)) {
    AddCastleOOOB();
    g_board = g_board_orig;
  }
}

void MgenCastlingMovesB() {
  Add_OO_B();
  Add_OOO_B();
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
  g_board->rule50 = 0;

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
  g_board->rule50 = 0;

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
  const auto eat = g_board->pieces[to];

  g_moves[g_moves_n]    = *g_board;
  g_board               = &g_moves[g_moves_n];
  g_board->from         = from;
  g_board->to           = to;
  g_board->score        = 110;
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

void AddPromotionB(const int from, const int to, const int piece) {
  const auto eat        = g_board->pieces[to];

  g_moves[g_moves_n]    = *g_board;
  g_board               = &g_moves[g_moves_n];
  g_board->from         = from;
  g_board->to           = to;
  g_board->score        = 110;
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

void AddPromotionStuffW(const int from, const int to) {
  auto *tmp = g_board;
  if (g_underpromos) {
    for (const auto p : {+5, +2, +4, +3}) { // QNRB
      AddPromotionW(from, to, p);
      g_board = tmp;
    }
  } else { // Only Q
    AddPromotionW(from, to, +5);
    g_board = tmp;
  }
}

void AddPromotionStuffB(const int from, const int to) {
  auto *tmp = g_board;
  if (g_underpromos) {
    for (const auto p : {-5, -2, -4, -3}) {
      AddPromotionB(from, to, p);
      g_board = tmp;
    }
  } else {
    AddPromotionB(from, to, -5);
    g_board = tmp;
  }
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

void AddW(const int from, const int to) {
  (g_board->pieces[from] == 1 && Ycoord(from) == 6) ? AddPromotionStuffW(from, to)
                                                    : AddNormalStuffW(from, to);
  g_board = g_board_orig;
}

void AddB(const int from, const int to) {
  (g_board->pieces[from] == -1 && Ycoord(from) == 1) ? AddPromotionStuffB(from, to)
                                                     : AddNormalStuffB(from, to);
  g_board = g_board_orig;
}

void AddMovesW(const int from, std::uint64_t moves) {
  for (; moves; moves = ClearBit(moves))
    AddW(from, Ctz(moves));
}

void AddMovesB(const int from, std::uint64_t moves) {
  for (; moves; moves = ClearBit(moves))
    AddB(from, Ctz(moves));
}

void MgenSetupW() {
  g_white   = White();
  g_black   = Black();
  g_both    = g_white | g_black;
  g_empty   = ~g_both;
  g_pawn_sq = g_black | (g_board->epsq > 0 ? Bit(g_board->epsq) & 0x0000FF0000000000ULL : 0x0ULL);
}

void MgenSetupB() {
  g_white   = White();
  g_black   = Black();
  g_both    = g_white | g_black;
  g_empty   = ~g_both;
  g_pawn_sq = g_white | (g_board->epsq > 0 ? Bit(g_board->epsq) & 0x0000000000FF0000ULL : 0x0ULL);
}

void MgenPawnsW() {
  for (auto p = g_board->white[0]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesW(sq, g_pawn_checks_w[sq] & g_pawn_sq);
    if (Ycoord(sq) == 1) {
      if (g_pawn_1_moves_w[sq] & g_empty) AddMovesW(sq, g_pawn_2_moves_w[sq] & g_empty);
    } else {
      AddMovesW(sq, g_pawn_1_moves_w[sq] & g_empty);
    }
  }
}

void MgenPawnsB() {
  for (auto p = g_board->black[0]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesB(sq, g_pawn_checks_b[sq] & g_pawn_sq);
    if (Ycoord(sq) == 6) {
      if (g_pawn_1_moves_b[sq] & g_empty) AddMovesB(sq, g_pawn_2_moves_b[sq] & g_empty);
    } else {
      AddMovesB(sq, g_pawn_1_moves_b[sq] & g_empty);
    }
  }
}

void MgenPawnsOnlyCapturesW() {
  for (auto p = g_board->white[0]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesW(sq, Ycoord(sq) == 6 ? g_pawn_1_moves_w[sq] & (~g_both) : g_pawn_checks_w[sq] & g_pawn_sq);
  }
}

void MgenPawnsOnlyCapturesB() {
  for (auto p = g_board->black[0]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesB(sq, Ycoord(sq) == 1 ? g_pawn_1_moves_b[sq] & (~g_both) : g_pawn_checks_b[sq] & g_pawn_sq);
  }
}

void MgenKnightsW() {
  for (auto p = g_board->white[1]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesW(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenKnightsB() {
  for (auto p = g_board->black[1]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesB(sq, g_knight_moves[sq] & g_good);
  }
}

void MgenBishopsPlusQueensW() {
  for (auto p = g_board->white[2] | g_board->white[4]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesW(sq, BishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenBishopsPlusQueensB() {
  for (auto p = g_board->black[2] | g_board->black[4]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesB(sq, BishopMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensW() {
  for (auto p = g_board->white[3] | g_board->white[4]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
    AddMovesW(sq, RookMagicMoves(sq, g_both) & g_good);
  }
}

void MgenRooksPlusQueensB() {
  for (auto p = g_board->black[3] | g_board->black[4]; p; p = ClearBit(p)) {
    const auto sq = Ctz(p);
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

int MgenTacticalW(struct Board_t *moves) {
  return ChecksB() ? MgenW(moves) : MgenCapturesW(moves);
}

int MgenTacticalB(struct Board_t *moves) {
  return ChecksW() ? MgenB(moves) : MgenCapturesB(moves);
}

void MgenRoot() {
  g_root_n = g_wtm ? MgenW(g_root) : MgenB(g_root);
}

// Evaluate

// Probe Eucalyptus KPK bitbases
inline bool ProbeKPK(const bool wtm) {
  return g_board->white[0] ? eucalyptus::IsDraw(     Ctz(g_board->white[5]),      Ctz(g_board->white[0]),      Ctz(g_board->black[5]),  wtm)
                           : eucalyptus::IsDraw(63 - Ctz(g_board->black[5]), 63 - Ctz(g_board->black[0]), 63 - Ctz(g_board->white[5]), !wtm);
}

// Detect trivial draws really fast
bool EasyDraw(const bool wtm) {
  // R/Q/r/q -> No draw
  if (g_board->white[3] | g_board->white[4] | g_board->black[3] | g_board->black[4])
    return false;

  // N/B/n/b -> Drawish ?
  if (g_board->white[1] | g_board->white[2] | g_board->black[1] | g_board->black[2]) {
    // Pawns -> No draw
    if (g_board->white[0] | g_board->black[0])
      return false;

    // Max 1 N/B per side -> Draw
    if (PopCount(g_board->white[1] | g_board->white[2]) <= 1 && PopCount(g_board->black[1] | g_board->black[2]) <= 1)
      return true;

    // No draw
    return false;
  }

  // No N/B/R/Q/n/b/r/q -> Pawns ?
  const auto pawns_n = PopCount(g_board->white[0] | g_board->black[0]);

  // Check KPK ? / Bare kings ? -> Draw
  return pawns_n == 1 ? ProbeKPK(wtm) : pawns_n == 0;
}

// Classical evaluation. To finish the game or no NNUE
class ClassicalEval {
  private:
    const std::uint64_t white, black, both;
    const bool wtm;
    int white_n, black_n, both_n, wk, bk, wpn, wnn, wbn, wrn, wqn,
        bpn, bnn, bbn, brn, bqn, score, mg, eg, scale_factor;

    int pow2(const int x) const {
      return x * x;
    }

    int flip_y(const int sq) const {
      return sq ^ 56;
    }

    int closer_bonus(const int sq1, const int sq2) const {
      return pow2(7 - std::abs(Xcoord(sq1) - Xcoord(sq2))) + pow2(7 - std::abs(Ycoord(sq1) - Ycoord(sq2)));
    }

    int any_corner_bonus(const int sq) const {
      return std::max({this->closer_bonus(sq, 0),  this->closer_bonus(sq, 7),
                       this->closer_bonus(sq, 56), this->closer_bonus(sq, 63)});
    }

    inline void add(const int a, const int b) {
      this->mg += a;
      this->eg += b;
    }

    inline void add(const int a, const int b, const int k) {
      this->add(a * k, b * k);
    }

    void pesto_w(const int p, const int sq) {
      this->add(+kPesto[p][0][sq], +kPesto[p][1][sq]);
    }

    void pesto_b(const int p, const int sq) {
      this->add(-kPesto[p][0][this->flip_y(sq)], -kPesto[p][1][this->flip_y(sq)]);
    }

    void mobility_w(const int k, const std::uint64_t m) {
      this->score += k * PopCount(m);
    }

    void mobility_b(const int k, const std::uint64_t m) {
      this->score -= k * PopCount(m);
    }

    void pawn_w(const int sq) {
      this->wpn++;
      this->pesto_w(0, sq);
    }

    void pawn_b(const int sq) {
      this->bpn++;
      this->pesto_b(0, sq);
    }

    void knight_w(const int sq) {
      this->wnn++;
      this->pesto_w(1, sq);
      this->mobility_w(2, g_knight_moves[sq] & (~this->white));
    }

    void knight_b(const int sq) {
      this->bnn++;
      this->pesto_b(1, sq);
      this->mobility_b(2, g_knight_moves[sq] & (~this->black));
    }

    void bishop_w(const int sq) {
      this->wbn++;
      this->pesto_w(2, sq);
      this->mobility_w(3, BishopMagicMoves(sq, this->both) & (~this->white));
    }

    void bishop_b(const int sq) {
      this->bbn++;
      this->pesto_b(2, sq);
      this->mobility_b(3, BishopMagicMoves(sq, this->both) & (~this->black));
    }

    void rook_w(const int sq) {
      this->wrn++;
      this->pesto_w(3, sq);
      this->mobility_w(3, RookMagicMoves(sq, this->both) & (~this->white));
    }

    void rook_b(const int sq) {
      this->brn++;
      this->pesto_b(3, sq);
      this->mobility_b(3, RookMagicMoves(sq, this->both) & (~this->black));
    }

    void queen_w(const int sq) {
      this->wqn++;
      this->pesto_w(4, sq);
      this->mobility_w(2, (BishopMagicMoves(sq, this->both) | RookMagicMoves(sq, this->both)) & (~this->white));
    }

    void queen_b(const int sq) {
      this->bqn++;
      this->pesto_b(4, sq);
      this->mobility_b(2, (BishopMagicMoves(sq, this->both) | RookMagicMoves(sq, this->both)) & (~this->black));
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
      for (auto b = this->both; b; b = ClearBit(b))
        this->eval_piece(Ctz(b));

      this->white_n = this->wpn + this->wnn + this->wbn + this->wrn + this->wqn + 1;
      this->black_n = this->bpn + this->bnn + this->bbn + this->brn + this->bqn + 1;
      this->both_n  = this->white_n + this->black_n;
    }

    void bonus_knbk_w() {
      this->score += 2 * this->closer_bonus(this->wk, this->bk);
      this->score += (g_board->white[2] & 0xaa55aa55aa55aa55ULL) ?
                       10 * std::max(this->closer_bonus(0, this->bk), this->closer_bonus(63, this->bk))
                     : 10 * std::max(this->closer_bonus(7, this->bk), this->closer_bonus(56, this->bk));
    }

    void bonus_knbk_b() {
      this->score -= 2 * this->closer_bonus(this->wk, this->bk);
      this->score -= (g_board->black[2] & 0xaa55aa55aa55aa55ULL) ?
                       10 * std::max(this->closer_bonus(0, this->wk), this->closer_bonus(63, this->wk))
                     : 10 * std::max(this->closer_bonus(7, this->wk), this->closer_bonus(56, this->wk));
    }

    void bonus_tempo() {
      this->score += this->wtm ? +kTempo : -kTempo;
    }

    // Force black king in the corner and get closer
    void bonus_mating_w() {
      this->score += 6 * this->any_corner_bonus(this->bk) + 4 * this->closer_bonus(this->wk, this->bk);
    }

    void bonus_mating_b() {
      this->score -= 6 * this->any_corner_bonus(this->wk) + 4 * this->closer_bonus(this->wk, this->bk);
    }

    void bonus_checks() {
      if (     ChecksW()) this->score += kChecks;
      else if (ChecksB()) this->score -= kChecks;
    }

    void bonus_bishop_pair() {
      if (this->wbn >= 2) this->score += kBishopPair;
      if (this->bbn >= 2) this->score -= kBishopPair;
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
      if (     (this->wrn == 2 && this->brn) || (this->wrn && (this->wbn || this->wnn) && (this->bbn || this->bnn)))
        this->bonus_mating_w();
      // KRvKRR / K(NB)vKR(NB)
      else if ((this->brn == 2 && this->wrn) || (this->brn && (this->bbn || this->bnn) && (this->wbn || this->wnn)))
        this->bonus_mating_b();
    }

    // Blind KBPvK ( Not perfect, just a hint ) -> Drawish ?
    void check_blind_bishop_w() {
      if (this->white_n == 3 && this->wbn && this->wpn) {
        const auto wpx   = Xcoord(Ctz(g_board->white[0]));
        const auto color = g_board->white[2] & 0x55aa55aa55aa55aaULL;
        if ((color && wpx == 7) || (!color && wpx == 0))
          this->scale_factor = 2;
      }
    }

    void check_blind_bishop_b() {
      if (this->black_n == 3 && this->bbn && this->bpn) {
        const auto bpx   = Xcoord(Ctz(g_board->black[0]));
        const auto color = g_board->black[2] & 0x55aa55aa55aa55aaULL;
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
      const auto n = Between<int>(2, this->both_n, 32);
      const auto s = (n * this->mg + (32 - n) * this->eg) / 32;
      return (this->score + s) / this->scale_factor;
    }

  public:
    ClassicalEval(const bool wtm2) :
      white(White()), black(Black()), both(this->white | this->black), wtm(wtm2),
      white_n(0), black_n(0), both_n(0), wk(0), bk(0), wpn(0), wnn(0), wbn(0), wrn(0), wqn(0),
      bpn(0), bnn(0), bbn(0), brn(0), bqn(0), score(0), mg(0), eg(0), scale_factor(1) {}

    int evaluate() {
      this->evaluate_pieces();
      this->bonus_tempo();
      this->bonus_checks();
      this->bonus_bishop_pair();
      this->bonus_special();
      return this->calculate_score();
    }
};

class NnueEval {
  private:
    const bool wtm;

    int probe() {
      int pieces[33], squares[33], i = 2;

      for (auto both = Both(); both; both = ClearBit(both)) {
        const auto sq = Ctz(both);
        switch (g_board->pieces[sq]) {
          case +1: case +2: case +3: case +4: case +5:
            pieces[i]    = 7 - g_board->pieces[sq];
            squares[i++] = sq;
            break;
          case -1: case -2: case -3: case -4: case -5:
            pieces[i]    = 13 + g_board->pieces[sq];
            squares[i++] = sq;
            break;
          case +6:
            pieces[0]  = 1;
            squares[0] = sq;
            break;
          case -6:
            pieces[1]  = 7;
            squares[1] = sq;
            break;
        }
      }

      pieces[i] = squares[i] = 0;

      return this->wtm ? +(nnue::nnue_evaluate(0, pieces, squares) + kTempo)
                       : -(nnue::nnue_evaluate(1, pieces, squares) + kTempo);
    }

  public:
    NnueEval(const bool wtm2) : wtm(wtm2) {}

    int evaluate() {
      const auto hash = Hash(wtm);
      auto *entry     = &g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)];
      if (entry->eval_hash == hash) return entry->score;
      entry->eval_hash = hash;
      return entry->score = this->probe();
    }
};

int EvaluateClassical(const bool wtm) {
  ClassicalEval e(wtm);
  return e.evaluate();
}

int EvaluateNNUE(const bool wtm) {
  NnueEval e(wtm);
  return e.evaluate() / 4;
}

int FullEvaluate(const bool wtm) {
  return g_scale[g_board->rule50] * static_cast<float>(g_classical ? EvaluateClassical(wtm) : EvaluateNNUE(wtm));
}

int Evaluate(const bool wtm) {
  return EasyDraw(wtm) ? 0
                       : FullEvaluate(wtm);
}

// Search

void Speak(const int score, const std::uint64_t ms) {
  std::cout << "info depth " << std::min(g_max_depth, g_depth + 1)
            << " nodes "     << g_nodes
            << " time "      << ms
            << " nps "       << Nps(g_nodes, ms)
            << " score cp "  << ((g_wtm ? +1 : -1) * (std::abs(score) == kInf ? score / 100 : score))
            << " pv "        << MoveName(g_root) << std::endl;
}

// g_r50_positions.pop() must contain hash !
bool Draw(const bool wtm) {
  if (g_board->rule50 > 100 || EasyDraw(wtm)) return true;

  const auto hash = g_r50_positions[g_board->rule50];

  if (poseidon::IsDraw(hash)) return true;

  for (auto i = g_board->rule50 - 2; i >= 0; i -= 2)
    if (g_r50_positions[i] == hash)
      return true;

  return false;
}

// 1. Check against standpat to see whether we are better -> Done
// 2. Iterate deeper
int QSearchW(int alpha, const int beta, const int depth) {
  g_nodes++;

  if (g_stop_search) return 0;
  alpha = std::max(alpha, Evaluate(true));
  if (depth <= 0 || alpha >= beta) return alpha;

  struct Board_t moves[64];
  const auto moves_n = MgenTacticalW(moves);

  SortAll();

  for (auto i = 0; i < moves_n; i++) {
    g_board = moves + i;
    if ((alpha = std::max(alpha, QSearchB(alpha, beta, depth - 1))) >= beta)
      return alpha;
  }

  return alpha;
}

int QSearchB(const int alpha, int beta, const int depth) {
  g_nodes++;

  if (g_stop_search) return 0;
  beta = std::min(beta, Evaluate(false));
  if (depth <= 0 || alpha >= beta) return beta;

  struct Board_t moves[64];
  const auto moves_n = MgenTacticalB(moves);

  SortAll();

  for (auto i = 0; i < moves_n; i++) {
    g_board = moves + i;
    if (alpha >= (beta = std::min(beta, QSearchW(alpha, beta, depth - 1))))
      return beta;
  }

  return beta;
}

// Update hashtable sorting algorithm
void UpdateSort(struct Hash_t *entry,
                const enum MoveType type,
                const std::uint64_t hash,
                const std::uint8_t index) {
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
  struct Board_t moves[kMaxMoves];
  const auto hash    = g_r50_positions[g_board->rule50];
  const auto checks  = ChecksB();
  const auto moves_n = MgenW(moves);

  if (!moves_n) return checks ? -kInf : 0;
  if (moves_n == 1 || (depth == 1 && checks)) depth++;

  const auto ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  auto *entry       = &g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)];
  SortByScore(entry, hash);

  for (auto i = 0; i < moves_n; i++) {
    g_board = moves + i;
    g_is_pv = i <= 1 && !moves[i].score;

    if (ok_lmr && i >= 1 && !g_board->score && !ChecksW()) {
      if (SearchB(alpha, beta, depth - 2 - g_lmr[depth][i], ply + 1) <= alpha)
        continue;
      g_board = moves + i;
    }

    const auto score = SearchB(alpha, beta, depth - 1, ply + 1);
    if (score > alpha) {
      if ((alpha = score) >= beta) {
        UpdateSort(entry, MoveType::kKiller, hash, moves[i].index);
        return alpha;
      }
      UpdateSort(entry, moves[i].score ? MoveType::kGood : MoveType::kQuiet, hash, moves[i].index);
    }
  }

  return alpha;
}

int SearchMovesB(const int alpha, int beta, int depth, const int ply) {
  struct Board_t moves[kMaxMoves];
  const auto hash    = g_r50_positions[g_board->rule50];
  const auto checks  = ChecksW();
  const auto moves_n = MgenB(moves);

  if (!moves_n) return checks ? +kInf : 0;
  if (moves_n == 1 || (depth == 1 && checks)) depth++;

  const auto ok_lmr = moves_n >= 5 && depth >= 2 && !checks;
  auto *entry       = &g_hash[static_cast<std::uint32_t>(hash % g_hash_entries)];
  SortByScore(entry, hash);

  for (auto i = 0; i < moves_n; i++) {
    g_board = moves + i;
    g_is_pv = i <= 1 && !moves[i].score;

    if (ok_lmr && i >= 1 && !g_board->score && !ChecksB()) {
      if (SearchW(alpha, beta, depth - 2 - g_lmr[depth][i], ply + 1) >= beta)
        continue;
      g_board = moves + i;
    }

    const auto score = SearchW(alpha, beta, depth - 1, ply + 1);
    if (score < beta) {
      if (alpha >= (beta = score)) {
        UpdateSort(entry, MoveType::kKiller, hash, moves[i].index);
        return beta;
      }
      UpdateSort(entry, moves[i].score ? MoveType::kGood : MoveType::kQuiet, hash, moves[i].index);
    }
  }

  return beta;
}

// If we do nothing and we are still better -> Done
bool TryNullMoveW(int *alpha, const int beta, const int depth, const int ply) {
  // No nullmove on the path ?
  if (   (!g_nullmove_active)
      // Not pv ?
      && (!g_is_pv)
      // Enough depth ?
      && (depth >= 3)
      // Non pawn material or at least 2 pawns ( Zugzwang ... ) ?
      && ((g_board->white[1] | g_board->white[2] | g_board->white[3] | g_board->white[4])
        || PopCount(g_board->white[0]) >= 2)
      // Not under checks ?
      && (!ChecksB())
      // Looks good ?
      && (Evaluate(true) >= beta)) {
    const auto ep     = g_board->epsq;
    auto *tmp         = g_board;
    g_board->epsq     = -1;

    g_nullmove_active = true;
    const auto score  = SearchB(*alpha, beta, depth - (depth / 4 + 3), ply);
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
  if (   (!g_nullmove_active)
      && (!g_is_pv)
      && (depth >= 3)
      && ((g_board->black[1] | g_board->black[2] | g_board->black[3] | g_board->black[4])
        || PopCount(g_board->black[0]) >= 2)
      && (!ChecksW())
      && (alpha >= Evaluate(false))) {
    const auto ep     = g_board->epsq;
    auto *tmp         = g_board;
    g_board->epsq     = -1;

    g_nullmove_active = true;
    const auto score  = SearchW(alpha, *beta, depth - (depth / 4 + 3), ply);
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
  g_nodes++;

  if (g_stop_search) return 0;
  if (depth <= 0 || ply >= kMaxDepth) return QSearchW(alpha, beta, g_qs_depth);

  const auto rule50 = g_board->rule50;
  const auto tmp    = g_r50_positions[rule50];

  if (TryNullMoveW(&alpha, beta, depth, ply)) return alpha;

  g_r50_positions[rule50] = Hash(true);
  alpha                   = Draw(true) ? 0 : SearchMovesW(alpha, beta, depth, ply);
  g_r50_positions[rule50] = tmp;

  return alpha;
}

int SearchB(const int alpha, int beta, const int depth, const int ply) {
  g_nodes++;

  if (g_stop_search) return 0;
  if (depth <= 0 || ply >= kMaxDepth) return QSearchB(alpha, beta, g_qs_depth);

  const auto rule50 = g_board->rule50;
  const auto tmp    = g_r50_positions[rule50];

  if (TryNullMoveB(alpha, &beta, depth, ply)) return beta;

  g_r50_positions[rule50] = Hash(false);
  beta                    = Draw(false) ? 0 : SearchMovesB(alpha, beta, depth, ply);
  g_r50_positions[rule50] = tmp;

  return beta;
}

// Root search
int BestW() {
  auto score  = 0;
  auto best_i = 0;
  auto alpha  = -kInf;

  for (auto i = 0; i < g_root_n && !g_stop_search; i++) {
    g_board = g_root + i;
    // 1 + 2 moves too good and not tactical -> pv
    g_is_pv = i <= 1 && !g_root[i].score;

    if (g_depth >= 1 && i >= 1) {
      if ((score = SearchB(alpha, alpha + 1, g_depth, 0)) > alpha) {
        g_board = g_root + i;
        score   = SearchB(alpha, kInf, g_depth, 0);
      }
    } else {
      score = SearchB(alpha, kInf, g_depth, 0);
    }

    if (score > alpha) {
      // Skip underpromos unless really good ( 3+ pawns )
      if (IsUnderpromo(g_root + i) && ((score + (3 * 100)) < alpha))
        continue;
      alpha  = score;
      best_i = i;
    }
  }

  if (g_stop_search) return g_best_score;

  SortRoot(best_i);
  return alpha;
}

int BestB() {
  auto score  = 0;
  auto best_i = 0;
  auto beta   = +kInf;

  for (auto i = 0; i < g_root_n && !g_stop_search; i++) {
    g_board = g_root + i;
    g_is_pv = i <= 1 && !g_root[i].score;

    if (g_depth >= 1 && i >= 1) {
      if ((score = SearchW(beta - 1, beta, g_depth, 0)) < beta) {
        g_board = g_root + i;
        score   = SearchW(-kInf, beta, g_depth, 0);
      }
    } else {
      score = SearchW(-kInf, beta, g_depth, 0);
    }

    if (score < beta) {
      if (IsUnderpromo(g_root + i) && ((score - (3 * 100)) > beta))
        continue;
      beta   = score;
      best_i = i;
    }
  }

  if (g_stop_search) return g_best_score;

  SortRoot(best_i);
  return beta;
}

// Material detection for classical activation
class Material {
  private:
    const int white_n, black_n, both_n;

  public:
    Material() :
      white_n(PopCount(White())),
      black_n(PopCount(Black())),
      both_n(this->white_n + this->black_n) {}

    // KRRvKR / KRvKRR / KRRRvK / KvKRRR ?
    bool is_rook_ending() const {
      return this->both_n == 5 && PopCount(g_board->white[3] | g_board->black[3]) == 3;
    }

    // Vs king + (PNBRQ) ?
    bool is_easy() const {
      return g_wtm ? this->black_n <= 2 : this->white_n <= 2;
    }

    // 7 pieces or less both side -> Endgame
    bool is_endgame() const {
      return g_wtm ? this->black_n <= 7 : this->white_n <= 7;
    }
};

void Activation(const Material &m) {
  // No NNUE or Easy or Rook ending -> Classical eval
  if ((!g_nnue_exist) || m.is_easy() || m.is_rook_ending())
    g_classical = true;
}

void ThinkReset(const int ms) {
  g_stop_search = g_classical = g_nullmove_active = g_is_pv = false;
  g_qs_depth = g_best_score = g_nodes = g_depth = 0;
  g_stop_search_time = Now() + std::max(0, ms);
}

bool ThinkRandomMove() {
  if (g_level) return false;

  const auto i = Random(0, g_root_n - 1);
  if (i) Swap(g_root, g_root + i);

  return true;
}

// Play the book move from root list
bool FindBookMove(const int from, const int to, const int type) {
  for (auto i = 0; i < g_root_n; i++)
    if (g_root[i].from == from && g_root[i].to == to) {
      if (type && g_root[i].type != type) continue;
      SortRoot(i);
      return true;
    }

  return false;
}

// Probe PolyGlot book
bool ProbeBook() {
  const auto move = g_book.setup(g_board->pieces, Both(), g_board->castle, g_board->epsq, g_wtm)
                          .probe(Random(8) == 1);

  if (!move) return false;

  const auto from = 8 * ((move >> 9) & 0x7) + ((move >> 6) & 0x7);
  const auto to   = 8 * ((move >> 3) & 0x7) + ((move >> 0) & 0x7);

  auto type = 0;
  for (const auto i : {0, 1, 2, 3}) // Promos
    if (move & (0x1 << (12 + i))) {
      type = 5 + i;
      break;
    }

  return FindBookMove(from, to, type);
}

void UserLevel() {
  if ((g_level >= 1 && g_level <= 9) && (Random(0, 9) >= g_level))
    Swap(g_root, g_root + 1);
}

bool FastMove(const int ms) {
  if (   (g_root_n <= 1)   // Only move
      || (ms <= 1)         // Hurry up !
      || ThinkRandomMove() // Level 0
      // Make sure we have at least 100ms for book lookup
      || (g_book_exist && ms >= 100 && !g_analyzing && ProbeBook())) {
    Speak(g_last_eval, 0);
    return true;
  }

  return false;
}

void SearchRootMoves(const bool is_eg) {
  int good       = 0;
  const auto now = Now();
  const std::function<int()> best = std::bind(g_wtm ? BestW : BestB);

  for (; (std::abs(g_best_score) != kInf) && (g_depth < g_max_depth) && (!g_stop_search); g_depth++) {
    g_best_score = best();

    // Switch to classical only when the game is decided ( 4+ pawns ) !
    if (!g_classical && is_eg && std::abs(g_best_score) > (4 * 100) && ++good >= 7)
      g_classical = true;

    Speak(g_best_score, Now() - now);
    g_qs_depth = std::min(g_qs_depth + 2, 12);
  }

  // Also stop the time thread
  g_stop_search = true;

  UserLevel();
  g_last_eval = g_best_score;
  Speak(g_best_score, Now() - now);
}

bool UserStop() {
  if (!g_analyzing || !InputAvailable()) return false;
  Input();
  return Token("stop");
}

bool TimeCheckSearch() {
  return Now() >= g_stop_search_time || UserStop();
}

// Check time every 5ms
void CheckTime() {
  while (!g_stop_search) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5)); // zZzZz ...
    if (g_stop_search) return;
    g_stop_search = TimeCheckSearch();
  }
}

void Think(const int ms) {
  auto *tmp = g_board;
  const Material m;

  ThinkReset(ms);
  Activation(m);
  MgenRoot();

  if (FastMove(ms)) return;

  EvalRootMoves();
  SortAll();

  // Time check thread
  std::thread th_time([]{ CheckTime(); });

  // Underpromos are almost useless for gameplay
  // Disable if you need "full" analysis
  g_underpromos = false;
  SearchRootMoves(m.is_endgame());
  g_underpromos = true;

  // Wait for the time thread to finish
  th_time.join();

  g_board = tmp;
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
  for (auto i = 0; i < g_root_n; i++)
    if (move == MoveName(g_root + i)) {
      Make(i);
      return;
    }

  Assert(false, "Error #3: Bad move !");
}

void TakeSpecialFen() {
  TokenPop(); // fen

  std::string fen = "";
  for (; TokenOk() && !Token("moves", 0); TokenPop())
    fen += TokenCurrent() + " ";

  Fen(fen);
}

void UciFen() {
  Token("startpos") ? Fen(kStartPos)
                    : TakeSpecialFen();
}

void UciMoves() {
  while (TokenOk()) {
    MakeMove();
    TokenPop();
  }
}

void UciPosition() {
  UciFen();
  if (Token("moves"))
    UciMoves();
}

void UciSetoption() {
  if (Peek("name") && Peek("UCI_Chess960", 1) && Peek("value", 2)) {
    g_chess960 = Peek("true", 3);
    TokenPop(4);
  } else if (Peek("name") && Peek("Level", 1) && Peek("value", 2)) {
    g_level = Between<int>(0, TokenNumber(3), 10);
    TokenPop(4);
  } else if (Peek("name") && Peek("Hash", 1) && Peek("value", 2)) {
    g_hash_mb = TokenNumber(3);
    SetupHashtable();
    TokenPop(4);
  } else if (Peek("name") && Peek("MoveOverhead", 1) && Peek("value", 2)) {
    g_move_overhead = Between<int>(0, TokenNumber(3), 5000);
    TokenPop(4);
  } else if (Peek("name") && Peek("EvalFile", 1) && Peek("value", 2)) {
    g_eval_file = TokenCurrent(3);
    SetupNNUE();
    TokenPop(4);
  } else if (Peek("name") && Peek("BookFile", 1) && Peek("value", 2)) {
    g_book_file = TokenCurrent(3);
    SetupBook();
    TokenPop(4);
  }
}

void PrintBestMove() {
  std::cout << "bestmove " << (g_root_n <= 0 ? "0000" : MoveName(g_root)) << std::endl;
}

void UciGoAnalyze() {
  g_analyzing = true;
  Think(kInf);
  g_analyzing = false;
  PrintBestMove();
}

void UciGoMovetime() {
  Think(TokenNumber());
  TokenPop();
  PrintBestMove();
}

void UciGoDepth() {
  g_max_depth = Between<int>(1, TokenNumber(), kMaxDepth);
  Think(kInf);
  g_max_depth = kMaxDepth;
  TokenPop();
  PrintBestMove();
}

// Make sure we never lose on time
void UciGo() {
  int wtime = 0, btime = 0, winc = 0, binc = 0, mtg = 26;

  for (; TokenOk(); TokenPop()) {
    if (     Token("infinite"))  {UciGoAnalyze(); return;}
    else if (Token("wtime"))     {wtime = std::max(0, TokenNumber() - g_move_overhead);}
    else if (Token("btime"))     {btime = std::max(0, TokenNumber() - g_move_overhead);}
    else if (Token("winc"))      {winc  = std::max(0, TokenNumber() - g_move_overhead / 2);}
    else if (Token("binc"))      {binc  = std::max(0, TokenNumber() - g_move_overhead / 2);}
    else if (Token("movestogo")) {mtg   = std::max(1, TokenNumber());}
    else if (Token("movetime"))  {UciGoMovetime(); return;}
    else if (Token("depth"))     {UciGoDepth(); return;}
  }

  Think(g_wtm ? wtime / mtg + winc : btime / mtg + binc);
  PrintBestMove();
}

void UciUci() {
  std::cout
    << "id name " << kVersion << "\n"
    << "id author Toni Helminen" << "\n"
    << "option name UCI_Chess960 type check default " << (g_chess960 ? "true" : "false") << "\n"
    << "option name Level type spin default "         << g_level         << " min 0 max 10" << "\n"
    << "option name Hash type spin default "          << g_hash_mb       << " min 4 max 1048576" << "\n"
    << "option name MoveOverhead type spin default "  << g_move_overhead << " min 0 max 5000" << "\n"
    << "option name EvalFile type string default "    << g_eval_file     << "\n"
    << "option name BookFile type string default "    << g_book_file     << "\n"
    << "uciok" << std::endl;
}

bool UciCommands() {
  if (!TokenOk()) return true;

  if (Token("position"))        UciPosition();
  else if (Token("go"))         UciGo();
  else if (Token("ucinewgame")) g_last_eval = 0;
  else if (Token("isready"))    std::cout << "readyok" << std::endl;
  else if (Token("setoption"))  UciSetoption();
  else if (Token("uci"))        UciUci();
  else if (Token("quit"))       return false;

  // Ignore the rest
  for (; TokenOk(); TokenPop());

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

  for (auto i = 0; i < 64; i++)
    if (moves & Bit(i))
      good[total++] = i;

  const int popn = PopCount(moves);
  for (auto i = 0; i < popn; i++)
    if ((1 << i) & index)
      permutations |= Bit(good[i]);

  return permutations & moves;
}

std::uint64_t MakeSliderMagicMoves(const int *slider_vectors, const int sq, const std::uint64_t moves) {
  std::uint64_t possible_moves = 0x0ULL;
  const auto x_pos = Xcoord(sq);
  const auto y_pos = Ycoord(sq);

  for (auto i = 0; i < 4; i++)
    for (auto j = 1; j < 8; j++) {
      const auto x = x_pos + j * slider_vectors[2 * i];
      const auto y = y_pos + j * slider_vectors[2 * i + 1];
      if (!OnBoard(x, y)) break;
      const auto tmp  = Bit(8 * y + x);
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
  std::uint64_t moves = 0x0ULL;
  const auto x_pos    = Xcoord(sq);
  const auto y_pos    = Ycoord(sq);

  for (auto i = 0; i < 4; i++) {
    const auto dx     = slider_vectors[2 * i];
    const auto dy     = slider_vectors[2 * i + 1];
    std::uint64_t tmp = 0x0ULL;
    for (auto j = 1; j < 8; j++) {
      const auto x = x_pos + j * dx;
      const auto y = y_pos + j * dy;
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
  std::uint64_t moves = 0x0ULL;
  const auto x_pos    = Xcoord(sq);
  const auto y_pos    = Ycoord(sq);

  for (auto i = 0; i < len; i++) {
    const auto x = x_pos + jump_vectors[2 * i];
    const auto y = y_pos + dy * jump_vectors[2 * i + 1];
    if (OnBoard(x, y)) moves |= Bit(8 * y + x);
  }

  return moves;
}

void InitJumpMoves() {
  constexpr int pawn_check_vectors[2 * 2] = {-1, +1, +1, +1}, pawn_1_vectors[1 * 2] = {0, +1};

  for (auto i = 0; i < 64; i++) {
    g_king_moves[i]     = MakeJumpMoves(i, 8, +1, kKingVectors);
    g_knight_moves[i]   = MakeJumpMoves(i, 8, +1, kKnightVectors);
    g_pawn_checks_w[i]  = MakeJumpMoves(i, 2, +1, pawn_check_vectors);
    g_pawn_checks_b[i]  = MakeJumpMoves(i, 2, -1, pawn_check_vectors);
    g_pawn_1_moves_w[i] = MakeJumpMoves(i, 1, +1, pawn_1_vectors);
    g_pawn_1_moves_b[i] = MakeJumpMoves(i, 1, -1, pawn_1_vectors);
  }

  for (auto i = 0; i < 8; i++) {
    g_pawn_2_moves_w[ 8 + i] = MakeJumpMoves( 8 + i, 1, +1, pawn_1_vectors) | MakeJumpMoves( 8 + i, 1, +2, pawn_1_vectors);
    g_pawn_2_moves_b[48 + i] = MakeJumpMoves(48 + i, 1, -1, pawn_1_vectors) | MakeJumpMoves(48 + i, 1, -2, pawn_1_vectors);
  }
}

void InitZobrist() {
  for (auto i = 0; i < 13; i++) for (auto j = 0; j < 64; j++) g_zobrist_board[i][j] = Random8x64();
  for (auto i = 0; i < 64; i++) g_zobrist_ep[i]     = Random8x64();
  for (auto i = 0; i < 16; i++) g_zobrist_castle[i] = Random8x64();
  for (auto i = 0; i <  2; i++) g_zobrist_wtm[i]    = Random8x64();
}

// Shuffle period 30 plies then scale
void InitScale() {
  for (auto i = 0; i < 128; i++)
    g_scale[i] = i < 30 ? 1.0f : (1.0f - ((static_cast<float>(i - 30)) / 110.0f));
}

void InitLMR() {
  for (auto d = 0; d < kMaxDepth; d++)
    for (auto m = 0; m < kMaxMoves; m++)
      g_lmr[d][m] = Between<int>(1, static_cast<int>(std::log(d) * std::log(m) * 0.25), 4);
}

[[maybe_unused]] void Debug(const std::string &fen) {
  Fen(fen);
  Think(10000);
  std::exit(EXIT_SUCCESS);
}

void Init() {
  g_seed += std::time(nullptr);
  InitBishopMagics();
  InitRookMagics();
  InitZobrist();
  InitSliderMoves();
  InitJumpMoves();
  InitScale();
  InitLMR();
  SetupHashtable();
  SetupNNUE();
  SetupBook();
  Fen(kStartPos);
}

void Bench() {
  const auto now      = Now();
  std::uint64_t nodes = 0x0ULL;

  for (const auto &fen : kBench) {
    std::cout << "[ " << fen << " ]" << std::endl;
    Fen(fen);
    Think(10000);
    nodes += g_nodes;
    std::cout << std::endl;
  }

  std::cout << "===\n\n" << "NPS: " << Nps(nodes, Now() - now) << std::endl;
}

void PrintVersion() {
  std::cout << kVersion << " by Toni Helminen" << std::endl;
}

void UciLoop() {
  PrintVersion();
  while (Uci());
}

}
