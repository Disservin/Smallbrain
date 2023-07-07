# Smallbrain

## History

During the pandemic I rediscovered chess and played a lot of games with my friends.<br>
Then I started to program my first engine [python-smallbrain](https://github.com/Disservin/python-smallbrain) in python, with the help of python-chess.<br>

I quickly realized how slow python is for chess engine programming, so I started to learn C++.<br>
My first try was [cppsmallbrain](https://github.com/Disservin/cppsmallbrain), though after some time I found the code very buggy and ugly.<br>
So I started [Smallbrain](https://github.com/Disservin/Smallbrain) from scratch, during that time, I also joined Stockfish development. <br>

After some time I began implementing a NNUE into Smallbrain, with the help of Luecx from [Koivisto](https://github.com/Luecx/Koivisto).<br>
As of now Smallbrain has a NNUE trained on 1 billion depth 9 + depth 7 + 150 million depth 9 DFRC fens generated with the built in data generator and using [CudAD trainer](https://github.com/Luecx/CudAD) to ultimately train the network.

## News

The latest development versions support FRC/DFRC.

## Compile

Compile it using the Makefile

```
make -j
.\smallbrain.exe bench
```

compare the Bench with the Bench in the commit messages,
they should be the same.

or download the latest the latest executable directly over Github. <br>
At the bottom you should be able to find multiple different compiles, choose one that doesnt crash.

Ordered by performance you should try x86-64-avx2 first then x86-64-modern and at last x86-64.
If you want maximum performance you should compile Smallbrain yourself.

## Elo

#### [CCRL 40/2 FRC](https://ccrl.chessdom.com/ccrl/404FRC/)

| Name                  | Elo  | +   | -   |
| --------------------- | ---- | --- | --- |
| Smallbrain 7.0        | 3537 | +13 | −13 |
| Smallbrain dev-221204 | 3435 | +15 | −15 |

#### [CCRL 40/15](http://ccrl.chessdom.com/ccrl/4040/)

| Name                       | Elo  | +   | -   |
| -------------------------- | ---- | --- | --- |
| Smallbrain 7.0 64-bit 4CPU | 3374 | +20 | −20 |
| Smallbrain 7.0 64-bit      | 3309 | +15 | −15 |
| Smallbrain 6.0 4CPU        | 3307 | +23 | −23 |
| Smallbrain 6.0             | 3227 | +23 | −23 |
| Smallbrain 5.0 4CPU        | 3211 | +23 | −23 |
| Smallbrain 5.0             | 3137 | +20 | −20 |
| Smallbrain 4.0             | 2978 | +25 | −25 |
| Smallbrain 2.0             | 2277 | +28 | −29 |
| Smallbrain 1.1             | 2224 | +29 | −30 |

#### [CCRL Blitz 2'+1" (Blitz)](http://ccrl.chessdom.com/ccrl/404/)

| Name                       | Elo  | +   | -   |
| -------------------------- | ---- | --- | --- |
| Smallbrain 7.0 64-bit 8CPU | 3581 | +30 | −29 |
| Smallbrain 7.0 64-bit      | 3433 | +14 | −14 |
| Smallbrain 6.0             | 3336 | +17 | −17 |
| Smallbrain 5.0             | 3199 | +18 | −18 |
| Smallbrain 4.0             | 3005 | +18 | −18 |
| Smallbrain 3.0             | 2921 | +20 | −20 |
| Smallbrain 1.1             | 2174 | +20 | −20 |

#### [Stefan Pohl Computer Chess](https://www.sp-cc.de/)

| no  | Program             | Elo  | +   | -   | Games | Score | Av.Op. | Draws |
| --- | ------------------- | ---- | --- | --- | ----- | ----- | ------ | ----- |
| 32  | Smallbrain 7.0 avx2 | 3445 | 6   | 6   | 10000 | 46.7% | 3469   | 63.0% |
| 34  | Smallbrain 6.0 avx2 | 3345 | 7   | 7   | 9000  | 52.1% | 3331   | 49.9% |

#### [CEGT](http://www.cegt.net/40_40%20Rating%20List/40_40%20All%20Versions/rangliste.html)

| no  | Program                   | Elo  | +   | -   | Games | Score | Av.Op. | Draws |
| --- | ------------------------- | ---- | --- | --- | ----- | ----- | ------ | ----- |
| 217 | Smallbrain 7.0 x64 1CPU   | 3296 | 14  | 14  | 1596  | 50.7% | 3291   | 63.4% |
| 271 | Smallbrain 6.0NN x64 1CPU | 3203 | 16  | 16  | 1300  | 42.8% | 3258   | 51.2% |

## UCI settings

- Hash
  The size of the hash table in MB.
- Threads
  The number of threads used for search.
- EvalFile
  The neural net used for the evaluation,
  currently only default.nnue exist.

- SyzygyPath
  Path to the syzygy files.

## Engine specific commands

- go perft \<depth>
  calculates perft from a set position up to _depth_.
- print
  prints the current board
- eval
  prints the evaluation of the board.
- perft
  tests all perft position.

## Features

- Evaluation
  - As of v6.0 the NNUE training dataset was regenerated using depth 9 selfplay games + random 8 piece combinations.

## Datageneration

- Starts the data generation.

  ```
  -gen
  ```

- Specify the number of threads to use.
  default: 1

  ```
  -threads <int>
  ```

- If you want to start from a book instead of using random playout.
  default: ""

  ```
  -book <path/to/book>
  ```

- Path to TB, only used for adjudication.
  default: ""

  ```
  -tb <path/to/tb>
  ```

- Analysis depth, values between 7-9 are good.
  default: 7

  ```
  -depth <int>
  ```

- Analysis nodes, values between 2500-10000 are good.
  default: 0

  ```
  -nodes <int>
  ```

- The amount of hash in MB. This gets multiplied by the number of threads.
  default: 16

  ```
  -hash <int>
  ```

- With a possibility of 1/random start from a random position.
  default: 0 to turn this off.

  ```
  -random <int>
  ```

- Example:

```
.\smallbrain.exe -generate threads=30 book=E:\Github\Smallbrain\src\data\DFRC_openings.epd tb=E:/Chess/345
```

```
.\smallbrain.exe -generate threads=30 depth=7 hash=256 tb=F:\syzygy_5\3-4-5
.\smallbrain.exe -generate threads=30 depth=9 tb=H:/Chess/345
.\smallbrain.exe -generate threads=30 nodes=5000 tb=H:/Chess/345
```
