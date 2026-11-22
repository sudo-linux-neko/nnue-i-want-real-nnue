// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vaibhav Agarwal

#pragma once

#include <string>
#include <array>
#include <cstdint>
#include "move.hpp"
#include <vector>

// Simple board representation for a chess engine.
// Uses a 0x88 board internally (0..127) for easy move generation.
// Empty squares are represented by '.' (dot).
// Uppercase letters for White pieces, lowercase for Black.
// Pieces: P,p N,n B,b R,r Q,q K,k
// The board array is indexed by 0x88 indices where squares with (idx & 0x88) == 0 are valid.

class Board {
public:
    // ... (same as before)
public:
    // 0x88 board representation
    std::array<char, 128> squares{}; // 0x88 used cells store piece chars, others unused.

    bool whiteToMove = true;
    bool castlingWhiteKing = false;   // K
    bool castlingWhiteQueen = false;  // Q
    bool castlingBlackKing = false;   // k
    bool castlingBlackQueen = false;  // q
    int enPassantSquare = -1; // 0x88 index of en-passant target square, -1 if none
    int halfmoveClock = 0;
    int fullmoveNumber = 1;

    Board() = default;
    // Set board to standard start position
    void setStartPosition();
    // Initialize board from a FEN string.
    bool setFromFEN(const std::string& fen);

    // Convert board to FEN string (for debugging)
    std::string getFEN() const;

    // Apply a UCI move like "e2e4" or "e7e8q" for promotion.
    bool applyUciMove(const std::string& move);
    // Apply a UCI move and record the full details needed to undo it.
    bool makeUciMove(const std::string& move, MoveInfo& outInfo);
    // Undo a move previously applied with makeUciMove.
    void unmakeMove(const MoveInfo& info);

    // Generate all legal moves for side to move.
    // Returns moves in UCI long algebraic format (e.g., e2e4, e7e8q).
    std::vector<std::string> generateLegalMoves() const;

private:
    static constexpr int8_t knightOffsets[8] = { -33, -31, -18, -14, 14, 18, 31, 33 };
    static constexpr int8_t bishopOffsets[4] = { -15, -17, 15, 17 };
    static constexpr int8_t rookOffsets[4] = { -16, -1, 1, 16 };
    static constexpr int8_t kingOffsets[8] = { -17, -16, -15, -1, 1, 15, 16, 17 };

    bool isSquareAttacked(int sq, bool attackerWhite) const;
    bool isInCheck(bool white) const;
    int algebraicToIndex(const std::string& sq) const;
    std::string indexToAlgebraic(int idx) const;
};
