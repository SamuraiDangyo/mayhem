# Mayhem

Linux UCI Chess960 engine.
Written in C++17 language.
Licensed under the GPLv3.
Optimized for speed and simplicity.

Mayhem uses EvalFile ( NNUE ) and BookFile ( PolyGlot ) for maximum strength.
Works well w/o them of course.

Strength in Blitz: Classical mode ( ~2300 Elo ) and NNUE mode ( ~3000 Elo ).

Simple `make` should build a good binary.
If not then modify *compiler flags*.

If needed adjust _MoveOverhead / Hash_ values.
See `Makefile / mayhem.hpp` for more details.

Credits: polyglotbook.cpp and nnue.cpp.

Happy hacking !
