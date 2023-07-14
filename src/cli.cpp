#include "cli.h"

#include <filesystem>

#include "syzygy/Fathom/src/tbprobe.h"

#include "benchmark.h"
#include "board.h"
#include "datagen.h"
#include "evaluation.h"
#include "perft.h"
#include "tests/tests.h"
#include "thread.h"
#include "uci.h"

extern ThreadPool Threads;

void parseDashArguments(int &i, int argc, char const *argv[],
                        const std::function<void(std::string, std::string)> &func) {
    while (i + 1 < argc && argv[i + 1][0] != '-' && i++) {
        std::string param = argv[i];
        std::size_t pos = param.find('=');

        if (pos == std::string::npos || pos == param.length() - 1) {
            continue;
        }

        std::string key = param.substr(0, pos);
        std::string value = param.substr(pos + 1);

        char quote = value[0];

        if (quote == '"' || quote == '\'') {
            while (i + 1 < argc && argv[i + 1][0] != quote) {
                i++;
                value += " " + std::string(argv[i]);
            }
        }

        func(key, value);
    }
}

class Version : public Argument {
   public:
    int parse(int &i, int, char const *[]) override {
        std::cout << ArgumentsParser::getVersion() << std::endl;
        i++;
        return 1;
    }
};

class Perft : public Argument {
   public:
    int parse(int &i, int argc, char const *argv[]) override {
        std::string fen;

        parseDashArguments(i, argc, argv, [&](const std::string &key, const std::string &value) {
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
            return 1;
        }
        return 0;
    }

   private:
    bool no_args_ = true;
};

class Eval : public Argument {
   public:
    int parse(int &i, int argc, char const *argv[]) override {
        std::string fen;

        parseDashArguments(i, argc, argv, [&](const std::string &key, const std::string &value) {
            if (key == "fen") {
                fen = value;
                Board board = Board(fen);
                std::cout << uci::convertScore(eval::evaluate(board)) << std::endl;
            } else {
                ArgumentsParser::throwMissing("eval", key, value);
            }
        });
        return 0;
    }
};

class See : public Argument {
   public:
    int parse(int &i, int argc, char const *argv[]) override {
        std::string fen;

        parseDashArguments(i, argc, argv, [&](const std::string &key, const std::string &value) {
            if (key == "fen") {
                fen = value;
                board = Board(fen);
            } else if (key == "move") {
                // std::cout << board.see(moveToUci(value).move(), -93) << std::endl;
                // return 1;
            } else {
                ArgumentsParser::throwMissing("eval", key, value);
            }
        });
        return 0;
    }

   private:
    Board board;
};

class Benchmark : public Argument {
   public:
    int parse(int &, int, char const *argv[]) override {
        if (std::string(argv[1]) == std::string("bench")) {
            bench::run(12);
            return 1;
        }
        return 0;
    }
};

class Generate : public Argument {
   public:
    int parse(int &i, int argc, char const *argv[]) override {
        parseDashArguments(i, argc, argv, [&](const std::string &key, const std::string &value) {
            if (key == "threads") {
                workers_ = std::stoi(value);
            } else if (key == "book") {
                book_path_ = value;
            } else if (key == "tb") {
                tb_init(value.c_str());

                use_tb_ = true;
                std::cout << "info string successfully loaded syzygy path " << value << std::endl;
            } else if (key == "depth") {
                depth_ = std::stoi(value);
            } else if (key == "nodes") {
                nodes_ = std::stoi(value);
            } else if (key == "hash") {
                hash_ = std::stoi(value);
            } else {
                ArgumentsParser::throwMissing("eval", key, value);
            }
        });

        TTable.allocateMB(hash_ * workers_);

        datagen_.generate(workers_, book_path_, depth_, nodes_, use_tb_);

        std::string input;
        std::cin >> std::ws;

        while (true) {
            if (!std::getline(std::cin, input) && std::cin.eof()) {
                input = "quit";
            }

            if (input == "quit") {
                Threads.stop = true;

                return 1;
            }
        }
    }

   private:
    std::string book_path_;
    std::string tb_path_;
    int workers_ = 1;
    int depth_ = 9;
    int nodes_ = 0;
    int hash_ = 16;
    bool use_tb_ = false;

    datagen::TrainingData datagen_ = datagen::TrainingData();
};

class TestRunner : public Argument {
    int parse(int &, int, char const *[]) override {
        assert(tests::testall());
        return 1;
    }
};

ArgumentsParser::ArgumentsParser() {
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
}