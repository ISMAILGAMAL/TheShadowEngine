#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include "pcsq.h"
#include "dataStructures.h"
#include "TranspositionTable.h"
#include "move.h"
#include "logic.h"

using namespace std;

// Some helper functions.

void printBits(uint16_t num) {
    for (int i = 15; i >= 0; i--) {
        if (i == 13 || i == 9 || i == 6 || i == 3) cout << " ";
        cout << (((1 << i) & num) != 0);
    }cout << endl;
}

// Checks if index is in range of the board.
bool in_board(int x, int y) {
    if (x < 0 || y < 0 || x>7 || y>7)
        return 0;
    return 1;
}

// A function that takes the move in indices and turns it to Standard chess UCI
// ex: 1 0 3 0 -> a7a5
string to_algebraic(int from_x, int from_y, int target_x, int target_y) {
    string s = "";
    s += char(from_y + 'a');
    s += char((8 - from_x) + '0');
    s += char(target_y + 'a');
    s += char((8 - target_x) + '0');
    return s;
}

// This does the opposite of the above and turns two chars (the first or second part of uci)
// to indices to be used inside of the board ex: a7 -> 1 0
myPair<int, int> to_index(char file, char rank) {
    int x, y;
    x = 8 - int(rank - '0');
    y = int(file - 'a');
    return { x, y };
}

// Matches a piece type which we wrote in integers to a char to display in the debugging terminal board.
char match_to_char(int piece) {
    char piece_char;
    if (abs(piece) == 1) piece_char = 'k';
    else if (abs(piece) == 2) piece_char = 'q';
    else if (abs(piece) == 3) piece_char = 'r';
    else if (abs(piece) == 4) piece_char = 'n';
    else if (abs(piece) == 5) piece_char = 'b';
    else if (abs(piece) == 6) piece_char = 'p';
    else return '.';
    if (piece > 0) return toupper(piece_char);
    return piece_char;
}


// A struct that encapsulates an entire game state which helps us to copy and pass 
// new game states to the searching Alpha-beta pruned minimax algorithm without 
// needing complex logic to handle special moves and also allows us to interface with the gui.

void GameState::initialize_board(TranspositionTable& Ttable) {
    
    //  resetting everything by initializing all the board's and array's values to zero.

    player = 1;
    black_king = { 0,4 };
    white_king = { 7,4 };
    memset(board, 0, sizeof(board));
    currentGameState = 0b0000000000001111;;

    // Initializing the new board to standard beginning chess position.
    int z = 0;
    for (int i = 0; i < 8; i++) {
        board[1][i] = -6;
    }
    for (int i = 0; i < 8; i++) {
        board[6][i] = 6;
    }
    board[0][0] = -3; board[0][7] = -3; // rooks
    board[7][0] = 3; board[7][7] = 3;
    board[0][1] = -4; board[0][6] = -4; // knights
    board[7][1] = 4; board[7][6] = 4;
    board[0][2] = -5; board[0][5] = -5; // bishops
    board[7][2] = 5; board[7][5] = 5;
    board[0][3] = -2;  board[7][3] = 2; // queens
    board[0][4] = -1; board[7][4] = 1; // kings

    table = &Ttable;
    zobristKey = table->generateZobristKey(board);
}

// A constructor that allows us to copy any board fen strings from the internet 
// and initialize the board to that state.
void GameState::initialize_board(TranspositionTable& Ttable, string FEN) {
    string board_fen = "", player_fen = "", castling_fen = "", en_passant_fen = "";
    int num_break = 0;

    currentGameState = 0;

    // Parses the fen into four strings.
    for (int i = 0; i < FEN.size(); i++) {
        if (FEN[i] == ' ') { num_break++; continue; }
        if (num_break == 0) board_fen.push_back(FEN[i]);
        else if (num_break == 1) player_fen.push_back(FEN[i]);
        else if (num_break == 2) castling_fen.push_back(FEN[i]);
        else if (num_break == 3) en_passant_fen.push_back(FEN[i]);
        else break;
    }

    memset(board, 0, sizeof(board));

    int i = 0, j = 0;

    // Fills the board according to the parsed FEN.
    // You can read more on FENs from here: https://w...content-available-to-author-only...s.com/terms/fen-chess
    for (int k = 0; k < board_fen.size(); k++) {
        if (board_fen[k] == '/') { j = 0; i++; continue; }
        else if (board_fen[k] < 58) { j += int(board_fen[k] - '0'); continue; }// If it's a number skip that amount of squares.
        else if (toupper(board_fen[k]) == 'K') {
            board[i][j] = 1;
            if (board_fen[k] == 'K') white_king = { i, j };
            else black_king = { i, j };
        }
        else if (toupper(board_fen[k]) == 'Q') board[i][j] = 2;
        else if (toupper(board_fen[k]) == 'R') board[i][j] = 3;
        else if (toupper(board_fen[k]) == 'N') board[i][j] = 4;
        else if (toupper(board_fen[k]) == 'B') board[i][j] = 5;
        else if (toupper(board_fen[k]) == 'P') board[i][j] = 6;
        if (islower(board_fen[k])) board[i][j] *= -1;
        j++;
    }

    // Assigning the player from the parsed FEN
    if (player_fen == "w") player = 1;
    else player = -1;

    // Assigning castling rights according to the FEN
    for (int i = 0; i < castling_fen.size(); i++) {
        if (castling_fen[i] == 'q') { currentGameState |= 8; }
        else if (castling_fen[i] == 'k') { currentGameState |= 4; }
        else if (castling_fen[i] == 'Q') { currentGameState |= 2; }
        else if (castling_fen[i] == 'K') { currentGameState |= 1; }
    }

    // en passant
    if (en_passant_fen.size() > 1) {
        int rank = 8 - int(en_passant_fen[1] - '0'), file = int(en_passant_fen[0] - 'a');
        currentGameState |= (rank << 4);
        currentGameState |= (file << 7);
    }

    table = &Ttable;
    zobristKey = table->generateZobristKey(board);

    if (player == -1) zobristKey ^= table->blackToMove;
}

