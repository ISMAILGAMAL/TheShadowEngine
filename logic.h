#pragma once
#include<iostream>
#include <chrono>
#include "dataStructures.h"
#include "TranspositionTable.h"

using namespace std;

void printBits(uint16_t);
bool in_board(int x, int y);
string to_algebraic(int from_x, int from_y, int target_x, int target_y);
myPair<int, int> to_index(char file, char rank);
char match_to_char(int piece);

// A struct that encapsulates an entire game state which helps us to copy and pass 
// new game states to the searching Alpha-beta pruned minimax algorithm without 
// needing complex logic to handle special moves and also allows us to interface with the gui.
struct GameState {
    int player = 1;
    int board[8][8] = {};
    myPair<int, int> black_king, white_king;

    uint16_t currentGameState;
    myVector<uint16_t> gameStateHistory;
    TranspositionTable* table;
    myVector<uint64_t> zobristKeys;
    uint64_t zobristKey;


    // The first four bits of the currentGameState are the castling rights.
    // |1|  |1|  |1|  |1|
    // BQ   BK   WQ   WK
    static constexpr uint16_t BQueenSide = 8;
    static constexpr uint16_t BKingSide = 4;
    static constexpr uint16_t WQueenSide = 2;
    static constexpr uint16_t WKingSide = 1;

    // Moves are stored in a dynamic array containing 16 bit numbers describing the pseudo-legal
    // moves that the specific white or black player can do.
    myVector<Move> white_possible_moves, black_possible_moves;

    // The pieces are encoded as follows:
    //// 1 -> king
    //// 2 -> queen
    //// 3 -> rook
    //// 4 -> knight
    //// 5 -> bishop
    //// 6 -> pawn 
    //// Negative represents black and positive represents white.

    void initialize_board(TranspositionTable& Ttable);
    void initialize_board(TranspositionTable& Ttable, string FEN);
    void pawn_moves(int x, int y, int team);
    void rook_moves(int x, int y, int team);
    void king_moves(int x, int y, int team);
    void knight_moves(int x, int y, int team);
    void bishop_moves(int x, int y, int team);
    void queen_moves(int x, int y, int team);
    void generate_piece_moves(int x, int y, int team, int type);
    void generate_all_possible_moves(int team);
    void display_possible_moves();
    bool checked(int kingx, int kingy, int type);
    void makeMove(Move& move);
    void unMakeMove(Move& move);
    bool check_legal(Move& move);
    bool checkMate(int team);
    bool staleMate(int team);
    string show();
    myPair<int, int> enPassant();
    bool canCastle(uint16_t side);
    int capturedPiece();
    Move findMove(int fromX, int fromY, int toX, int toY);
};



struct Minimax {
private:
    static constexpr int gamephaseInc[7] = { 0, 0, 4, 2, 1, 1, 0 };

    static constexpr int mgValue[7] = { 0, 0, 1025, 477, 337, 365,  82 };
    static constexpr int egValue[7] = { 0, 0, 936, 512, 281, 297,  94 };

    static constexpr int passedPawnBonuses[7] = { 0, 120, 80, 50, 30, 15, 15 };
    static constexpr int isolatedPawnPenaltyByCount[9] = { 0, -10, -25, -50, -75, -75, -75, -75, -75 };

    TranspositionTable* table;
    MoveOrderer moveOrderer;
    Move bestMove, bestMoveThisIteration;
    int node_counter = 0, reached_depth, time_limit = 3000, least_depth = 1, Q_nodes = 0, quiescenceMaxDepth = 32;
    int bestScore, bestScoreThisIteration, tableUses = 0, maxDepth = 255;
    double time_in_seconds;
    chrono::steady_clock::time_point start_time;
    chrono::milliseconds duration;
    bool broke_early = false;


    void merge(myVector<myPair<int, Move>>& leftVec, myVector<myPair<int, Move>>& rightVec, myVector<myPair<int, Move>>& vec);
    void mergeSort(myVector<myPair<int, Move>>& vec);
    void sort_moves(GameState& state);
    bool timeLimitExceeded(chrono::steady_clock::time_point& start, chrono::milliseconds& duration, int& depth);
    int get_pcsq_value(int x, int y, int piece, bool endgame);
    int evaluate_pawns(int team, int white_pawns_row[], int black_pawns_row[]);
    int minimax(GameState& state, int plyRemaining = 3, int depth = 3, int alpha = INT_MIN + 1, int beta = INT_MAX);
    int evaluation(GameState& state);
    int quiescenceSearch(GameState& state, int depth, int mainSearchDepth, int alpha, int beta);
public:
    void setTimeLimit(int time);
    Minimax(TranspositionTable& Ttable);
    Move iterative_deepening(GameState& state);
    string displayStatistics(GameState& state);

};

void perftResults(GameState& state, int depth = 0, int startDepth=1);
