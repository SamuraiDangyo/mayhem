# Mayhem

Linux UCI Chess960 engine. Written in C++14 language. 
Derivated from Sapeli. Licensed under the GPLv3.

Credits: NNUE library: Dr. Shawul. 
EvalFile + PolyGlot code: Stockfish.

Mayhem requires EvalFile (NNUE) and BookFile (PolyGlot) for maximum strength. Works w/o them too.
Estimated strength ~2800+ Elo in Blitz (vs Fruit 2.1). By testers: ~2790 Elo (v0.50).

`make` should build a good binary. If not then remove *nnue - flags* until it does.
Default settings are the best. If needed increase Hash/MoveOverhead in longer games.

Happy hacking!