// The following functions all Generate pseudo-legal moves for the pieces and push it to the object's move vector.

// Generate pseudo-legal moves for the pawn.
void GameState::pawn_moves(int x, int y, int team) {
    myPair<int, int> enPassantSq = enPassant();
    uint16_t flag = Move::None;

    if (team == 1) {

        if (x - 1 == 0) flag = Move::Promotion;

        if (in_board(x - 1, y) && board[x - 1][y] == 0){ // Moving once.
            Move move(x, y, x - 1, y, flag);
            if (check_legal(move))
                white_possible_moves.push_back(move);
        }
        if (in_board(x - 2, y) && board[x - 2][y] == 0 && board[x - 1][y] == 0 && x == 6) { // Moving twice.
            Move move(x, y, x - 2, y, Move::PawnTwoMoves);
            if (check_legal(move))
                white_possible_moves.push_back(move);
        }

        if (in_board(x - 1, y - 1) && board[x - 1][y - 1] < 0) {
            Move move(x, y, x - 1, y - 1, flag, true);
            if (check_legal(move))       // Diagonal piece capturing.
                white_possible_moves.push_back(move);
        }
        if (in_board(x - 1, y + 1) && board[x - 1][y + 1] < 0) {
            Move move(x, y, x - 1, y + 1, flag, true);
            if (check_legal(move))
                white_possible_moves.push_back(move);
        }

        if ((enPassantSq.first != 0 || enPassantSq.second != 0) && enPassantSq.first == 2) {
            if (enPassantSq.first == x - 1 && enPassantSq.second == y - 1) {
                Move move(x, y, x - 1, y - 1, Move::EnPassant, true); // En passant
                if (check_legal(move))
                    white_possible_moves.push_back(move);  
            }
            if (enPassantSq.first == x - 1 && enPassantSq.second == y + 1) {
                Move move(x, y, x - 1, y + 1, Move::EnPassant, true);
                if (check_legal(move))
                    white_possible_moves.push_back(move);
            }
        }

    }
    else {

        if (x + 1 == 7) flag = Move::Promotion;

        if (in_board(x + 1, y) && board[x + 1][y] == 0) {
            Move move(x, y, x + 1, y, flag);
            if (check_legal(move))
                black_possible_moves.push_back(move);
        }
        if (in_board(x + 2, y) && board[x + 2][y] == 0 && board[x + 1][y] == 0 && x == 1) {
            Move move(x, y, x + 2, y, Move::PawnTwoMoves);
            if (check_legal(move))
                black_possible_moves.push_back(move);
        }

        if (in_board(x + 1, y - 1) && board[x + 1][y - 1] > 0) {
            Move move(x, y, x + 1, y - 1, flag, true);
            if (check_legal(move))
                black_possible_moves.push_back(move);
        }
        if (in_board(x + 1, y + 1) && board[x + 1][y + 1] > 0) {
            Move move(x, y, x + 1, y + 1, flag, true);
            if (check_legal(move))
                black_possible_moves.push_back(move);
        }

        if ((enPassantSq.first != 0 || enPassantSq.second != 0) && enPassantSq.first == 5) {
            if (enPassantSq.first == x + 1 && enPassantSq.second == y - 1) {
                Move move(x, y, x + 1, y - 1, Move::EnPassant, true);
                if (check_legal(move))
                    black_possible_moves.push_back(move);
            }
            if (enPassantSq.first == x + 1 && enPassantSq.second == y + 1) {
                Move move(x, y, x + 1, y + 1, Move::EnPassant, true);
                if (check_legal(move))
                    black_possible_moves.push_back(move);
            }
        }
    }
}

void GameState::rook_moves(int x, int y, int team) {
    int x_offsets[] = { 0, 0, 1, -1 }; // defining an array of offsets to loop over and generate moves  
    int y_offsets[] = { 1, -1, 0, 0 }; // in the four vertical and horizontal directions.

    for (int i = 0; i < 4; i++) {
        int target_x = x + x_offsets[i], target_y = y + y_offsets[i];
        while (in_board(target_x, target_y)) {
            int piece = board[target_x][target_y];
            if (piece * team > 0) break; // breaking before pushing the position of a friendly piece.

            Move move(x, y, target_x, target_y, Move::None, (piece != 0));
            if (team == 1 && check_legal(move)) 
                white_possible_moves.push_back(move);
            else if (team != 1 && check_legal(move))
                black_possible_moves.push_back(move);

            if (piece * team < 0) break; // breaking after pushing the position of exactly one enemy piece.
            target_x += x_offsets[i]; target_y += y_offsets[i];
        }
    }
}


