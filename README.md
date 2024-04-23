# Mayhem

Linux UCI Chess960 engine.
Written in C++20 language.
Licensed under the GPLv3.
Optimized for speed and simplicity.
Programmed in self-documenting style.

Mayhem uses EvalFile and BookFile for maximum strength.
Works well w/o them of course.

Install: `make all install clean`
Build:   `make -j`
`> perft / bench` commands should be used to verify the program.

If needed adjust _MoveOverhead_ and _Hash_ values accordingly.
See `Makefile` and `mayhem.hpp` for more information.
Credit for _PolyGlot_ and _NNUE_ stuff.

Happy hacking !
