# Smallbrain

## Compile

Compile it using the Makefile in ./src or use the VS Solution (is only maintained from time to time)<br>
```
make
.\smallbrain.exe bench
```
compare the Bench with the Bench in the commit messages, they should be <br>
the same, unless I screwed up.

## Features
* Engine
  * Bitboard representation
  * Zobrist hashing
* Search
  * Negamax framework
  * Aspiration Window
  * Alpha-Beta pruning fail-soft
  * PVS
  * Quiescence search
  * Repetition detection
  * Killer moves
  * History heuristic
  * MVV-LVA move ordering
  * Null-move pruning
  * Razoring
  * Late-move reduction
* Evaluation
  * Material evaluation
  * Piece square table

