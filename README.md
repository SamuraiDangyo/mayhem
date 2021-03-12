# Mayhem

Linux UCI Chess960 engine.
Written in C++17 language.
Licensed under the GPLv3.

Credits: SF ( EvalFile + PolyGlot ) and Dr. Shawul ( NNUE lib ).

Mayhem requires EvalFile (NNUE) and BookFile (PolyGlot) for maximum strength.
Works w/o them too.

Estimated strength ~2800 Elo in Blitz.

`make` should build a good binary.
If not then remove *nnue - flags* until it does.

Default settings are the best.
If needed increase Hash/MoveOverhead in longer games.

Happy hacking!
