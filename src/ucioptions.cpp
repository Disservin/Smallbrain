#include "ucioptions.h"

std::vector<optionType> optionsPrint{
    optionType("Hash", "spin", "400", "1", "57344"), optionType("EvalFile", "string", "default.nnue", "", ""),
    optionType("Threads", "spin", "1", "1", "256"), optionType("SyzygyPath", "string", "<empty>", "", ""),
    optionType("UCI_Chess960", "check", "false", "", "")};

void uciOptions::printOptions()
{
    for (auto vectoriter = optionsPrint.begin(); vectoriter != optionsPrint.end(); vectoriter++)
    {
        if ((*vectoriter).min != "")
        {
            std::cout << "option name " << (*vectoriter).name << " type " << (*vectoriter).type << " default "
                      << (*vectoriter).defaultValue << " min " << (*vectoriter).min << " max " << (*vectoriter).max
                      << std::endl;
        }
        else
        {
            std::cout << "option name " << (*vectoriter).name << " type " << (*vectoriter).type << " default "
                      << (*vectoriter).defaultValue << std::endl;
        }
    }
}

void uciOptions::uciHash(int value)
{
    int sizeMB = std::clamp(value, 2, MAXHASH);
    U64 elements = (static_cast<uint64_t>(sizeMB) * 1024 * 1024) / sizeof(TEntry);
    reallocateTT(elements);
}

void uciOptions::uciEvalFile(std::string name)
{
    std::cout << "Loading eval file: " << name << std::endl;
    NNUE::init(name.c_str());
}

int uciOptions::uciThreads(int value)
{
    return std::clamp(value, 1, 512);
}

void uciOptions::uciSyzygy(std::string input)
{
    std::string path = input.substr(input.find("value ") + 6);

    tb_init(path.c_str());
    if (TB_LARGEST == 0)
    {
        std::cout << "TB NOT FOUND" << std::endl;
    }
    else
    {
        std::cout << "using " << TB_LARGEST << " syzygy " << std::endl;
        useTB = true;
    }
}

void uciOptions::uciPosition(Board &board, std::string fen, bool update)
{
    board.applyFen(fen, update);
}

void uciOptions::uciChess960(Board &board, std::string_view v)
{
    board.chess960 = v == "true" ? true : false;
}

void uciOptions::uciMoves(Board &board, std::vector<std::string> &tokens)
{
    std::size_t index = std::find(tokens.begin(), tokens.end(), "moves") - tokens.begin();
    index++;
    for (; index < tokens.size(); index++)
    {
        Move move = convertUciToMove(board, tokens[index]);
        board.makeMove<false>(move);
    }
}

void uciOptions::addIntTuneOption(std::string name, std::string type, int defaultValue, int min, int max)
{
    optionsPrint.push_back(
        optionType(name, type, std::to_string(defaultValue), std::to_string(min), std::to_string(max)));
}

void uciOptions::addDoubleTuneOption(std::string name, std::string type, double defaultValue, double min, double max)
{
    optionsPrint.push_back(
        optionType(name, type, std::to_string(defaultValue), std::to_string(min), std::to_string(max)));
}

void allocateTT()
{
    if ((TTable = (TEntry *)malloc(TT_SIZE * sizeof(TEntry))) == NULL)
    {
        std::cout << "Error: Could not allocate memory for TT" << std::endl;
        exit(1);
    }
    std::memset(TTable, 0, TT_SIZE * sizeof(TEntry));
}

void reallocateTT(U64 elements)
{
    TEntry *oldbuffer = TTable;
    if ((TTable = (TEntry *)realloc(TTable, elements * sizeof(TEntry))) == NULL)
    {
        std::cout << "Error: Could not allocate memory for TT" << std::endl;
        free(oldbuffer);
        exit(1);
    }

    TT_SIZE = elements;
    std::memset(TTable, 0, TT_SIZE * sizeof(TEntry));
}

void clearTT()
{
    std::memset(TTable, 0, TT_SIZE * sizeof(TEntry));
}

Move convertUciToMove(Board &board, std::string input)
{
    std::string from = input.substr(0, 2);
    char letter = from[0];
    int file = letter - 96;
    int rank = from[1] - 48;
    int from_index = (rank - 1) * 8 + file - 1;
    Square source = Square(from_index);

    if (input.length() == 4)
    {
        std::string to = input.substr(2);
        letter = to[0];
        file = letter - 96;
        rank = to[1] - 48;

        int to_index = (rank - 1) * 8 + file - 1;
        Square target = Square(to_index);
        PieceType piece = type_of_piece(board.pieceAtBB(source));

        // convert to king captures rook
        if (!board.chess960 && piece == KING && square_distance(target, source) == 2)
        {
            target = file_rank_square(target > source ? FILE_H : FILE_A, square_rank(source));
        }

        return make(piece, source, target, false);
    }
    else if (input.length() == 5)
    {
        std::string to = input.substr(2, 2);
        letter = to[0];
        file = letter - 96;
        rank = to[1] - 48;

        int to_index = (rank - 1) * 8 + file - 1;
        Square target = Square(to_index);

        char prom = input.at(4);
        PieceType piece = pieceToInt[prom];
        return make(piece, source, target, true);
    }
    else
    {
        std::cout << "FALSE INPUT" << std::endl;
        return make(NONETYPE, NO_SQ, NO_SQ, false);
    }
}