void GameState::king_moves(int x, int y, int team) {
    int x_offsets[] = { 1, 1, 1, -1, -1, -1, 0, 0 };
    int y_offsets[] = { -1, 0, 1, -1, 0, 1, 1, -1 };


    // Castling:
    //      a b c d e f g h
    // 
    //  8   r n b q k b n r
    //  7   p p p p p p p p
    //  6   . . . . . . . .
    //  5   . . . . . . . .
    //  4   . . . . . . . .
    //  3   . . . . . . . .
    //  2   P P P P P P P P
    //  1   R N B Q K B N R
    // BQ -> a8, BK -> h8, WQ -> a1, WK -> h1

    for (int i = 0; i < 8; i++) {
        int target_x = x + x_offsets[i], target_y = y + y_offsets[i];
        int piece = board[target_x][target_y];
        if (in_board(target_x, target_y) && piece * team <= 0) {
            Move move(x, y, target_x, target_y, Move::None, (piece != 0));
            if (team == 1 && check_legal(move))
                white_possible_moves.push_back(move);
            else if (team != 1 && check_legal(move))
                black_possible_moves.push_back(move);
        }
    }


    if (team == 1) {
        if (!board[7][1] && !board[7][2] && !board[7][3] && canCastle(WQueenSide))
            if (!checked(7, 3, 1) && !checked(x, y, 1)) {
                Move move(x, y, 7, 2, Move::Castling);
                if (check_legal(move))
                    white_possible_moves.push_back(move);
            }
        if (!board[7][5] && !board[7][6] && canCastle(WKingSide))
            if (!checked(7, 5, 1) && !checked(x, y, 1)) {
                Move move(x, y, 7, 6, Move::Castling);
                if (check_legal(move))
                    white_possible_moves.push_back(move);
            }
    }
    else {
        if (!board[0][1] && !board[0][2] && !board[0][3] && canCastle(BQueenSide)) {
            if (!checked(0, 3, -1) && !checked(x, y, -1)) {
                Move move(x, y, 0, 2, Move::Castling);
                if (check_legal(move))
                    black_possible_moves.push_back(move);
            }
        }
        if (!board[0][5] && !board[0][6] && canCastle(BKingSide)) {
            if (!checked(0, 5, -1) && !checked(x, y, -1)) {
                Move move(x, y, 0, 6, Move::Castling);
                if (check_legal(move))
                    black_possible_moves.push_back(move);
            }
        }
    }
}


void GameState::knight_moves(int x, int y, int team) {
    int x_offsets[] = { -2, -1, -2, -1, 1, 1, 2, 2 };
    int y_offsets[] = { 1, 2, -1, -2, -2, 2, -1, 1 };

    for (int i = 0; i < 8; i++) {
        int target_x = x + x_offsets[i], target_y = y + y_offsets[i];
        int piece = board[target_x][target_y];
        if (in_board(target_x, target_y) && piece * team <= 0) {
            Move move(x, y, target_x, target_y, Move::None, (piece != 0));
            if (team == 1 && check_legal(move))
                white_possible_moves.push_back(move);
            else if (team != 1 && check_legal(move))
                black_possible_moves.push_back(move);
        }
    }

}


void GameState::bishop_moves(int x, int y, int team) {
    int x_offsets[] = { 1, 1, -1, -1 }; // defining an array of offsets like the rook function but
    int y_offsets[] = { 1, -1, 1, -1 }; // in the four diagonal directions.

    for (int i = 0; i < 4; i++) {
        int target_x = x + x_offsets[i], target_y = y + y_offsets[i];
        while (in_board(target_x, target_y)) {
            int piece = board[target_x][target_y];
            if (piece * team > 0) break;

            Move move(x, y, target_x, target_y, Move::None, (piece != 0));
            if (team == 1 && check_legal(move))
                white_possible_moves.push_back(move);
            else if (team != 1 && check_legal(move))
                black_possible_moves.push_back(move);

            if (piece * team < 0) break;
            target_x += x_offsets[i]; target_y += y_offsets[i];
        }
    }
}


void GameState::queen_moves(int x, int y, int team) {
    int x_offsets[] = { 1, 1, -1, -1, 0, 0, 1, -1 };
    int y_offsets[] = { 1, -1, 1, -1, 1, -1, 0, 0 };

    for (int i = 0; i < 8; i++) {
        int target_x = x + x_offsets[i], target_y = y + y_offsets[i];
        while (in_board(target_x, target_y)) {
            int piece = board[target_x][target_y];
            if (piece * team > 0) break;

            Move move(x, y, target_x, target_y, Move::None, (piece != 0));
            if (team == 1 && check_legal(move))
                white_possible_moves.push_back(move);
            else if (team != 1 && check_legal(move))
                black_possible_moves.push_back(move);

            if (piece * team < 0) break;
            target_x += x_offsets[i]; target_y += y_offsets[i];
        }
    }
}


// Matches the type of a specific piece and calls it's moves generation function.
void GameState::generate_piece_moves(int x, int y, int team, int type) {
    if (type == 1)
        king_moves(x, y, team);
    else if (type == 2)
        queen_moves(x, y, team);
    else if (type == 3)
        rook_moves(x, y, team);
    else if (type == 4)
        knight_moves(x, y, team);
    else if (type == 5)
        bishop_moves(x, y, team);
    else if (type == 6)
        pawn_moves(x, y, team);
}

// Loops over the board to generate all the possible moves for each piece storing it 
// in the object's white_possible_moves or black_possible_moves.
void GameState::generate_all_possible_moves(int team) {
    // White -> 1 , Black -> -1
    // If the parameter type == 1 then it will only generate moves for white
    // if it was -1 then it will only generate moves for black.

    if (team != player) runtime_error("Can generate possible moves only for the current player");

    if (team == 1) white_possible_moves.clear();
    else black_possible_moves.clear();

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            int type = board[i][j];
            if (board[i][j] > 0 && player == 1) {
                generate_piece_moves(i, j, 1, type);
            }
            else if (board[i][j] < 0 && player != 1) {
                generate_piece_moves(i, j, -1, abs(type));
            }
        }
    }
}

