#include "cli.h"

#include <filesystem>

#include "syzygy/Fathom/src/tbprobe.h"

#include "benchmark.h"
#include "board.h"
#include "movegen.h"
#include "nnue.h"
#include "perft.h"
#include "datagen.h"
#include "tests/tests.h"
#include "thread.h"

extern std::atomic_bool UCI_FORCE_STOP;
extern ThreadPool Threads;

class Version : public Argument {
   public:
    void parse(int &i, int, char const *[]) override {
        std::cout << ArgumentsParser::getVersion() << std::endl;
        i++;
        std::exit(0);
    }
};

class Perft : public Argument {
   public:
    void parse(int &i, int argc, char const *argv[]) override {
        std::string fen;

        parseDashArguments(i, argc, argv, [&](std::string key, std::string value) {
            if (key == "fen") {
                fen = value;
                no_args_ = false;
            } else if (key == "depth") {
                int depth = std::stoi(value);
                PerftTesting perft = PerftTesting();
                perft.board = Board(fen);
                perft.perfTest(depth, depth);
                no_args_ = false;
            }
        });

        if (no_args_) {
            PerftTesting perft = PerftTesting();
            perft.board = Board();
            perft.testAllPos(1);
            std::exit(0);
        }
    }

   private:
    bool no_args_ = true;
};

class Eval : public Argument {
   public:
    void parse(int &i, int argc, char const *argv[]) override {
        std::string fen;

        parseDashArguments(i, argc, argv, [&](std::string key, std::string value) {
            if (key == "fen") {
                fen = value;
                Board board = Board(fen);
                std::cout << nnue::output(board.getAccumulator(), board.side_to_move) << std::endl;
            } else {
                ArgumentsParser::throwMissing("eval", key, value);
            }
        });
    }
};

class See : public Argument {
   public:
    void parse(int &i, int argc, char const *argv[]) override {
        std::string fen;

        parseDashArguments(i, argc, argv, [&](std::string key, std::string value) {
            if (key == "fen") {
                fen = value;
                board = Board(fen);
            } else if (key == "move") {
                // std::cout << board.see(uciMove(value).move(), -93) << std::endl;
                std::exit(0);
            } else {
                ArgumentsParser::throwMissing("eval", key, value);
            }
        });
    }

   private:
    Board board;
};

class Benchmark : public Argument {
   public:
    void parse(int &, int, char const *argv[]) override {
        if (std::string(argv[1]) == std::string("bench")) {
            bench::startBench(12);
            std::exit(0);
        }
    }

   private:
};

class Generate : public Argument {
   public:
    void parse(int &i, int argc, char const *argv[]) override {
        parseDashArguments(i, argc, argv, [&](std::string key, std::string value) {
            if (key == "threads") {
                workers_ = std::stoi(value);
            } else if (key == "book") {
                book_path_ = value;
            } else if (key == "tb") {
                tb_init(value.c_str());
            } else if (key == "depth") {
                depth_ = std::stoi(value);
            } else if (key == "nodes") {
                nodes_ = std::stoi(value);
            } else if (key == "hash") {
                hash_ = std::stoi(value);
            } else if (key == "random") {
                random_ = std::stoi(value);
            } else {
                ArgumentsParser::throwMissing("eval", key, value);
            }
        });

        int ttsize = hash_ * 1024 * 1024 / sizeof(TEntry);  // 16 MiB
        TTable.allocate(ttsize * workers_);

        datagen_.generate(workers_, book_path_, depth_, nodes_, use_tb_, random_);

        std::string input;
        std::cin >> std::ws;

        while (true) {
            if (!std::getline(std::cin, input) && std::cin.eof()) {
                input == "quit";
            }

            if (input == "quit") {
                Threads.stop = UCI_FORCE_STOP = true;

                for (std::thread &th : datagen_.threads) {
                    if (th.joinable()) th.join();
                }
                std::exit(0);
            }
        }
    }

   private:
    std::string book_path_ = "";
    std::string tb_path_ = "";
    int workers_ = 1;
    int depth_ = 7;
    int nodes_ = 0;
    int hash_ = 16;
    int random_ = 0;
    bool use_tb_ = false;

    datagen::TrainingData datagen_ = datagen::TrainingData();
};

class TestRunner : public Argument {
    void parse(int &, int, char const *[]) override { assert(tests::testall()); }
};

ArgumentsParser::ArgumentsParser(int argc, char const *argv[]) {
    addArgument("-eval", new Eval());
    addArgument("perft", new Perft());
    addArgument("-version", new Version());
    addArgument("--version", new Version());
    addArgument("-v", new Version());
    addArgument("--v", new Version());
    addArgument("bench", new Benchmark());
    addArgument("-see", new See());
    addArgument("-generate", new Generate());
    addArgument("-tests", new TestRunner());

    parse(argc, argv);
}