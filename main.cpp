#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "pcsq.h"
#include "dataStructures.h"
#include "logic.h"
#include "TranspositionTable.h"

using namespace std;

struct Logger {
    ofstream logFile;
    static constexpr int maxSize = 100 * 1024; // 100 KB

    Logger(string& filename) {
        logFile.open(filename, ios::app);
        ifstream file(filename, std::ios::binary | std::ios::ate);

        long long fileSize = file.tellg();

        if (fileSize > maxSize) {
            clearLogFile(filename);
        }

        if (!logFile.is_open()) {
            cerr << "Failed to open log file: " << filename << endl;
        }
    }

    void clearLogFile(const std::string& filename) {
        std::ofstream clearFile(filename, std::ios::trunc);
        if (!clearFile.is_open()) {
            std::cerr << "Failed to clear log file: " << filename << std::endl;
            return;
        }
    }

    ~Logger() {
        logFile.close();
    }

    void log(string message) {
        if (logFile.is_open()) {
            logFile << message << endl;
        }
        else {
            cerr << "Log file is not open. Cannot write log: " << message << endl;
        }
    }
};

// Function to split strings (utility function)
myVector<string> split(const string& s, char delimiter) {
    myVector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool contains(string s, myVector<string>& tokens) {
    for (int i = 0; i < tokens.size(); i++) {
        if (s == tokens[i]) return true;
    }
    return false;
}

struct ChessEngine {
    GameState state;
    Minimax AI;
    TranspositionTable Ttable;
    Logger logger;

    ChessEngine (int sizeMB, string& filename) : Ttable(sizeMB) , AI(Ttable), logger(filename){
        state.initialize_board(Ttable);
    }

    void positionCommand(myVector<string>& tokens) {
        if (contains("startpos", tokens)) {
            state.initialize_board(Ttable);
            logger.log("Initialized board to new startpos and cleared Transposition Table");
        }
        else if (contains("fen", tokens)) {
            state.initialize_board(Ttable, tokens[2]);
            logger.log("Initialized board to new FEN" + tokens[2]);
        }
        else {
            cout << "Invalid command" << endl;
            string output = "Invalid command: ";
            for (int i = 0; i < tokens.size(); i++)
                output += tokens[i] + " ";
            logger.log(output);
        }

        if (contains("moves", tokens)) {
            for (int i = 3; i < tokens.size(); i++) {
                state.generate_all_possible_moves(state.player);
                myPair<int, int> from = to_index(tokens[i][0], tokens[i][1]);
                myPair<int, int> to = to_index(tokens[i][2], tokens[i][3]);
                Move move = state.findMove(from.first, from.second, to.first, to.second);
                state.makeMove(move);

                if (tokens[i].size() > 4) {
                    int piece = matchPieceType(state.board, tokens[i][4]);
                    state.board[to.first][to.second] = piece * state.player * -1;
                }
            }
        }
    }

    int matchPieceType(int board[8][8], char piece) {
        if (piece == 'q') return 2;
        else if (piece == 'b') return 5;
        else if (piece == 'b') return 3;
        else return 4;
    }

    void goCommand(myVector<string>& tokens) {
        // Start the search

        if (contains("movetime", tokens)) {
            int time = (stoi(tokens[2]) * 99) / 100;
            AI.setTimeLimit(time);
            string time_s = to_string(time);
            logger.log("Thinking for: " + time_s);
        }
        else {
            int wtime = stoi(tokens[2]), btime = stoi(tokens[4]);
            int winc = stoi(tokens[6]), binc = stoi(tokens[8]);
            int time = chooseThinkTime(wtime, btime, winc, binc);
            AI.setTimeLimit(time);
            logger.log("Thinking for: " + to_string(time));
        }

        state.generate_all_possible_moves(state.player);
        Move move = AI.iterative_deepening(state);
        string output = "bestmove " + to_algebraic(move.FromX(), move.FromY(), move.ToX(), move.ToY());
        if (move.IsPromotion()) output = output + 'q';
        cout << output << endl;
        string logs = "";
        logs += AI.displayStatistics(state);
        logs += Ttable.getFillData();
        state.makeMove(move);
        logger.log(output);
        logger.log(logs);
    }

    void uciLoop() {
        string input;
        while (getline(cin, input)) {
            logger.log("Recieved command: " + input);
            myVector<string> tokens = split(input, ' ');
            if (tokens[0] == "uci") {
                cout << "id name TheShadowEngine" << endl;
                cout << "id author Ismail Gamal" << endl;
                cout << "uciok" << endl;
                logger.log("Response: uciok" );
            }
            else if (tokens[0] == "isready"){
                logger.log("Response: readyok");
                cout << "readyok" << endl;
            }
            else if (tokens[0] == "ucinewgame") {
                Ttable.clear();
                state.initialize_board(Ttable);
            }
            else if (tokens[0] == "position") {
                positionCommand(tokens);
            }
            else if (tokens[0] == "go") {
                goCommand(tokens);
            }
            else if (input == "quit") {
                break;
            }
        }
    }

    int chooseThinkTime(int timeRemainingWhiteMs, int timeRemainingBlackMs, int incrementWhiteMs, int incrementBlackMs) {
        int myTimeRemainingMs = (state.player == 1) ? timeRemainingWhiteMs : timeRemainingBlackMs;
        int myIncrementMs = (state.player == 1) ? incrementWhiteMs : incrementBlackMs;

        // Get a fraction of remaining time to use for current move
        // this should smooth the time usage.
        int thinkTimeMs = myTimeRemainingMs / 40;

        if (myTimeRemainingMs > myIncrementMs * 2)
        {
            thinkTimeMs += (myIncrementMs * 7) / 10;
        }

        int minThinkTime = min(50, myTimeRemainingMs / 4);
        return max(minThinkTime, thinkTimeMs);
    }
};


int main()
{
    string logFileName = "log.txt";
    ChessEngine bot(400, logFileName);

    bot.uciLoop();

    return 0;
}