// A function used for testing and debugging.
void GameState::display_possible_moves() {
    myVector<Move> possible;
    if (player == 1) possible = white_possible_moves;
    else  possible = black_possible_moves;
    cout << "Possible moves: " << endl;
    for (int i = 0; i < possible.size(); i++) {
        Move move = possible[i];
        cout << to_algebraic(move.FromX(), move.FromY(), move.ToX(), move.ToY()) << endl;
    }
}


// Checks if this position is threatened by an enemy piece.
bool GameState::checked(int kingx, int kingy, int type) {
    // Checking if a knight threatens the king.
    int knight_x_offsets[] = { -2, -1, -2, -1, 1, 1, 2, 2 };
    int knight_y_offsets[] = { 1, 2, -1, -2, -2, 2, -1, 1 };

    for (int i = 0; i < 8; i++) {
        int tx = kingx + knight_x_offsets[i];
        int ty = kingy + knight_y_offsets[i];
        if (in_board(tx, ty) && board[tx][ty] * type == -4) {
            return 1;
        }
    }

    int x_offsets[] = { 0, 0, 1, -1, 1, 1, -1, -1 };
    int y_offsets[] = { 1, -1, 0, 0, 1, -1, 1, -1 };

    // Checking if a sliding piece (Rook, Queen, Bishop) is attacking the king vertically or horizontally.
    for (int i = 0; i < 4; i++) {
        int tx = kingx + x_offsets[i], ty = kingy + y_offsets[i];
        while (in_board(tx, ty)) {
            int piece = board[tx][ty];
            if (piece * type == -3 || piece * type == -2) {
                return 1;
            }
            if (piece != 0) break;
            tx += x_offsets[i]; ty += y_offsets[i];
        }
    }

    // Checking if a sliding piece (Rook, Queen, Bishop) is attacking the king diagonally.
    for (int i = 4; i < 8; i++) {
        int tx = kingx + x_offsets[i], ty = kingy + y_offsets[i];
        while (in_board(tx, ty)) {
            int piece = board[tx][ty];
            if (piece * type == -5 || piece * type == -2) {
                return 1;
            }
            if (piece != 0) break;
            tx += x_offsets[i]; ty += y_offsets[i];
        }
    }

    // Checking for the other king.
    for (int i = 0; i < 8; i++) {
        int tx = kingx + x_offsets[i], ty = kingy + y_offsets[i];
        if (in_board(tx, ty) && board[tx][ty] * type == -1) return 1;
    }

    // Checking for pawns.
    if (type > 0) {
        if (in_board(kingx - 1, kingy - 1) && board[kingx - 1][kingy - 1] == -6) { return 1; }
        if (in_board(kingx - 1, kingy + 1) && board[kingx - 1][kingy + 1] == -6) { return 1; }
    }
    else {
        if (in_board(kingx + 1, kingy - 1) && board[kingx + 1][kingy - 1] == 6) { return 1; }
        if (in_board(kingx + 1, kingy + 1) && board[kingx + 1][kingy + 1] == 6) { return 1; }
    }

    return 0;
}

