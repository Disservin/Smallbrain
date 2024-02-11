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
- UCI_ShowWDL
  Shows the WDL score in the UCI info.
- UCI_Chess960
  Enables Chess960 support.

## Engine specific uci commands

- go perft \<depth>
  calculates perft from a set position up to _depth_.
- print
  prints the current board
- eval
  prints the evaluation of the board.

## CLI commands

- bench
  Starts the bench.
- perft fen=\<fen> depth=\<depth>
  fen and depth are optional.
- -eval fen=\<fen>
- -version/--version/--v/-v
  Prints the version.
- -see
  Calculates the static exchange evaluation of the current position.
- -generate
  Starts the data generation.
- -tests
  Starts the tests.

## Features

- Evaluation
  - As of v6.0 the NNUE training dataset was regenerated using depth 9 selfplay games + random 8 piece combinations.

## Datageneration

- Starts the data generation.

  ```
  -generate
  ```

- Specify the number of threads to use.
  default: 1

  ```
  threads=<int>
  ```

- If you want to start from a book instead of using random playout.
  default: ""

  ```
  book=<path/to/book>
  ```

- Path to TB, only used for adjudication.
  default: ""

  ```
  tb=<path/to/tb>
  ```

- Analysis depth, values between 7-9 are good.
  default: 7

  ```
  depth=<int>
  ```

- Analysis nodes, values between 2500-10000 are good.
  default: 0

  ```
  nodes=<int>
  ```

- The amount of hash in MB. This gets multiplied by the number of threads.
  default: 16

  ```
  hash=<int>
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

## Acknowledgements

I'd also like to thank the following people for their help and support:

- A big thanks to Luecx for his amazing CudAd trainer and his help with the NNUE implementation.
- Andrew Grant for the OpenBench platform https://github.com/AndyGrant/OpenBench
- Morgan Houppin, author of Stash https://github.com/mhouppin/stash-bot for his debug sessions.
- Various other people from Stockfish discord for their help.
- [Chess.com](https://www.chess.com/computer-chess-championship) for their Smallbrain inclusion in the Computer Chess Championship (CCC)
- [TCEC](https://tcec-chess.com/) for their Smallbrain invitation.

### Engines

The following engines have taught me a lot about chess programming and I'd like to thank their authors for their work:

- [Stockfish](https://github.com/official-stockfish/Stockfish)
- [Koivisto](https://github.com/Luecx/Koivisto)
- [Weiss](https://github.com/TerjeKir/weiss)

### Tools

Included:
The following parts of the code are from other projects, I'd like to thank their authors for their work and their respective licenses remain the same:

- [Fathom](https://github.com/jdart1/Fathom) [MIT](https://github.com/jdart1/Fathom/blob/master/LICENSE)
- [Incbin](https://github.com/graphitemaster/incbin) [UNLICENSE](https://github.com/graphitemaster/incbin/blob/main/UNLICENSE)

External:

- [OpenBench](https://github.com/AndyGrant/OpenBench) [GPL-3.0](https://github.com/AndyGrant/OpenBench/blob/master/LICENSE)
