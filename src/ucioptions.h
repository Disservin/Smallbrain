#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>

#include "types.h"
#include "tt.h"
#include "neuralnet.h"
#include "board.h"

extern TEntry* TTable;
extern U64 TT_SIZE;
extern NNUE nnue;

// 57344 MB = 2^32 * 14B / (1024 * 1024)
#define MAXHASH 57344

struct optionType {
    std::string name;
    std::string type;
    std::string defaultValue;
    std::string min;
    std::string max;
    // constructor
    optionType(std::string name, std::string  type, std::string  defaultValue, std::string min, std::string max) {
        this->name = name;
        this->type = type;
        this->defaultValue = defaultValue;
        this->min = min;
        this->max = max;
    }
};

struct optionTune {
    std::string name;
    // constructor
    optionTune(std::string name) {
        this->name = name;
    }
};

class uciOptions{
    public:
        void printOptions();
        void uciHash(int value);
        void uciEvalFile(std::string name);
        int uciThreads(int value);  
        void uciPosition(Board& board, std::string fen = DEFAULT_POS, bool update = true);
        void uciMoves(Board& board, std::vector<std::string> &tokens);
        void add_int_tune_option(std::string name, std::string type, 
                                 int defaultValue, int min, int max); 
        void add_double_tune_option(std::string name, std::string type, 
                                 double defaultValue, double min, double max);
};

void allocate_tt();

void reallocate_tt(U64 elements);

Move convert_uci_to_Move(Board& board, std::string input);