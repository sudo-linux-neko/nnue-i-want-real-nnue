// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vaibhav Agarwal

#pragma once

#include <string>
#include <cstdint>

// Representation of a single UCI move with enough information to undo it.
// All indices use the 0x88 board representation (0..127, invalid squares have the 0x88 mask set).
struct MoveInfo {
    int from = -1;                // source square index (0x88)
    int to = -1;                  // destination square index (0x88)
    char movingPiece = '.';      // piece character before the move (including case)
    char capturedPiece = '.';    // piece captured on destination (or en‑passant pawn), '.' if none
    char promotion = '\0';      // promotion piece character (uppercase for white, lowercase for black), '\0' if none
    bool isCastling = false;      // true if this move was a king‑side or queen‑side castle
    bool isEnPassant = false;    // true if this move captured en‑passant pawn
    // State that must be restored on unmake:
    bool whiteToMovePrev = true;
    bool castlingWhiteKingPrev = false;
    bool castlingWhiteQueenPrev = false;
    bool castlingBlackKingPrev = false;
    bool castlingBlackQueenPrev = false;
    int enPassantSquarePrev = -1;
    int halfmoveClockPrev = 0;
    int fullmoveNumberPrev = 1;
};
