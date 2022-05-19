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

19/05/2020 (DD/MM/YY) <br>
SHA: cc5236ec07f36e1660d1030776a10340b99fcb43<br>
TC: 40/240 (4 minutes) <br>

    # PLAYER         : RATING  ERROR   POINTS  PLAYED    (%)
    1 Smallbrain     : 2225.4   94.0     36.5      50   73.0%
    2 Drofa 1.0.0    : 2051.0   ----     13.5      50   27.0%

    White advantage = 8.89 +/- 48.64
    Draw rate (equal opponents) = 30.98 % +/- 8.03
