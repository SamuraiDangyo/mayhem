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

## Overview
Mayhem, Linux UCI Chess960 program.
Mayhem is Sapeli written in C++14 with SF NNUE evaluation (Credits to the SF team).
Mayhem requires an EvalFile to properly evaluate.
Estimated strength is 3000+ Elo in Blitz time controls. Measured against stable Fruit 2.1 and Crafty 25.6 engines.
`make` should build a good binary. If not then remove flags until it does.
`--bench` Should yield at least 1 Mnps for decent level.
`Hash` UCI option should be at least 256MB. Because NNUE evaluation is slow. Used for sorting too.
Happy hacking!
