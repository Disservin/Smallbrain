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
  Smallbrain 5.0 4CPU   3214	+23	−23
  Smallbrain 5.0        3134	+21	−21
  Smallbrain 4.0        2979	+25	−25	
  Smallbrain 2.0        2277	+28	−29	
  Smallbrain 1.1        2224	+29	−30
```
 CCRL Blitz 2'+1" (Blitz)
```
  Name                  Elo      +       -
  Smallbrain 5.0      	3200	+18	−19
  Smallbrain 4.0      	3005	+18	−18
  Smallbrain 3.0        2921	+19	−20
  Smallbrain 1.1        2173	+20	−20
```

## Features
* Engine
  * Bitboard representation
* Evaluation
  * NNUE 768 Input -> 512 hidden neurons -> 1 output

## Datageneration
* .\smallbrain5.0.exe -gen
  * -threads \<num>
  * -book \<path/to/book>
  * -tb \<path/to/tb>
  * -depth \<depth>
  * -endgame \<onlyGenEndgames>
* Example: 
.\smallbrain5.0.exe -gen -threads 4 -book E:\Github\Smallbrain\src\data\DFRC_openings.epd -tb E:/Chess/345
.\smallbrain5.0.exe -gen -threads 30 -tb E:/Chess/345