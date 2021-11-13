/*
NNUE lib
Copyright (C) 2020-2021 Daniel Shawul

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

#pragma once

extern "C" {

#include <stdbool.h>
#include <stdalign.h>

// misc.hpp start

#include <inttypes.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/mman.h>
#endif

#if defined (__GNUC__)
#   define INLINE  __inline __attribute__((always_inline))
#elif defined (_WIN32)
#   define INLINE  __forceinline
#else
#   define INLINE  __inline
#endif

#ifdef _WIN32

typedef HANDLE FD;
#define FD_ERR INVALID_HANDLE_VALUE
typedef HANDLE map_t;

#else /* Unix */

typedef int FD;
#define FD_ERR -1
typedef size_t map_t;

#endif

FD open_file(const char *name);
void close_file(FD fd);
size_t file_size(FD fd);
const void *map_file(FD fd, map_t *map);
void unmap_file(const void *data, map_t map);

INLINE uint32_t readu_le_u32(const void *p)
{
  const uint8_t *q = (const uint8_t*) p;
  return q[0] | (q[1] << 8) | (q[2] << 16) | (q[3] << 24);
}

INLINE uint16_t readu_le_u16(const void *p)
{
  const uint8_t *q = (const uint8_t*) p;
  return q[0] | (q[1] << 8);
}

// misc.hpp end

#ifdef __cplusplus
#   define EXTERNC extern "C"
#else
#   define EXTERNC
#endif
#if defined (_WIN32)
#   define _CDECL __cdecl
#ifdef DLL_EXPORT
#   define DLLExport EXTERNC __declspec(dllexport)
#else
#   define DLLExport EXTERNC __declspec(dllimport)
#endif
#else
#   define _CDECL
#   define DLLExport EXTERNC
#endif

/*pieces*/
enum colors {
    white,black
};
enum chessmen {
    blank,king,queen,rook,bishop,knight,pawn
};

#define COMBINE(c,x)     ((x) + (c) * 6)

/*nnue data*/
typedef struct {
  alignas(64) int16_t accumulation[2][256];
  bool computedAccumulation;
} Accumulator;

/*position*/
typedef struct Position {
  int player;
  int* pieces;
  int* squares;
  Accumulator accumulator;
} Position;

int nnue_evaluate_pos(Position* pos);

/**
* Load NNUE file
*/

bool nnue_init(
  const char * evalFile             /** Path to NNUE file */
);

/**
* Evaluation subroutine suitable for chess engines.
* -------------------------------------------------
* Piece codes are
*     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
*     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12,
* Square are
*     A1=0, B1=1 ... H8=63
* Input format:
*     piece[0] is white king, square[0] is its location
*     piece[1] is black king, square[1] is its location
*     ..
*     piece[x], square[x] can be in any order
*     ..
*     piece[n+1] is set to 0 to represent end of array
*/
int nnue_evaluate(
  int player,                       /** Side to move */
  int* pieces,                      /** Array of pieces */
  int* squares                      /** Corresponding array of squares the piece stand on */
);

}
