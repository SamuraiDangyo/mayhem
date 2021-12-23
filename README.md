# Mayhem

Linux UCI Chess960 engine.
Written in C++17 language.
Licensed under the GPLv3.
Optimized for speed and simplicity.

Mayhem uses EvalFile ( NNUE ) and BookFile ( PolyGlot ) for maximum strength.
Works well w/o them of course.

Strength ( Blitz ): *HCE* _~2300 Elo_ / *NNUE* _~3000 Elo_.

Simple `make` should build a good binary.
See `Makefile` for more information.
`perft/bench/p` commands should be used to verify/test speed of the program.

If needed adjust _MoveOverhead / Hash_ values accordingly.
Credit for _polyglotbook.cpp and nnue.cpp_.

Happy hacking !
