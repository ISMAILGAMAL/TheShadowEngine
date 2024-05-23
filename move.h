#pragma once
#include <iostream>
#include "dataStructures.h"

struct Move {
    static constexpr uint16_t None = 0;
    static constexpr uint16_t EnPassant = 1;
    static constexpr uint16_t Castling = 2;
    static constexpr uint16_t Promotion = 3;
    static constexpr uint16_t PawnTwoMoves = 4;

    // Moves are stored as 16 bit numbers divided as follows:
    //    4  +  3  +  3  +  3   +   3  =  16 bits
    // |0000| |000| |000| |000|   |000|
    //  Flag   Toy   ToX  FromY   FromX
    // The flag can be any of the static constexpr above essentially 
    // flagging the move as any of the special moves and the last bit stores if it's a capture or not.
    uint16_t move = 0;
    uint16_t moveOrderingValue = 0;

    Move(int fromX, int fromY, int toX, int toY, int flag = 0, bool capture = false);
    Move();

    uint16_t FromX();
    uint16_t FromY();
    uint16_t ToX();
    uint16_t ToY();
    bool IsPromotion();
    bool IsCastle();
    bool IsEnPassant();
    bool IsPawnTwoMoves();
    bool IsCapture();
};

struct MoveOrderer {
    static constexpr uint16_t pieceOrderValue[] = {0, 0, 9, 5, 3, 3, 1};

    void merge(myVector<Move>& leftVec, myVector<Move>& rightVec, myVector<Move>& vec);
    void mergeSort(myVector<Move>& vec);
    void sortMoves(myVector<Move>& moves, int board[8][8]);
};