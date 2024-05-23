#pragma once
#include <random>
#include "dataStructures.h"
#include "move.h"

using namespace std;

struct RandomGenerator {
	random_device rd;
	mt19937_64 gen;
	uniform_int_distribution<uint64_t> dist;

	RandomGenerator();
	uint64_t generate64Bits();
};

struct Transposition {

	// When the value of the position is exact.
	static constexpr uint8_t Exact = 0;
	static constexpr uint8_t QExact = Exact + 3;
	// There was a better move for the current player earlier making the evaluation of this position
	// an upper bound meaning the actual evaluation is at most equal to value. (The actual value could be <= value)
	static constexpr uint8_t Alpha = 1;
	static constexpr uint8_t QAlpha = Alpha + 3;
	// The opponent had a better move earlier, meaning the opponent would not play the sequence of moves that will
	// lead to this move making the move a lower bound of the actual value. (The actual value could be >= value)
	static constexpr uint8_t Beta = 2;
	static constexpr uint8_t QBeta = Beta + 3;

	uint64_t key = 0; // Zobrist key of the board
	// The flag contains whether the position has been evaluated or there was a cut-off due to alpha-beta.
	uint8_t flag; // Node type flag: exact, lower bound (Beta), upper bound (alpha)
	uint8_t depth; // Depth of the search
	Move move;
	int value; // Evaluation score


	bool IsQuiscence();
};

struct TranspositionTable {
private:
	RandomGenerator randomGenerator;
public:
	uint64_t pieceKeys[2][7][8][8];
	uint64_t blackToMove;
	myVector<Transposition> table;
	int tableSize;
	int entriesCount = 0, overwrites = 0, collisions = 0;

	TranspositionTable(int sizeMB);
	void initializePieceKeys();
	void storeTransposition(uint64_t key, uint8_t flag, uint8_t depth, int value, Move move);
	bool probeTransposition(uint64_t key, Transposition& trans);
	int lookupEvaluation(uint64_t key, int depth, int alpha, int beta, bool& found, bool Quiescence);
	string getFillPercentage();
	void clear();
	uint64_t generateZobristKey(int board[8][8]);
};