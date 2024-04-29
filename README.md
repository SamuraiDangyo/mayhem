# Mayhem

Linux UCI Chess960 engine.
Written in C++20 language.
Licensed under the GPLv3.
Optimized for speed and simplicity.
Programmed in self-documenting style.

Mayhem uses EvalFile and BookFile for maximum strength.
Works well w/o them of course.

Strength ( Blitz ): *HCE* ~2300 _Elo_ and *NNUE* ~3000 _Elo_.

Simple `make all strip` should build a good binary.
`perft/bench/p` commands should be used to verify the program.
See `Makefile` and `mayhem.hpp` for more information.

If needed adjust _MoveOverhead_ and _Hash_ values accordingly.
Credit for _PolyGlot_ and _NNUE_ stuff.

Happy hacking !
