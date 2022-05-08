#pragma once
#include <iostream>
#include <string>
#include <map>
#include <bitset>

#include "types.h"
#include "helper.h"
#include "psqt.h"
#include "zobrist.h"

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
	Piece pieceAtBB(Square sq);
	Piece pieceAtB(Square sq);
	void applyFen(std::string fen);
	std::string getFen();
	void printBitboard(U64 bb);
	void print();
	Piece makePiece(PieceType type, Color c);
	PieceType piece_type(Piece piece);
	std::string printMove(Move& move);
	bool isRepetition(int draw = 2);
	bool nonPawnMat(Color c);

	U64 PawnAttacks(Square sq, Color c);
	U64 KnightAttacks(Square sq);
	U64 BishopAttacks(Square sq, U64 occupied);
	U64 RookAttacks(Square sq, U64 occupied);
	U64 QueenAttacks(Square sq, U64 occupied);
	U64 KingAttacks(Square sq);

	U64 Pawns(Color c);
	U64 Knights(Color c);
	U64 Bishops(Color c);
	U64 Rooks(Color c);
	U64 Queens(Color c);
	U64 Kings(Color c);

	void removePiece(Piece piece, Square sq);
	void placePiece(Piece piece, Square sq);

	Square KingSQ(Color c);

	U64 Us(Color c);

	U64 Enemy(Color c);

	U64 All();

	U64 Empty();

	U64 EnemyEmpty(Color c);

	U64 DoCheckmask(Color c, Square sq);

	void DoPinMask(Color c, Square sq);

	void init(Color c, Square sq);

	// Is square attacked by color c
	bool isSquareAttacked(Color c, Square sq);

	U64 PawnPush(Color c, Square sq);

	U64 LegalPawnMoves(Color c, Square sq, Square ep);
	U64 LegalKnightMoves(Color c, Square sq);
	U64 LegalBishopMoves(Color c, Square sq);
	U64 LegalRookMoves(Color c, Square sq);
	U64 LegalQueenMoves(Color c, Square sq);
	U64 LegalKingMoves(Color c, Square sq);

	Movelist legalmoves();
	Movelist capturemoves();

	void makeMove(Move& move);
	void unmakeMove(Move& move);
	void makeNullMove();
	void unmakeNullMove();
};

