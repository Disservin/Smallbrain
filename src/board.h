#pragma once
#include <iostream>
#include <string>
#include <map>
#include <bitset>

#include "types.h"
#include "helper.h"
#include "scores.h"
#include "zobrist.h"
#include "neuralnet.h"

// extern NNUE nnue;
extern uint8_t inputValues[INPUT_WEIGHTS];
extern int16_t inputWeights[INPUT_WEIGHTS * HIDDEN_WEIGHTS];
extern int16_t hiddenBias[HIDDEN_BIAS];
extern int16_t hiddenWeights[HIDDEN_WEIGHTS];
extern int32_t outputBias[OUTPUT_BIAS];

static constexpr U64 DEFAULT_CHECKMASK = 18446744073709551615ULL;

struct State {
	Square enPassant{};
	uint8_t castling{};
	uint8_t halfMove{};
	Piece capturedPiece = None;
	U64 h{};
	State(Square enpassantCopy = {}, uint8_t castlingRightsCopy = {},
		uint8_t halfMoveCopy = {}, Piece capturedPieceCopy = None, U64 hashCopy = {}) :
		enPassant(enpassantCopy), castling(castlingRightsCopy),
		halfMove(halfMoveCopy), capturedPiece(capturedPieceCopy), h(hashCopy) {}
};

struct States {
	State list[1024]{};
	uint16_t size{};

	void Add(State s) {
		list[size] = s;
		size++;
	}

	State Get() {
		size--;
		return list[size];
	}
};

struct Movelist {
	Move list[MAX_MOVES]{};
	uint8_t size{};

	void Add(Move move) {
		list[size] = move;
		size++;
	}
};

class Board
{
public:
	uint8_t castlingRights{};
	Square enPassantSquare{};
	uint8_t halfMoveClock{};
	uint16_t fullMoveNumber{};
	Color sideToMove = White;

	Piece board[MAX_SQ]{ None };
	U64 Bitboards[12]{};
	U64 SQUARES_BETWEEN_BB[MAX_SQ][MAX_SQ]{};

	// all bits set
	U64 checkMask = DEFAULT_CHECKMASK;

	U64 pinHV{};
	U64 pinD{};
	uint8_t doubleCheck{};
	U64 hashKey{};
	U64 occWhite{};
	U64 occBlack{};
	U64 occAll{};
	U64 enemyEmptyBB{};

	U64 hashHistory[1024]{};
	States prevStates{};

	int16_t accumulator[HIDDEN_BIAS];
	void activate(int inputNum);
	void deactivate(int inputNum);

	Board();
	U64 zobristHash();

	
	void initializeLookupTables();
	
	// Finds what piece are on the square using the bitboards
	Piece pieceAtBB(Square sq);

	// Finds what piece are on the square using the board (more performant)
	Piece pieceAtB(Square sq);

	// aplys a new Fen to the board
	void applyFen(std::string fen);

	// returns a Fen string of the current board
	std::string getFen();

	// prints any bitboard passed to it
	void printBitboard(U64 bb);

	// prints the board
	void print();

	// makes a Piece from only the piece type and color
	Piece makePiece(PieceType type, Color c);

	// returns the piecetype of a piece
	PieceType piece_type(Piece piece);

	// prints a move in uci format
	std::string printMove(Move& move);

	// detects if the position is a repetition by default 2, fide would be 3
	bool isRepetition(int draw = 2);

	// only pawns + king = true else false
	bool nonPawnMat(Color c);

	// Attacks of each piece
	U64 PawnAttacks(Square sq, Color c);
	U64 KnightAttacks(Square sq);
	U64 BishopAttacks(Square sq, U64 occupied);
	U64 RookAttacks(Square sq, U64 occupied);
	U64 QueenAttacks(Square sq, U64 occupied);
	U64 KingAttacks(Square sq);

	// Gets the piece of the specified color
	U64 Pawns(Color c);
	U64 Knights(Color c);
	U64 Bishops(Color c);
	U64 Rooks(Color c);
	U64 Queens(Color c);
	U64 Kings(Color c);

	void removePieceSimple(Piece piece, Square sq);

	void placePieceSimple(Piece piece, Square sq);
	
	// updates accumulator
	void removePiece(Piece piece, Square sq);
	void placePiece(Piece piece, Square sq);

	U64 updateKeyPiece(Piece piece, Square sq);
	U64 updateKeyCastling();
	U64 updateKeyEnPassant(Square sq);
	U64 updateKeySideToMove();

	// returns the King Square of the specified color
	Square KingSQ(Color c);

	// returns all pieces of color
	U64 Us(Color c);

	// returns all pieces of the other color
	U64 Enemy(Color c);

	// returns all pieces color
	U64 All();

	// returns all empty squares
	U64 Empty();

	// returns all empty squares or squares with an enemy on them
	U64 EnemyEmpty(Color c);

	// creates the checkmask
	U64 DoCheckmask(Color c, Square sq);

	// creates the pinmask
	void DoPinMask(Color c, Square sq);

	// creates the pinmask and checkmask
	void init(Color c, Square sq);

	// Is square attacked by color c
	bool isSquareAttacked(Color c, Square sq);
	U64 allAttackers(Square sq, U64 occupiedBB);
	U64 attackersForSide(Color attackerColor, Square sq, U64 occupiedBB);

	// returns a pawn push (only 1 square)
	U64 PawnPushSingle(Color c, Square sq);
	U64 PawnPushBoth(Color c, Square sq);

	// legal captures + promotions
	U64 LegalPawnNoisy(Color c, Square sq, Square ep);
	U64 LegalKingCaptures(Color c, Square sq);

	// all legal moves for each piece
	U64 LegalPawnMoves(Color c, Square sq);
	U64 LegalPawnMovesEP(Color c, Square sq, Square ep);
	U64 LegalKnightMoves(Color c, Square sq);
	U64 LegalBishopMoves(Color c, Square sq);
	U64 LegalRookMoves(Color c, Square sq);
	U64 LegalQueenMoves(Color c, Square sq);
	U64 LegalKingMoves(Color c, Square sq);
	U64 LegalKingMovesCastling(Color c, Square sq);

	// all legal moves for a position
	Movelist legalmoves();
	Movelist capturemoves();

	// plays the move on the internal board
	void makeMove(Move& move);
	void unmakeMove(Move& move);
	void makeNullMove();
	void unmakeNullMove();
};

