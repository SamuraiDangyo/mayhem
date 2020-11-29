```
╋╋╋╋╋╋╋╋╋╋┏┓
╋╋╋╋╋╋╋╋╋╋┃┃
┏┓┏┳━━┳┓╋┏┫┗━┳━━┳┓┏┓
┃┗┛┃┏┓┃┃╋┃┃┏┓┃┃━┫┗┛┃
┃┃┃┃┏┓┃┗━┛┃┃┃┃┃━┫┃┃┃
┗┻┻┻┛┗┻━┓┏┻┛┗┻━━┻┻┻┛
╋╋╋╋╋╋┏━┛┃
╋╋╋╋╋╋┗━━┛
```

### Overview
Mayhem is a Linux UCI Chess960 engine. Mayhem is written in C++14. Derivated from Sapeli. Licensed under GPLv3.
Credits: NNUE library: Dr. Shawul. EvalFile + PolyGlot: Stockfish.
Mayhem requires EvalFile (NNUE) and BookFile (PolyGlot) for maximum strength. Works w/o them too. But much weaker.
Estimated strength is ~2800+ Elo in Blitz (Against Fruit 2.1). By independent testers: ~2790 Elo (v0.50).
`make` should build a good binary. If not then remove nnue - flags until it does. 1Mnps+ is needed for decent level.
Default settings are the best. Increase Hash/MoveOverhead in longer games.
Happy hacking!
