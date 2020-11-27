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
Mayhem is a Linux UCI Chess960 engine. Mayhem is written in C++14. Derivated from Sapeli. 
Credits: NNUE library: Dr. Shawul. EvalFile+PolyGlot: The SF team.
Mayhem requires an EvalFile to properly evaluate.
Since v1.5 Mayhem supports polyglot books!
Estimated strength is ~2800+ Elo in blitz. Measured against Fruit 2.1. By independent testers: ~2790 Elo (v0.50).
`make` should build a good binary. If not then remove flags until it does.
`--bench` Should yield at least 1 Mnps for decent level.
`Hash` UCI option should be at least 256MB. Because NNUE evaluation is slow. Used for sorting too.
Happy hacking!
