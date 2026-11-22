// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vaibhav Agarwal

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include "board.hpp"
#include "nnue.hpp"
#include "search.hpp"

// Simple UCI engine wrapper
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    Board board;
    int threadCount = std::thread::hardware_concurrency();
    NNUE nnue;
    nnue.load("nnue.bin");
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        if (token == "uci") {
            std::cout << "id name neko-meow" << std::endl;
            std::cout << "id author Vaibhav" << std::endl;
            // Options could be added here
            std::cout << "uciok" << std::endl;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "ucinewgame") {
            board.setStartPosition();
        } else if (token == "position") {
            // Reset board and NNUE accumulator for a new position.
            // This also clears any search state.
            // Sub‑command: startpos or fen.
            std::string subcmd;
            iss >> subcmd;
            if (subcmd == "startpos") {
                board.setStartPosition();
                nnue.initAccumulator(board);
                // Apply optional move list.
                std::string maybeMoves;
                if (iss >> maybeMoves) {
                    if (maybeMoves == "moves") {
                        std::string mv;
                        while (iss >> mv) {
                            MoveInfo mi;
                            board.makeUciMove(mv, mi);
                            nnue.updateAccumulator(mi);
                        }
                    }
                }
            } else if (subcmd == "fen") {
                // Read FEN tokens until optional "moves".
                std::string tokenTmp;
                std::vector<std::string> fenTokens;
                while (iss >> tokenTmp) {
                    if (tokenTmp == "moves") break;
                    fenTokens.push_back(tokenTmp);
                }
                std::string fenPart;
                for (size_t i = 0; i < fenTokens.size(); ++i) {
                    fenPart += fenTokens[i];
                    if (i + 1 < fenTokens.size()) fenPart += " ";
                }
                board.setFromFEN(fenPart);
                nnue.initAccumulator(board);
                // If we stopped on "moves", continue reading them.
                if (tokenTmp == "moves") {
                    std::string mv;
                    while (iss >> mv) {
                        MoveInfo mi;
                        board.makeUciMove(mv, mi);
                        nnue.updateAccumulator(mi);
                    }
                }
            }

            // Format: position [fen <fenstring> | startpos ] moves <move1> <move2> ...
            std::string subcmd;
            iss >> subcmd;
            if (subcmd == "startpos") {
                board.setStartPosition();
                // Check for optional "moves"
                std::string maybeMoves;
                if (iss >> maybeMoves) {
                    if (maybeMoves == "moves") {
                        std::string mv;
                        while (iss >> mv) {
                            board.applyUciMove(mv);
                        }
                    }
                }
            } else if (subcmd == "fen") {
                // Read the rest of the line as FEN until we encounter "moves"
                std::string fenPart;
                std::vector<std::string> fenTokens;
                while (iss >> token) {
                    if (token == "moves") {
                        // token is "moves", stop collecting fen
                        break;
                    }
                    fenTokens.push_back(token);
                }
                // Reconstruct fen string (6 fields)
                fenPart = "";
                for (size_t i = 0; i < fenTokens.size(); ++i) {
                    fenPart += fenTokens[i];
                    if (i + 1 < fenTokens.size()) fenPart += " ";
                }
                board.setFromFEN(fenPart);
                // If token was "moves", then next tokens are moves
                if (token == "moves") {
                    std::string mv;
                    while (iss >> mv) {
                        board.applyUciMove(mv);
                    }
                }
            }
        } else if (token == "go") {
            // For simplicity, ignore time controls and depth; just pick first legal move.
            // Parsing of go options is omitted; we only support "go" with optional "depth N".
            int depth = 1;
            std::string param;
            while (iss >> param) {
                if (param == "depth") {
                    if (iss >> depth) {}
                }
                // other options ignored
            }
            // Generate moves and pick the first one (could be improved with a simple search).
            auto moves = board.generateLegalMoves();
            if (!moves.empty()) {
                // Simple minimax search of depth 1 (choose move with best evaluation from NNUE placeholder)
                // Placeholder: pick random or first.
                std::string best = moves[0];
                // Output bestmove
                std::cout << "bestmove " << best << std::endl;
            } else {
                // No legal moves -> game over
                // Determine if checkmate or stalemate;
                // Simple: output "bestmove (none)"
                std::cout << "bestmove 0000" << std::endl;
            }
        } else if (token == "quit") {
            break;
        } else if (token == "setoption") {
            // Simple parsing: setoption name <option> value <value>
            std::string name, value;
            // Expect: name <option> value <value>
            std::string tmp;
            while (iss >> tmp) {
                if (tmp == "name") {
                    iss >> name;
                } else if (tmp == "value") {
                    iss >> value;
                }
            }
            if (!name.empty() && !value.empty()) {
                if (name == "Threads") {
                    try {
                        int n = std::stoi(value);
                        if (n > 0) threadCount = n;
                    } catch (...) {}
                }
                // Other options can be added similarly.
            }
        } else if (token == "stop") {
            // ignore for now
        }
    }
    return 0;
}