// Update the internal representation of the board inside the GameState object 
// while handling special moves like en passants, castling and promotions.
void GameState::makeMove(Move& move) {
    gameStateHistory.push_back(currentGameState);
    zobristKeys.push_back(zobristKey);

    int fromX = move.FromX(), fromY = move.FromY();
    int toX = move.ToX(), toY = move.ToY();
    int pieceToMove = board[fromX][fromY], targetPiece = board[toX][toY];

    currentGameState &= ~(63U << 4); // Clearing the enPassant bits.

    // Reassigning captured piece.
    currentGameState &= ~(15U << 10);
    currentGameState |= (abs(targetPiece) << 10);
    if (targetPiece > 0) currentGameState |= (1 << 13);

    board[fromX][fromY] = 0;
    board[toX][toY] = pieceToMove;

    // Updating the zobrist key.
    if (pieceToMove > 0) {
        zobristKey ^= table->pieceKeys[0][pieceToMove][fromX][fromY];
        zobristKey ^= table->pieceKeys[0][pieceToMove][toX][toY];
    }
    else {
        zobristKey ^= table->pieceKeys[1][abs(pieceToMove)][fromX][fromY];
        zobristKey ^= table->pieceKeys[1][abs(pieceToMove)][toX][toY];
    }

    if (targetPiece > 0) {
        zobristKey ^= table->pieceKeys[0][targetPiece][toX][toY];
    }
    else if (targetPiece < 0) {
        zobristKey ^= table->pieceKeys[1][abs(targetPiece)][toX][toY];
    }

    
    // Changing the position of the white and black king used for O(1) access to king positions
    // and changing castling rights if king or rook moved.
    if (pieceToMove == 1) {
        currentGameState &= ~(WQueenSide | WKingSide);
        white_king = { toX, toY };
    }
    else if (pieceToMove == -1) {
        currentGameState &= ~(BQueenSide | BKingSide);
        black_king = { toX, toY };
    }
    else if (pieceToMove == -3){
        if (fromY == 0 && fromX == 0) currentGameState &= ~(BQueenSide);
        else if (fromY == 7 && fromX == 0) currentGameState &= ~(BKingSide);
    }
    else if (pieceToMove == 3) {
        if (fromY == 0 && fromX == 7) currentGameState &= ~(WQueenSide);
        else if (fromY == 7 && fromX == 7) currentGameState &= ~(WKingSide);
    }

    if (move.IsPromotion()) {
        board[toX][toY] = 2 * player;
        if (player == 1) {
            zobristKey ^= table->pieceKeys[0][6][toX][toY];
            zobristKey ^= table->pieceKeys[0][2][toX][toY];
        }
        else {
            zobristKey ^= table->pieceKeys[1][6][toX][toY];
            zobristKey ^= table->pieceKeys[1][2][toX][toY];
        }
    }
    else if (move.IsCastle()) {
        // Flag the kings and rooks as moved to make them lose castling rights
        if (toX == 0 && toY == 2) {
            swap(board[0][0], board[0][3]);     
            zobristKey ^= table->pieceKeys[1][3][0][0];
            zobristKey ^= table->pieceKeys[1][3][0][3];
        }
        else if (toX == 0 && toY == 6) {
            swap(board[0][7], board[0][5]);
            zobristKey ^= table->pieceKeys[1][3][0][7];
            zobristKey ^= table->pieceKeys[1][3][0][5];
        }
        else if (toX == 7 && toY == 2) {
            swap(board[7][0], board[7][3]);
            zobristKey ^= table->pieceKeys[0][3][7][0];
            zobristKey ^= table->pieceKeys[0][3][7][3];
        }
        else if (toX == 7 && toY == 6) {
            swap(board[7][7], board[7][5]);
            zobristKey ^= table->pieceKeys[0][3][7][7];
            zobristKey ^= table->pieceKeys[0][3][7][5];
        }
    }
    else if (move.IsPawnTwoMoves()) {
        // Check if the move is a pawn who moved twice to flag
        // an en passant as an available move.
        if (player == 1) {
            currentGameState |= ((toX + 1U) << 4); // Flagging the sqaure behind the pawn as open for en passant.
            currentGameState |= (toY << 7); 
        }
        else {
            currentGameState |= ((toX - 1U) << 4);
            currentGameState |= (toY << 7);
        }

    }
    else if (move.IsEnPassant()) {
        if (player == 1) {
            board[toX + 1][toY] = 0;
            zobristKey ^= table->pieceKeys[1][6][toX + 1][toY];
        }
        else {
            board[toX - 1][toY] = 0;
            zobristKey ^= table->pieceKeys[0][6][toX - 1][toY];
        }
    }

    player *= -1;
    zobristKey ^= table->blackToMove;
}

void GameState::unMakeMove(Move& move) {
    int fromX = move.FromX(), fromY = move.FromY();
    int toX = move.ToX(), toY = move.ToY();
    int pieceToReturn = board[toX][toY], captured = capturedPiece();

    board[fromX][fromY] = pieceToReturn;
    board[toX][toY] = captured;

    player *= -1;

    if (pieceToReturn == 1) {
        white_king = { fromX, fromY };
    }
    else if (pieceToReturn == -1) {
        black_king = { fromX, fromY };
    }

    if (move.IsPromotion()) {
        board[fromX][fromY] = 6 * player;
    }
    else if (move.IsCastle()) {
        if (toX == 0 && toY == 2) {
            swap(board[0][0], board[0][3]);
        }
        else if (toX == 0 && toY == 6) {
            swap(board[0][7], board[0][5]);
        }
        else if (toX == 7 && toY == 2) {
            swap(board[7][0], board[7][3]);
        }
        else if (toX == 7 && toY == 6) {
            swap(board[7][7], board[7][5]);
        }
    }
    else if (move.IsEnPassant()) {
        if (player == 1) {
            board[toX + 1][toY] = -6;
        }
        else {
            board[toX - 1][toY] = 6;
        }
    }

    currentGameState = gameStateHistory[gameStateHistory.size() - 1];
    gameStateHistory.pop_back();

    zobristKey = zobristKeys[zobristKeys.size() - 1];
    zobristKeys.pop_back();
}


// checks if the move is legal by simulating a move and checking for checkMates.
bool GameState::check_legal(Move& move) { 
    int kingx, kingy, currentPlayer = player;

    makeMove(move);

    if (currentPlayer == 1) {
        kingx = white_king.first;
        kingy = white_king.second;
    }
    else {
        kingx = black_king.first;
        kingy = black_king.second;
    }

    bool isLegal = true;
    if (checked(kingx, kingy, currentPlayer)) isLegal = false;

    unMakeMove(move);

    return isLegal;
}

// Checks if the given player has no moves and the king is checked meaning a checkmate.
bool GameState::checkMate(int team) {

    if (team == 1 && player == 1) {
        if (white_possible_moves.empty() && checked(white_king.first, white_king.second, 1))
            return 1;
    }
    else if (team == -1 && player == -1){
        if (black_possible_moves.empty() && checked(black_king.first, black_king.second, -1))
            return 1;
    }
    return 0;
}

bool GameState::staleMate(int team) {
    if (team == 1 && player == 1) {
        if (white_possible_moves.empty() && !checked(white_king.first, white_king.second, 1)) return 1;
    }
    if (team == -1 && player == -1) {
        if (black_possible_moves.empty() && !checked(black_king.first, black_king.second, -1)) return 1;
    }
    return 0;
}


