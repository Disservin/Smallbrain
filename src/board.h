#pragma once
#include <iostream>
#include <string>
#include <map>
#include <bitset>

#include "types.h"
#include "helper.h"
#include "psqt.h"
#include "zobrist.h"
#include "neuralnet.h"

extern NNUE nnue;

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
	Move list[256]{};
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

	Piece board[64]{ None };
	U64 Bitboards[12]{};
	U64 SQUARES_BETWEEN_BB[64][64]{};

	// all bits set
	U64 checkMask = 18446744073709551615ULL;

	U64 pinHV{};
	U64 pinD{};
	uint8_t doubleCheck{};
	U64 hashKey{};
	U64 occWhite{};
	U64 occBlack{};
	U64 occAll{};
	U64 enemyEmptyBB{};

	int psqt_mg{};
	int psqt_eg{};

	U64 hashHistory[1024]{};
	States prevStates{};
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

	void accumulate();

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

	// removes a piece from a square, also removes psqt values
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

	// returns a pawn push (only 1 square)
	U64 PawnPush(Color c, Square sq);
	U64 PawnPush2(Color c, Square sq, U64 push);

	// all legal moves for each piece
	U64 LegalPawnMoves(Color c, Square sq, Square ep);
	U64 LegalKnightMoves(Color c, Square sq);
	U64 LegalBishopMoves(Color c, Square sq);
	U64 LegalRookMoves(Color c, Square sq);
	U64 LegalQueenMoves(Color c, Square sq);
	U64 LegalKingMoves(Color c, Square sq);

	// all legal moves for a position
	Movelist legalmoves();
	Movelist capturemoves();

	// plays the move on the internal board
	void makeMove(Move& move);
	void unmakeMove(Move& move);
	void makeNullMove();
	void unmakeNullMove();
};

