# Smallbrain

## Compile

Compile it using the Makefile in ./src or use the VS Solution (which is only maintained from time to time)<br>
```
make
.\smallbrain.exe bench
```
compare the Bench with the Bench in the commit messages, they should be <br>
the same.

## Elo
CCRL 40/15

```
  Name                  Elo      +       -
  Smallbrain 2.0 64-bit	2268	+30	−30
  Smallbrain 1.1 64-bit	2223	+29	−30
```
 CCRL Blitz 2'+1"
```
  Name                  Elo      +       -
  Smallbrain 3.0 64-bit	2925	+25	−25
  Smallbrain 1.1 64-bit	2174	+21	−21
```

I estimate version 4.0 to be above 3000 elo.

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
  * TT move ordering
  * Killer moves
  * History heuristic
  * MVV-LVA move ordering
  * Null-move pruning
  * Razoring
  * Late-move reduction
  * See Algorithm
* Evaluation
  * NNUE 768 Input -> 256 hidden neurons -> 1 output