// Used for debugging.
string GameState::show() {
    string output = "";
    output += "   ";
    for (int i = 0; i < 8; i++)
        output += to_string('a' + i) + " ";
    output += "\n\n";
    for (int i = 0, z = 8; i < 8; i++, z--) {
        output += z + "  ";
        for (int j = 0; j < 8; j++) {
            output += to_string(match_to_char(board[i][j])) + " ";
        }
        output += '\n';
    }
    return output;
}

myPair<int, int> GameState::enPassant() {
    int x = (currentGameState >> 4) & 7;
    int y = (currentGameState >> 7) & 7;

    return { x, y };
}

bool GameState::canCastle(uint16_t side) {
    return (currentGameState & side);
}

int GameState::capturedPiece() {
    int type = (currentGameState >> 10) & 7;
    // The 13th bit contains the color of the captured piece. W->1, B->0
    if ((currentGameState >> 13) & 1) return type;
    else return -type;
}

// Given the indices in the board finds the corresponding move which contains
// additional information. like, if it was a castling move, en Passant etc...
Move GameState::findMove(int fromX, int fromY, int toX, int toY) {
    myVector<Move> possible = (board[fromX][fromY] > 0) ? white_possible_moves : black_possible_moves;
    for (int i = 0; i < possible.size(); i++) {
        Move move = possible[i];
        if (move.FromX() == fromX && move.FromY() == fromY && move.ToX() == toX && move.ToY() == toY) 
            return move;
    }
}


Minimax::Minimax(TranspositionTable& Ttable) : table(&Ttable) {}


bool Minimax::timeLimitExceeded(chrono::steady_clock::time_point& start, chrono::milliseconds& duration, int& depth) {
    auto current_time = chrono::steady_clock::now();
    duration = chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
    if (duration.count() > time_limit && depth > least_depth) return true;
    return false;
}

int Minimax::get_pcsq_value(int x, int y, int piece, bool endgame) {
    int pos = x * 8 + y;
    switch (abs(piece)) {
    case 6:
        if (endgame) {
            if (piece > 0) return eg_pawn_table[pos];
            else return -eg_pawn_table[flip[pos]];
        }
        else {
            if (piece > 0) return mg_pawn_table[pos];
            else return -mg_pawn_table[flip[pos]];
        }
    case 5:
        if (endgame) {
            if (piece > 0) return eg_bishop_table[pos];
            else return -eg_bishop_table[flip[pos]];
        }
        else {
            if (piece > 0) return mg_bishop_table[pos];
            else return -mg_bishop_table[flip[pos]];
        }
    case 4:
        if (endgame) {
            if (piece > 0) return eg_knight_table[pos];
            else return -eg_knight_table[flip[pos]];
        }
        else {
            if (piece > 0) return mg_knight_table[pos];
            else return -mg_knight_table[flip[pos]];
        }
    case 3:
        if (endgame) {
            if (piece > 0) return eg_rook_table[pos];
            else return -eg_rook_table[flip[pos]];
        }
        else {
            if (piece > 0) return mg_rook_table[pos];
            else return -mg_rook_table[flip[pos]];
        }
    case 2:
        if (endgame) {
            if (piece > 0) return eg_queen_table[pos];
            else return -eg_queen_table[flip[pos]];
        }
        else {
            if (piece > 0) return mg_queen_table[pos];
            else return -mg_queen_table[flip[pos]];
        }
    case 1:
        if (endgame) {
            if (piece > 0) return eg_king_table[pos];
            else return -eg_king_table[flip[pos]];
        }
        else {
            if (piece > 0) return mg_king_table[pos];
            else return -mg_king_table[flip[pos]];
        }
    }
}

int Minimax::evaluate_pawns(int team, int white_pawns_row[], int black_pawns_row[]) {
    int num_isolated = 0;
    int penalty = 0, bonus = 0;

    for (int i = 0; i < 8; i++) {
        int row_white = white_pawns_row[i], row_black = black_pawns_row[i];
        if (team == 1 && row_white != -1) {
            // Isolated pawns.
            if ((i == 0 || white_pawns_row[i - 1] == -1) && (i == 7 || white_pawns_row[i + 1] == -1)) {
                num_isolated++;
            }

            // Passed pawns.
            bool noOpposingPawns = true;
            if (i > 0 && black_pawns_row[i - 1] < row_white && black_pawns_row[i - 1] != -1) noOpposingPawns = false;
            if (i < 7 && black_pawns_row[i + 1] < row_white && black_pawns_row[i + 1] != -1) noOpposingPawns = false;
            if (black_pawns_row[i] < row_white && black_pawns_row[i] != -1) noOpposingPawns = false;

            if (noOpposingPawns) {
                bonus += passedPawnBonuses[row_white];
            }
        }
        else if (team == -1 && black_pawns_row[i] != -1) {
            if ((i == 0 || black_pawns_row[i - 1] == -1) && (i == 7 || black_pawns_row[i + 1] == -1)) {
                num_isolated++;
            }

            bool noOpposingPawns = true;
            if (i > 0 && white_pawns_row[i - 1] > row_black) noOpposingPawns = false;
            if (i < 7 && white_pawns_row[i + 1] > row_black) noOpposingPawns = false;
            if (white_pawns_row[i] > row_black) noOpposingPawns = false;

            if (noOpposingPawns) {
                bonus -= passedPawnBonuses[7 - row_black];
            }
        }
    }

    penalty = isolatedPawnPenaltyByCount[num_isolated];
    penalty = (team == 1) ? penalty : -penalty;

    return penalty + bonus;
}

