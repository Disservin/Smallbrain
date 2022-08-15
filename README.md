# Smallbrain

## History

During the pandemic I rediscovered chess and played a lot of games with my friends.<br>
Then I started to program my first engine [python-smallbrain](https://github.com/Disservin/python-smallbrain) in python, with the help of python-chess.<br>

I quickly realized how slow python is for chess engine programming, so I started to learn C++.<br>
My first try was [cppsmallbrain](https://github.com/Disservin/cppsmallbrain), though after some time I found the code very buggy and ugly.<br>
So I started [Smallbrain](https://github.com/Disservin/Smallbrain) from scratch, during that time, I also joined Stockfish development. <br>

After some time I began implementing a NNUE into Smallbrain, with the help of Luecx from [Koivisto](https://github.com/Luecx/Koivisto).<br>
As of now Smallbrain has a NNUE trained on data given to me by Luecx, while using his [trainer](https://github.com/Luecx/CudAD).
In the future I plan to generate my own data.

## Compile

Compile it using the Makefile in ./src or use the VS Solution (which is only maintained from time to time)<br>
```
make
.\smallbrain_5.0.exe bench
```
compare the Bench with the Bench in the commit messages,
they should be the same.

## UCI settings
* Hash<br>
  The size of the hash table in MB. 
  
* Threads<br>
  The number of threads used for search. 
  
* EvalFile<br>
  The neural net used for the evaluation,<br>
  currently only default.nnue exist.
  
## Engine specific commands
* go perft *depth*<br>
  calculates perft from a set position up to *depth*.
  
* print<br>
  prints the current board
  
* captures<br>
  prints all legal captures for a set position.
  
* moves<br>
  prints all legal moves for a set position.
  
* rep<br>
  checks for threefold repetition in a position
  
* eval<br>
  prints the evaluation of the board.
  
* perft<br>
  tests all perft position.

## Elo
CCRL 40/15

```
  Name                  Elo      +       -
  Smallbrain 5.0 64-bit	3106	+363	−306
  Smallbrain 4.0 64-bit	2980	+26	−26
  Smallbrain 2.0 64-bit	2268	+30	−30
  Smallbrain 1.1 64-bit	2223	+29	−30
```
 CCRL Blitz 2'+1" (Blitz)
```
  Name                  Elo      +       -
  Smallbrain 4.0 64-bit	3004	+18	−18
  Smallbrain 3.0 64-bit	2925	+25	−25
  Smallbrain 1.1 64-bit	2174	+21	−21
```

## Features
* Engine
  * Bitboard representation
  * Zobrist hashing
  * Legal move generation
* Evaluation
  * NNUE 768 Input -> 256 hidden neurons -> 1 output
