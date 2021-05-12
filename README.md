# Mayhem

Linux UCI Chess960 engine.
Written in C++17 language.
Licensed under the GPLv3.
Optimized for speed and simplicity.

Mayhem uses EvalFile ( NNUE ) and BookFile ( PolyGlot ) for maximum strength.
Works well w/o them of course.

Strength in Blitz: Classical mode ( ~2300 Elo ) / NNUE mode ( ~3000 Elo ).

Simple `make` should build a good binary.
See `Makefile` for more information.
Try `./mayhem bench` for testing your build's speed.

If needed adjust _MoveOverhead / Hash_ values accordingly.
Credit for _polyglotbook.cpp and nnue.cpp_.

Happy hacking !