int Minimax::evaluation(GameState& state) {
    int mgEval = 0;
    int egEval = 0;
    int gamePhase = 0;

    int black_pawns_row[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    int white_pawns_row[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };

    int minor_piece_counter = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            int piece = state.board[i][j];
            if (piece != 0) {
                if (piece == 6) white_pawns_row[j] = i;
                else if (piece == -6) black_pawns_row[j] = i;

                int mgPieceVal = mgValue[abs(piece)], egPieceVal = egValue[abs(piece)];

                if (piece < 0) {
                    mgPieceVal *= -1;
                    egPieceVal *= -1;
                }

                mgEval += get_pcsq_value(i, j, piece, false) + mgPieceVal;
                egEval += get_pcsq_value(i, j, piece, true) + egPieceVal;
                gamePhase += gamephaseInc[abs(piece)];
            }
        }
    }

    int pawnStructure = 0;

    pawnStructure += evaluate_pawns(1, white_pawns_row, black_pawns_row);
    pawnStructure += evaluate_pawns(-1, white_pawns_row, black_pawns_row);

    int mgPhase = min(gamePhase, 24);
    int egPhase = 24 - mgPhase;

    int eval = (mgEval * mgPhase + egEval * egPhase) / 24 + pawnStructure;
    return (state.player == 1) ? eval : -eval;
}


int Minimax::minimax(GameState& state, int plyRemaining, int depth, int alpha, int beta) {
    int plyFromRoot = depth - plyRemaining;
    node_counter++;

    if (plyRemaining == 0) {
        int eval = quiescenceSearch(state, quiescenceMaxDepth, depth, alpha, beta);
        //int eval = evaluation(state);
        return eval;
    }

    bool positionInTable = false;

    int transpositionValue = table->lookupEvaluation(state.zobristKey, plyRemaining, alpha, beta, positionInTable, false);

    if (positionInTable) {
        if (plyFromRoot == 0) {
            Transposition pos;
            table->probeTransposition(state.zobristKey, pos);
            if (!(abs(pos.value) > 1e9 && pos.IsQuiscence())) {
                bestMoveThisIteration = pos.move;
                bestScoreThisIteration = pos.value;
            }
        }
        tableUses++;
        return transpositionValue;
    }

    state.generate_all_possible_moves(state.player);
    myVector<Move> moves = (state.player == 1) ? state.white_possible_moves : state.black_possible_moves;
    // Move ordering have proven to be very effective even with that simple heuristic (MVV-LVA)
    // especially in quiescence search. i really didn't expect it to make that much of a difference but it does.
    moveOrderer.sortMoves(moves, state.board);

    if (moves.empty()) {
        myPair<int, int> king = (state.player == 1) ? state.white_king : state.black_king;
        if (state.checked(king.first, king.second, state.player)) {
            return INT_MIN + 2;
        }
        else return 0;
    }

    uint8_t evaluationBound = Transposition::Alpha;
    Move bestMoveInPos = moves[0];

    for (int i = 0; i < moves.size(); i++) {

        state.makeMove(moves[i]);
        int score = -minimax(state, plyRemaining - 1, depth, -beta, -alpha);
        state.unMakeMove(moves[i]);

        // Break if the time limit was exceeded.
        if (timeLimitExceeded(start_time, duration, depth)) { 
            broke_early = true; 
            return 0; 
        }

        // A Beta-cutoff meaning the opponent won't choose this move as they have a better option.
        if (score >= beta) {
            table->storeTransposition(state.zobristKey, Transposition::Beta, plyRemaining, beta, moves[i]);
            return beta;
        }

        // Found a new move that is better than the current best.
        if (score > alpha) {
            alpha = score;
            evaluationBound = Transposition::Exact;
            bestMoveInPos = moves[i];

            // Saves the moves to be sorted and assigned to best move after the search is finished.
            if (plyFromRoot == 0) {
                bestMoveThisIteration = moves[i];
                bestScoreThisIteration = score;
            }
        }
    }

    if (abs(alpha) > 1e9) alpha = (alpha > 0) ? alpha - 1 : alpha + 1;
    table->storeTransposition(state.zobristKey, evaluationBound, plyRemaining, alpha, bestMoveInPos);
    return alpha;
}

Move Minimax::iterative_deepening(GameState& state) {
    node_counter = 0, Q_nodes = 0; bestScore = INT_MIN + 1, bestScoreThisIteration + INT_MIN + 1, tableUses = 0;
    start_time = chrono::steady_clock::now();
    state.generate_all_possible_moves(state.player);

    if (state.player == 1) bestMoveThisIteration = state.white_possible_moves[0];
    else bestMoveThisIteration = state.black_possible_moves[0];

    int depth = 1; broke_early = false;

    while (depth <= 255) {
        int score = minimax(state, depth, depth, INT_MIN + 1, INT_MAX);

        if (timeLimitExceeded(start_time, duration, depth)) { broke_early = true; }

        if (bestScoreThisIteration > 1e9) {
            broke_early = true;
            bestMove = bestMoveThisIteration;
            bestScore = bestScoreThisIteration;
        }

        if (!broke_early) {
            bestMove = bestMoveThisIteration;
            bestScore = bestScoreThisIteration;
        }
        else{
            time_in_seconds = duration.count() / 1000.0;
            break;
        }
        depth++;
    }
    reached_depth = depth - broke_early;
    return bestMove;
}


