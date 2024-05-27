#pragma once
#include "TranspositionTable.h"
#include <iostream>
#include <string>

using namespace std;

RandomGenerator::RandomGenerator() : gen(rd()), dist(0, numeric_limits<uint64_t>::max()) {}

uint64_t RandomGenerator::generate64Bits() {
	return dist(gen);
}

bool Transposition::IsQuiscence() {
	return flag > 2;
}

// Initializes the transposition table with the specified size.
TranspositionTable::TranspositionTable(int sizeMB) {
	initializePieceKeys();
	tableSize = (sizeMB * 1024 * 1024) / sizeof(Transposition);
	table.resize(tableSize);
}

// Initialize the zobrist keys used in hashing the transpositions.
void TranspositionTable::initializePieceKeys() {
	blackToMove = randomGenerator.generate64Bits();
	for (int i = 0; i < 2; i++) 
		for (int j = 0; j < 7; j++) 
			for (int k = 0; k < 8; k++) 
				for (int l = 0;l < 8;l++)
					pieceKeys[i][j][k][l] = randomGenerator.generate64Bits();
}

void TranspositionTable::storeTransposition(uint64_t key, uint8_t flag, uint8_t depth, int value, Move move) {
	int hash = key % tableSize;

	// Clear the table if it's full.
	if (getFillPercentage() > 99)
		clear();

	// First time for this hash.
	if (table[hash].key == 0) {
		entriesCount++;
		table[hash] = { key, flag, depth, move, value };
	}
	else { // The entry exists in the table.
		int originalHash = hash;

		// Search linearly for the key.
		while (table[hash].key != 0 && table[hash].key != key) {
			hash = (hash + 1) % tableSize;

			// Table is full.
			if (hash == originalHash) return;
		}

		if (hash != originalHash) collisions++;

		if (table[hash].key == 0) {
			entriesCount++;
			table[hash] = { key, flag, depth, move, value };
		}
		else {

			bool isQuiescence = flag > 2;
			bool storedIsQuiescence = table[hash].flag > 2;

			// overwrite if better depth and the search type is equal, meaning The stored value was stored during main search
			// and the current search is also the main search and same for quiescence.
			bool betterDepth = table[hash].depth < depth && (storedIsQuiescence == isQuiescence);
			// replacing upper and lower bound evaluations with exact ones.
			bool exactEvaluation = (depth >= table[hash].depth && flag == Transposition::Exact);
			// replaces values stored during quiescence search with a value from the main search.
			bool replaceQuiescence = storedIsQuiescence && !isQuiescence;

			if (betterDepth || exactEvaluation || replaceQuiescence) {
				overwrites++;
				table[hash] = { key, flag, depth, move, value };
			}
		}

	}

	
}

// Checks if the transposition exists in the table.
bool TranspositionTable::probeTransposition(uint64_t key, Transposition& trans) {
	int hash = key % tableSize;
	int originalHash = hash;
	while (table[hash].key != 0) {
		if (table[hash].key == key) {
			trans = table[hash];
			return true;
		}
		
		hash = (hash + 1) % tableSize;

		if (hash == originalHash) return false;
		
	}
	return false;
}

int TranspositionTable::lookupEvaluation(uint64_t key, int depth, int alpha, int beta, bool& found, bool Quiescence) {
	Transposition pos;
	if (probeTransposition(key, pos)) {
		if (pos.IsQuiscence() == Quiescence && pos.depth >= depth || (!pos.IsQuiscence() && Quiescence)) {
			if (pos.flag == pos.Exact || pos.flag == pos.QExact) {
				found = true;
				return pos.value;
			}
			if ((pos.flag == pos.Alpha || pos.flag == pos.QAlpha) && pos.value <= alpha) {
				found = true;
				return pos.value;
			}
			if ((pos.flag == pos.Beta || pos.flag == pos.QBeta) && pos.value >= beta) {
				found = true;
				return pos.value;
			}
		}
	}

	found = false;
	return 0;
}


uint64_t TranspositionTable::generateZobristKey(int board[8][8]) {
	uint64_t key = 0;
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			int piece = board[i][j];
			if (piece > 0) {
				key ^= pieceKeys[0][piece][i][j];
			}
			else if (piece < 0) {
				key ^= pieceKeys[1][abs(piece)][i][j];
			}
		}
	}

	return key;
}

string TranspositionTable::getFillData() {
	string output = "";
	output += "Table Occupancy: " + to_string(entriesCount) + " : " + to_string((double(entriesCount) / double(tableSize)) * 100) + " %" + '\n';
	output += "Table Overwrites:  " + to_string(overwrites) + '\n';
	output += "Table Collisions:  " + to_string(collisions) + '\n';
	return output;
}

double TranspositionTable::getFillPercentage() {
	return (double(entriesCount) / double(tableSize)) * 100;
}

void TranspositionTable::clear() {
	for (int i = 0; i < table.size(); i++) {
		table[i] = Transposition();
	}

	overwrites = 0, collisions = 0, entriesCount = 0;
}