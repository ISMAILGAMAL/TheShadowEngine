#pragma once
#include "move.h"

// Some functions to make using moves more convenient handling all the bitwise operations.

Move::Move(int fromX, int fromY, int toX, int toY, int flag, bool capture) {
    move = 0;
    move |= fromX;
    move |= (fromY << 3);
    move |= (toX << 6);
    move |= (toY << 9);
    move |= (flag << 12);
    if (capture) move |= (1 << 15);
}

Move::Move() {};

uint16_t Move::FromX() {
    return move & 7U;
}

uint16_t Move::FromY() {
    return (move >> 3) & 7U;
}

uint16_t Move::ToX() {
    return (move >> 6) & 7U;
}

uint16_t Move::ToY() {
    return (move >> 9) & 7U;
}

bool Move::IsPromotion() {
    return ((move >> 12) & 7U) == Promotion;
}

bool Move::IsCastle() {
    return ((move >> 12) & 7U) == Castling;
}

bool Move::IsEnPassant() {
    return ((move >> 12) & 7U) == EnPassant;
}

bool Move::IsPawnTwoMoves() {
    return ((move >> 12) & 7U) == PawnTwoMoves;
}

bool Move::IsCapture() {
    return ((move >> 15) & 1);
}


void MoveOrderer::merge(myVector<Move>& leftVec, myVector<Move>& rightVec, myVector<Move>& vec) {
    int left = 0, right = 0;
    vec.clear();

    while (left < leftVec.size() && right < rightVec.size()) {
        if (leftVec[left].moveOrderingValue > rightVec[right].moveOrderingValue) {
            vec.push_back(leftVec[left++]);
        }
        else {
            vec.push_back(rightVec[right++]);
        }
    }

    while (left < leftVec.size()) {
        vec.push_back(leftVec[left++]);
    }

    while (right < rightVec.size()) {
        vec.push_back(rightVec[right++]);
    }
}


// Implements mergeSort to sort the moves in increasing order.
// move oredering is important as we explore the best moves from the previous search depth
// first which helps us prune more branches early on.
void MoveOrderer::mergeSort(myVector<Move>& vec) {
    size_t size = vec.size();
    if (size <= 1) return;

    int mid = size / 2;

    myVector<Move> leftVec;
    myVector<Move> rightVec;

    for (int i = 0; i < mid; i++) {
        leftVec.push_back(vec[i]);
    }
    for (int i = mid; i < size; i++) {
        rightVec.push_back(vec[i]);
    }

    mergeSort(leftVec);
    mergeSort(rightVec);

    merge(leftVec, rightVec, vec);
}

int partition(myVector<Move>& arr, int left, int right) {
    int i = left - 1;
    int pivotScore = arr[right].moveOrderingValue;

    for (int j = left; j <= right; j++) {
        if (arr[j].moveOrderingValue> pivotScore) {
            i++;
            Move temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    i++;
    Move temp = arr[right];
    arr[right] = arr[i];
    arr[i] = temp;

    return i;
}

// Sorts using quicksort which have proven to be much faster than mergesort in practice
// mostly due to sorting in place instead of copying.
void quickSort(myVector<Move>& arr, int left, int right) {
    if (left >= right) return;

    int pivotIndex = partition(arr, left, right);
    quickSort(arr, left, pivotIndex - 1);
    quickSort(arr, pivotIndex + 1, right);
}

// Sorting the moves using MVV-LVA heuristic (Most valuable victim-Least valuavle aggressor).
void MoveOrderer::sortMoves(myVector<Move>& moves, int board[8][8]) {
    for (int i = 0; i < moves.size(); i++) {
        uint16_t moveScore = 0;
        int capturedPiece = abs(board[moves[i].ToX()][moves[i].ToY()]);
        int movingPiece = abs(board[moves[i].FromX()][moves[i].FromY()]);

        if (moves[i].IsCapture()) {
            moveScore = 10 * pieceOrderValue[capturedPiece] - pieceOrderValue[movingPiece];
        }

        if (moves[i].IsPromotion()) {
            moveScore += pieceOrderValue[movingPiece];
        }

        moves[i].moveOrderingValue = moveScore;
    }

    quickSort(moves, 0, moves.size() - 1);
    //mergeSort(moves);
}