int Minimax::quiescenceSearch(GameState& state, int plyRemaining, int mainSearchDepth, int alpha, int beta) {
    int staticEval = evaluation(state);
    Q_nodes++;
    node_counter++;

    if (plyRemaining == 0) return staticEval;

    if (staticEval >= beta) {
        return beta;
    }
    if (staticEval > alpha) {
        alpha = staticEval;
    }

    bool positionInTable = false;

    int transpositionValue = table->lookupEvaluation(state.zobristKey, plyRemaining, alpha, beta, positionInTable, true);

    if (positionInTable) {
        tableUses++;
        return transpositionValue;
    }

    state.generate_all_possible_moves(state.player);
    myVector<Move> moves = (state.player == 1) ? state.white_possible_moves : state.black_possible_moves;
    moveOrderer.sortMoves(moves, state.board);

    if (moves.empty()) {
        myPair<int, int> king = (state.player == 1) ? state.white_king : state.black_king;
        if (state.checked(king.first, king.second, state.player)) {
            return INT_MIN + 2;
        }
        else return 0;
    }

    uint8_t evaluationBound = Transposition::QAlpha;
    Move bestMoveInPos = moves[0];

    for (int i = 0; i < moves.size(); i++) {

        if (!moves[i].IsCapture()) continue;

        state.makeMove(moves[i]);
        int score = -quiescenceSearch(state, plyRemaining - 1, mainSearchDepth, -beta, -alpha);
        state.unMakeMove(moves[i]);

        if (score >= beta) {
            table->storeTransposition(state.zobristKey, Transposition::QBeta, plyRemaining, beta, moves[i]);
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            evaluationBound = Transposition::QExact;
            bestMoveInPos = moves[i];
        }
    }

    if (abs(alpha) > 1e9) alpha = (alpha > 0) ? alpha - 1 : alpha + 1;
    table->storeTransposition(state.zobristKey, evaluationBound, plyRemaining, alpha, bestMoveInPos);
    return alpha;
}


string Minimax::displayStatistics(GameState& state) {
    Move move = bestMove;
    string output = "";
    output += "Best Move: " + to_algebraic(move.FromX(), move.FromY(), move.ToX(), move.ToY()) + " ";
    if (abs(bestScore) > 1e9) {
        //cout << bestScore << "   " << bestScoreThisIteration << "    " << (abs(INT_MAX - abs(bestScore))) + 1 << endl;
        if ((abs(INT_MAX - abs(bestScore)) - 1) / 2 > 0)
            output += "Mate In: " + to_string((abs(INT_MAX - abs(bestScore)) - 1) / 2);
        else
            output += "Checkmate! ";
        if (bestScore > 1e9) {
            if (state.player == 1)
                output += " For White.\n";
            else 
                output += " For Black.\n";
        }
        else {
            if (state.player == 1)
                output += " For Black.\n";
            else
                output += " For White.\n";
        }
    }
    else {
        output += to_string(bestScore) + '\n';
    }

    
    output += "Nodes Evaluated: " + to_string(node_counter) + '\n';
    output += "Quiescent Nodes: " + to_string(Q_nodes) + '\n';
    output += "Depth Reached: " + to_string(reached_depth) + '\n';
    output += "Time Taken: " + to_string(time_in_seconds) + '\n';
    output += "Table uses: " + to_string(tableUses) + '\n';

    return output;
}

void Minimax::setTimeLimit(int time) {
    time_limit = time;
}



int node_counter = 0, capture_counter = 0, check_counter = 0, EP_counter = 0, promotion_counter = 0, castle_counter = 0;

void perftResults(GameState& state, int depth, int startDepth) {
    if (depth == 0) {
        node_counter++;
        return;
    }

    if (state.player == 1) {
        state.generate_all_possible_moves(1);
        myVector<Move> Possible = state.white_possible_moves;
        for (int i = 0; i < Possible.size(); i++) {
            Move move = Possible[i];
            state.makeMove(move);

            if (depth == 1) {
                if (state.capturedPiece() != 0) {
                    capture_counter++;
                }

                if (move.IsCastle()) castle_counter++;
                if (move.IsEnPassant()) { EP_counter++; capture_counter++; }
                if (move.IsPromotion()) promotion_counter+=4;


                if (state.checked(state.black_king.first, state.black_king.second, -1)) {
                    check_counter++;
                }
            }

            perftResults(state, depth - 1, startDepth);
            state.unMakeMove(move);
        }
    }
    else {
        state.generate_all_possible_moves(-1);
        myVector<Move> Possible = state.black_possible_moves;
        for (int i = 0; i < Possible.size(); i++) {
            Move move = Possible[i];
            state.makeMove(move);

            if (depth == 1) {
                if (state.capturedPiece() != 0) {
                    capture_counter++;
                }

                if (move.IsCastle()) castle_counter++;
                if (move.IsEnPassant()) { EP_counter++; capture_counter++; }
                if (move.IsPromotion()) promotion_counter+=4;


                if (state.checked(state.white_king.first, state.white_king.second, 1)) {
                    check_counter++;
                }
            }

            perftResults(state, depth - 1, startDepth);
            state.unMakeMove(move);;
        }
    }

    if (depth == startDepth) {
        cerr << "Depth: " << startDepth << endl;
        cerr << "Nodes: " << node_counter << endl;
        cerr << "Captures: " << capture_counter << endl;
        cerr << "EP: " << EP_counter << endl;
        cerr << "Castles: " << castle_counter << endl;
        cerr << "Promotions: " << promotion_counter << endl;
        cerr << "Checks: " << check_counter << endl;
        node_counter = 0, capture_counter = 0, check_counter = 0, EP_counter = 0, promotion_counter = 0, castle_counter = 0;
    }
}