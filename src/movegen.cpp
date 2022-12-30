#include "movegen.h"

namespace Movegen
{

bool hasLegalMoves(Board &board)
{
    if (board.sideToMove == White)
        return hasLegalMoves<White>(board);
    else
        return hasLegalMoves<Black>(board);
}
} // namespace Movegen
