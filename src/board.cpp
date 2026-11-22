// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vaibhav Agarwal

#include "board.hpp"
#include <sstream>
#include <cctype>
#include <vector>

// Helper to convert file/rank to 0x88 index
static int fileRankToIndex(int file, int rank) {
    // file: 0..7 (a..h), rank: 0..7 (1..8) where 0 is rank 1
    return (rank << 4) | file;
}

static std::pair<int,int> indexToFileRank(int idx) {
    return {idx & 0xF, idx >> 4};
}

bool Board::setFromFEN(const std::string& fen) {
    if(fen == "startpos") {
        setStartPosition();
        return true;
    }
    // Basic FEN parser: piece placement, active color, castling, en passant, halfmove, fullmove.
    // Reset board
    squares.fill('.');
    std::istringstream ss(fen);
    std::string piecePlacement, activeColor, castling, enPassant, halfmoveStr, fullmoveStr;
    if(!(ss >> piecePlacement >> activeColor >> castling >> enPassant >> halfmoveStr >> fullmoveStr))
        return false;

    // Parse pieces
    int idx = 0; // 0x88 index
    int rank = 7; // start from 8th rank (top)
    int file = 0;
    for(char c : piecePlacement) {
        if(c == '/') {
            rank--;
            file = 0;
            continue;
        }
        if(std::isdigit(c)) {
            file += c - '0';
        } else {
            int sq = fileRankToIndex(file, rank);
            squares[sq] = c;
            file++;
        }
    }
    // Active color
    whiteToMove = (activeColor == "w");
    // Castling rights
    castlingWhiteKing = castling.find('K') != std::string::npos;
    castlingWhiteQueen = castling.find('Q') != std::string::npos;
    castlingBlackKing = castling.find('k') != std::string::npos;
    castlingBlackQueen = castling.find('q') != std::string::npos;
    // En passant square
    if(enPassant != "-") {
        int file = enPassant[0] - 'a';
        int rank = enPassant[1] - '1';
        enPassantSquare = fileRankToIndex(file, rank);
    } else {
        enPassantSquare = -1;
    }
    // Halfmove & fullmove
    halfmoveClock = std::stoi(halfmoveStr);
    fullmoveNumber = std::stoi(fullmoveStr);
    return true;
}

void Board::setStartPosition() {
    // Set to standard chess starting position using FEN parser.
    // This calls setFromFEN with the full FEN string.
    setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

std::string Board::getFEN() const {
    std::ostringstream oss;
    for(int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for(int file = 0; file < 8; ++file) {
            int idx = fileRankToIndex(file, rank);
            char piece = squares[idx];
            if(piece == '.' || piece == '\0') {
                empty++;
            } else {
                if(empty) oss << empty;
                empty = 0;
                oss << piece;
            }
        }
        if(empty) oss << empty;
        if(rank) oss << '/';
    }
    oss << ' ' << (whiteToMove ? 'w' : 'b');
    oss << ' ';
    std::string castle;
    if(castlingWhiteKing) castle.push_back('K');
    if(castlingWhiteQueen) castle.push_back('Q');
    if(castlingBlackKing) castle.push_back('k');
    if(castlingBlackQueen) castle.push_back('q');
    if(castle.empty()) castle = "-";
    oss << castle;
    oss << ' ';
    if(enPassantSquare >= 0) {
        auto [f,r] = indexToFileRank(enPassantSquare);
        oss << static_cast<char>('a' + f) << static_cast<char>('1' + r);
    } else {
        oss << '-';
    }
    oss << ' ' << halfmoveClock << ' ' << fullmoveNumber;
    return oss.str();
}

int Board::algebraicToIndex(const std::string& sq) const {
    if(sq.size() != 2) return -1;
    int file = sq[0] - 'a';
    int rank = sq[1] - '1';
    if(file < 0 || file > 7 || rank < 0 || rank > 7) return -1;
    return fileRankToIndex(file, rank);
}

std::string Board::indexToAlgebraic(int idx) const {
    auto [file, rank] = indexToFileRank(idx);
    std::string s;
    s.push_back('a' + file);
    s.push_back('1' + rank);
    return s;
}

bool Board::applyUciMove(const std::string& move) {
    // Existing simple apply (used by legacy code). It does not record undo info.
    // This implementation is kept for compatibility; it does not handle all special cases the same way as makeUciMove.
    if(move.length() < 4) return false;
    std::string fromSq = move.substr(0,2);
    std::string toSq = move.substr(2,2);
    int fromIdx = algebraicToIndex(fromSq);
    int toIdx = algebraicToIndex(toSq);
    if(fromIdx < 0 || toIdx < 0) return false;
    char piece = squares[fromIdx];
    if(piece == '.' || piece == '\0') return false;
    squares[toIdx] = piece;
    squares[fromIdx] = '.';
    // Promotion handling (very simplistic)
    if(move.size() == 5) {
        char promo = move[4];
        if(whiteToMove) squares[toIdx] = std::toupper(promo);
        else squares[toIdx] = std::tolower(promo);
    }
    // Update en passant square (only for double pawn pushes)
    enPassantSquare = -1;
    if(piece == 'P' && (fromIdx >> 4) == 1 && (toIdx >> 4) == 3) {
        enPassantSquare = fromIdx + 16;
    } else if(piece == 'p' && (fromIdx >> 4) == 6 && (toIdx >> 4) == 4) {
        enPassantSquare = fromIdx - 16;
    }
    // Update castling rights if king or rook moves (simple version)
    if(piece == 'K') {
        castlingWhiteKing = false;
        castlingWhiteQueen = false;
        if(fromIdx == 0x74 && toIdx == 0x76) { // king side white castle
            squares[0x77] = '.'; squares[0x75] = 'R';
        } else if(fromIdx == 0x74 && toIdx == 0x72) { // queen side white castle
            squares[0x70] = '.'; squares[0x73] = 'R';
        }
    } else if(piece == 'k') {
        castlingBlackKing = false;
        castlingBlackQueen = false;
        if(fromIdx == 0x04 && toIdx == 0x06) { // black king side
            squares[0x07] = '.'; squares[0x05] = 'r';
        } else if(fromIdx == 0x04 && toIdx == 0x02) { // black queen side
            squares[0x00] = '.'; squares[0x03] = 'r';
        }
    } else if(piece == 'R') {
        if(fromIdx == 0x77) castlingWhiteKing = false;
        if(fromIdx == 0x70) castlingWhiteQueen = false;
    } else if(piece == 'r') {
        if(fromIdx == 0x07) castlingBlackKing = false;
        if(fromIdx == 0x00) castlingBlackQueen = false;
    }
    // Switch side to move and update clocks
    whiteToMove = !whiteToMove;
    if(piece == 'P' || piece == 'p' || squares[toIdx] != '.') {
        halfmoveClock = 0;
    } else {
        halfmoveClock++;
    }
    if(!whiteToMove) fullmoveNumber++;
    return true;
}

bool Board::makeUciMove(const std::string& move, MoveInfo& outInfo) {
    // Record previous state for undo.
    outInfo.whiteToMovePrev = whiteToMove;
    outInfo.castlingWhiteKingPrev = castlingWhiteKing;
    outInfo.castlingWhiteQueenPrev = castlingWhiteQueen;
    outInfo.castlingBlackKingPrev = castlingBlackKing;
    outInfo.castlingBlackQueenPrev = castlingBlackQueen;
    outInfo.enPassantSquarePrev = enPassantSquare;
    outInfo.halfmoveClockPrev = halfmoveClock;
    outInfo.fullmoveNumberPrev = fullmoveNumber;

    if(move.length() < 4) return false;
    std::string fromSq = move.substr(0,2);
    std::string toSq = move.substr(2,2);
    outInfo.from = algebraicToIndex(fromSq);
    outInfo.to = algebraicToIndex(toSq);
    if(outInfo.from < 0 || outInfo.to < 0) return false;

    char piece = squares[outInfo.from];
    if(piece == '.' || piece == '\0') return false;
    outInfo.movingPiece = piece;
    outInfo.capturedPiece = squares[outInfo.to]; // may be '.'
    outInfo.promotion = '\0';
    outInfo.isCastling = false;
    outInfo.isEnPassant = false;

    // Handle castling detection (king move of two squares).
    if(std::tolower(piece) == 'k' && std::abs((outInfo.to & 0xF) - (outInfo.from & 0xF)) == 2) {
        outInfo.isCastling = true;
    }

    // Apply the move on the board.
    // Basic move (including promotion handling).
    squares[outInfo.to] = piece;
    squares[outInfo.from] = '.';

    // Promotion handling.
    if(move.size() == 5) {
        char promo = move[4];
        outInfo.promotion = promo;
        if(whiteToMove) squares[outInfo.to] = std::toupper(promo);
        else squares[outInfo.to] = std::tolower(promo);
    }

    // En‑passant handling – detect if pawn moves diagonally to empty square and enPassantSquare matches target.
    if(std::tolower(piece) == 'p') {
        int diffFile = (outInfo.to & 0xF) - (outInfo.from & 0xF);
        int diffRank = (outInfo.to >> 4) - (outInfo.from >> 4);
        if(std::abs(diffFile) == 1 && std::abs(diffRank) == 1 && squares[outInfo.to] == '.' && outInfo.to == enPassantSquare) {
            // Capturing en‑passant pawn.
            outInfo.isEnPassant = true;
            int capturedPawnSq = outInfo.to + (whiteToMove ? -16 : 16);
            outInfo.capturedPiece = squares[capturedPawnSq];
            squares[capturedPawnSq] = '.'; // remove the pawn.
        }
    }

    // Update en‑passant square (double pawn push).
    enPassantSquare = -1;
    if(piece == 'P' && (outInfo.from >> 4) == 1 && (outInfo.to >> 4) == 3) {
        enPassantSquare = outInfo.from + 16;
    } else if(piece == 'p' && (outInfo.from >> 4) == 6 && (outInfo.to >> 4) == 4) {
        enPassantSquare = outInfo.from - 16;
    }

    // Castling movement of rook.
    if(outInfo.isCastling) {
        // Determine side.
        bool sideWhite = std::isupper(piece);
        if(sideWhite) {
            // White castling.
            if(outInfo.to == 0x76) { // king side
                squares[0x77] = '.'; // remove rook from h1
                squares[0x75] = 'R'; // place rook on f1
            } else if(outInfo.to == 0x72) { // queen side
                squares[0x70] = '.'; // remove rook from a1
                squares[0x73] = 'R'; // place rook on d1
            }
        } else {
            // Black castling.
            if(outInfo.to == 0x06) { // king side
                squares[0x07] = '.';
                squares[0x05] = 'r';
            } else if(outInfo.to == 0x02) { // queen side
                squares[0x00] = '.';
                squares[0x03] = 'r';
            }
        }
    }

    // Update castling rights if king or rook moved.
    if(piece == 'K') {
        castlingWhiteKing = false;
        castlingWhiteQueen = false;
    } else if(piece == 'k') {
        castlingBlackKing = false;
        castlingBlackQueen = false;
    } else if(piece == 'R') {
        if(outInfo.from == 0x77) castlingWhiteKing = false; // h1 rook
        if(outInfo.from == 0x70) castlingWhiteQueen = false; // a1 rook
    } else if(piece == 'r') {
        if(outInfo.from == 0x07) castlingBlackKing = false; // h8 rook
        if(outInfo.from == 0x00) castlingBlackQueen = false; // a8 rook
    }

    // Switch side to move.
    whiteToMove = !whiteToMove;
    // Update halfmove clock.
    if(piece == 'P' || piece == 'p' || outInfo.capturedPiece != '.' ) {
        halfmoveClock = 0;
    } else {
        halfmoveClock++;
    }
    // Update fullmove number.
    if(!whiteToMove) fullmoveNumber++;

    return true;
}

void Board::unmakeMove(const MoveInfo& info) {
    // Restore piece placement.
    squares[info.from] = info.movingPiece;
    squares[info.to] = info.capturedPiece;
    // If a promotion had occurred, the moving piece on the destination was a promoted piece.
    // The original pawn is stored in movingPiece, so restoring squares[info.from] is sufficient.
    // Restore rook position if castling.
    if(info.isCastling) {
        bool sideWhite = std::isupper(info.movingPiece);
        if(sideWhite) {
            if(info.to == 0x76) { // king side white
                squares[0x77] = 'R';
                squares[0x75] = '.';
            } else if(info.to == 0x72) { // queen side white
                squares[0x70] = 'R';
                squares[0x73] = '.';
            }
        } else {
            if(info.to == 0x06) { // king side black
                squares[0x07] = 'r';
                squares[0x05] = '.';
            } else if(info.to == 0x02) { // queen side black
                squares[0x00] = 'r';
                squares[0x03] = '.';
            }
        }
    }
    // Restore en‑passant capture pawn if applicable.
    if(info.isEnPassant) {
        int capturedPawnSq = info.to + (info.whiteToMovePrev ? 16 : -16);
        squares[capturedPawnSq] = info.capturedPiece;
        // The destination square was empty (already restored above).
    }
    // Restore flags.
    whiteToMove = info.whiteToMovePrev;
    castlingWhiteKing = info.castlingWhiteKingPrev;
    castlingWhiteQueen = info.castlingWhiteQueenPrev;
    castlingBlackKing = info.castlingBlackKingPrev;
    castlingBlackQueen = info.castlingBlackQueenPrev;
    enPassantSquare = info.enPassantSquarePrev;
    halfmoveClock = info.halfmoveClockPrev;
    fullmoveNumber = info.fullmoveNumberPrev;
}

    // Expect move length 4 (e2e4) or 5 (e7e8q) for promotion.
    if(move.length() < 4) return false;
    std::string fromSq = move.substr(0,2);
    std::string toSq = move.substr(2,2);
    int fromIdx = algebraicToIndex(fromSq);
    int toIdx = algebraicToIndex(toSq);
    if(fromIdx < 0 || toIdx < 0) return false;
    char piece = squares[fromIdx];
    if(piece == '.' || piece == '\0') return false;
    // Basic move, no validation.
    squares[toIdx] = piece;
    squares[fromIdx] = '.';

    // Handle pawn promotion
    if(move.size() == 5) {
        char promo = move[4];
        // promotion piece is given in lowercase for black, uppercase for white in UCI.
        if(whiteToMove) squares[toIdx] = std::toupper(promo);
        else squares[toIdx] = std::tolower(promo);
    }

    // Update en passant square (only for double pawn pushes)
    enPassantSquare = -1;
    if(piece == 'P' && (fromIdx >> 4) == 1 && (toIdx >> 4) == 3) {
        // white double push from rank 2 to 4
        enPassantSquare = fromIdx + 16; // square behind pawn
    } else if(piece == 'p' && (fromIdx >> 4) == 6 && (toIdx >> 4) == 4) {
        // black double push from rank 7 to 5
        enPassantSquare = fromIdx - 16;
    }

    // Update castling rights if king or rook moves
    if(piece == 'K') {
        castlingWhiteKing = false;
        castlingWhiteQueen = false;
        // Castling move detection (e1g1 or e1c1)
        if(fromIdx == 0x74 && toIdx == 0x76) { // king side
            // move rook h1 to f1
            squares[0x77] = '.';
            squares[0x75] = 'R';
        } else if(fromIdx == 0x74 && toIdx == 0x72) { // queen side
            squares[0x70] = '.';
            squares[0x73] = 'R';
        }
    } else if(piece == 'k') {
        castlingBlackKing = false;
        castlingBlackQueen = false;
        if(fromIdx == 0x04 && toIdx == 0x06) { // black king side
            squares[0x07] = '.';
            squares[0x05] = 'r';
        } else if(fromIdx == 0x04 && toIdx == 0x02) { // black queen side
            squares[0x00] = '.';
            squares[0x03] = 'r';
        }
    } else if(piece == 'R') {
        // white rook moved, remove appropriate castling rights
        if(fromIdx == 0x77) castlingWhiteKing = false; // h1
        if(fromIdx == 0x70) castlingWhiteQueen = false; // a1
    } else if(piece == 'r') {
        if(fromIdx == 0x07) castlingBlackKing = false; // h8
        if(fromIdx == 0x00) castlingBlackQueen = false; // a8
    }

    // Switch side to move
    whiteToMove = !whiteToMove;
    // Increment halfmove and fullmove counters
    if(piece == 'P' || piece == 'p' || squares[toIdx] != '.') {
        halfmoveClock = 0;
    } else {
        halfmoveClock++;
    }
    if(!whiteToMove) fullmoveNumber++;
    return true;
}

bool Board::isSquareAttacked(int sq, bool attackerWhite) const {
    // Simple attack detection: check all piece types.
    // For performance we skip some edge cases (like en passant capture), sufficient for move legality.
    // Pawns
    int dir = attackerWhite ? -16 : 16;
    int leftCapture = sq + dir - 1;
    int rightCapture = sq + dir + 1;
    if((leftCapture & 0x88) == 0) {
        char p = squares[leftCapture];
        if(attackerWhite && p == 'P') return true;
        if(!attackerWhite && p == 'p') return true;
    }
    if((rightCapture & 0x88) == 0) {
        char p = squares[rightCapture];
        if(attackerWhite && p == 'P') return true;
        if(!attackerWhite && p == 'p') return true;
    }
    // Knights
    for(int8_t off : knightOffsets) {
        int target = sq + off;
        if((target & 0x88) != 0) continue;
        char p = squares[target];
        if(attackerWhite && p == 'N') return true;
        if(!attackerWhite && p == 'n') return true;
    }
    // Bishops / Queens (diagonals)
    for(int8_t off : bishopOffsets) {
        int target = sq + off;
        while((target & 0x88) == 0) {
            char p = squares[target];
            if(p != '.' && p != '\0') {
                if(attackerWhite && (p == 'B' || p == 'Q')) return true;
                if(!attackerWhite && (p == 'b' || p == 'q')) return true;
                break; // blocked
            }
            target += off;
        }
    }
    // Rooks / Queens (orthogonal)
    for(int8_t off : rookOffsets) {
        int target = sq + off;
        while((target & 0x88) == 0) {
            char p = squares[target];
            if(p != '.' && p != '\0') {
                if(attackerWhite && (p == 'R' || p == 'Q')) return true;
                if(!attackerWhite && (p == 'r' || p == 'q')) return true;
                break;
            }
            target += off;
        }
    }
    // King
    for(int8_t off : kingOffsets) {
        int target = sq + off;
        if((target & 0x88) != 0) continue;
        char p = squares[target];
        if(attackerWhite && p == 'K') return true;
        if(!attackerWhite && p == 'k') return true;
    }
    return false;
}

bool Board::isInCheck(bool white) const {
    // Find king square
    char kingChar = white ? 'K' : 'k';
    int kingSq = -1;
    for(int idx = 0; idx < 128; ++idx) {
        if((idx & 0x88) != 0) continue;
        if(squares[idx] == kingChar) { kingSq = idx; break; }
    }
    if(kingSq == -1) return true; // no king -> illegal but treat as in check
    return isSquareAttacked(kingSq, !white);
}

std::vector<std::string> Board::generateLegalMoves() const {
    std::vector<std::string> moves;
    bool sideWhite = whiteToMove;
    // For each piece of side to move, generate moves.
    for(int idx = 0; idx < 128; ++idx) {
        if((idx & 0x88) != 0) continue;
        char p = squares[idx];
        if(p == '.' || p == '\0') continue;
        if(sideWhite != std::isupper(p)) continue; // skip opponent pieces
        // Determine piece type
        char type = std::tolower(p);
        if(type == 'p') {
            int forward = sideWhite ? -16 : 16;
            int startRank = sideWhite ? 1 : 6;
            int promotionRank = sideWhite ? 7 : 0;
            int oneStep = idx + forward;
            // Single forward
            if((oneStep & 0x88) == 0 && squares[oneStep] == '.') {
                // Promotion?
                if((oneStep >> 4) == promotionRank) {
                    for(char promo : {'q','r','b','n'}) {
                        std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(oneStep) + promo;
                        // apply move and test legality
                        Board tmp = *this;
                        tmp.applyUciMove(mv);
                        if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
                    }
                } else {
                    std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(oneStep);
                    Board tmp = *this;
                    tmp.applyUciMove(mv);
                    if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
                    // Double forward from start rank
                    if((idx >> 4) == startRank) {
                        int twoStep = oneStep + forward;
                        if((twoStep & 0x88) == 0 && squares[twoStep] == '.') {
                            std::string mv2 = indexToAlgebraic(idx) + indexToAlgebraic(twoStep);
                            Board tmp2 = *this;
                            tmp2.applyUciMove(mv2);
                            if(!tmp2.isInCheck(sideWhite)) moves.push_back(mv2);
                        }
                    }
                }
            }
            // Captures
            for(int8_t capOff : {int8_t(forward - 1), int8_t(forward + 1)}) {
                int capIdx = idx + capOff;
                if((capIdx & 0x88) != 0) continue;
                char target = squares[capIdx];
                bool occupied = target != '.' && target != '\0';
                bool enemy = occupied && (std::isupper(target) != sideWhite);
                // Include en passant capture
                bool ep = (capIdx == enPassantSquare);
                if(enemy || ep) {
                    if((capIdx >> 4) == promotionRank) {
                        for(char promo : {'q','r','b','n'}) {
                            std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(capIdx) + promo;
                            Board tmp = *this;
                            // handle en passant capture by removing captured pawn
                            if(ep) {
                                int pawnSq = capIdx + (sideWhite ? 16 : -16);
                                tmp.squares[pawnSq] = '.';
                            }
                            tmp.applyUciMove(mv);
                            if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
                        }
                    } else {
                        std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(capIdx);
                        Board tmp = *this;
                        if(ep) {
                            int pawnSq = capIdx + (sideWhite ? 16 : -16);
                            tmp.squares[pawnSq] = '.';
                        }
                        tmp.applyUciMove(mv);
                        if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
                    }
                }
            }
        } else if(type == 'n') {
            for(int8_t off : knightOffsets) {
                int to = idx + off;
                if((to & 0x88) != 0) continue;
                char target = squares[to];
                if(target != '.' && target != '\0' && std::isupper(target) == sideWhite) continue;
                std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(to);
                Board tmp = *this;
                tmp.applyUciMove(mv);
                if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
            }
        } else if(type == 'b' || type == 'q') {
            const int8_t* dirs = (type == 'b') ? bishopOffsets : bishopOffsets; // queen uses both loops later
            for(int8_t off : bishopOffsets) {
                int to = idx + off;
                while((to & 0x88) == 0) {
                    char target = squares[to];
                    if(target != '.' && target != '\0') {
                        if(std::isupper(target) != sideWhite) {
                            // capture
                            std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(to);
                            Board tmp = *this;
                            tmp.applyUciMove(mv);
                            if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
                        }
                        break; // blocked
                    } else {
                        std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(to);
                        Board tmp = *this;
                        tmp.applyUciMove(mv);
                        if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
                    }
                    to += off;
                }
            }
        }
        if(type == 'r' || type == 'q') {
            for(int8_t off : rookOffsets) {
                int to = idx + off;
                while((to & 0x88) == 0) {
                    char target = squares[to];
                    if(target != '.' && target != '\0') {
                        if(std::isupper(target) != sideWhite) {
                            std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(to);
                            Board tmp = *this;
                            tmp.applyUciMove(mv);
                            if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
                        }
                        break;
                    } else {
                        std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(to);
                        Board tmp = *this;
                        tmp.applyUciMove(mv);
                        if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
                    }
                    to += off;
                }
            }
        }
        if(type == 'k') {
            // King normal moves
            for(int8_t off : kingOffsets) {
                int to = idx + off;
                if((to & 0x88) != 0) continue;
                char target = squares[to];
                if(target != '.' && target != '\0' && std::isupper(target) == sideWhite) continue;
                std::string mv = indexToAlgebraic(idx) + indexToAlgebraic(to);
                Board tmp = *this;
                tmp.applyUciMove(mv);
                if(!tmp.isInCheck(sideWhite)) moves.push_back(mv);
            }
            // Castling
            if(sideWhite) {
                if(castlingWhiteKing) {
                    // squares f1 (0x75) and g1 (0x76) must be empty and not attacked
                    if(squares[0x75] == '.' && squares[0x76] == '.' &&
                       !isSquareAttacked(0x74, false) && !isSquareAttacked(0x75, false) && !isSquareAttacked(0x76, false)) {
                        std::string mv = "e1g1";
                        Board tmp = *this;
                        tmp.applyUciMove(mv);
                        if(!tmp.isInCheck(true)) moves.push_back(mv);
                    }
                }
                if(castlingWhiteQueen) {
                    // squares d1 (0x73), c1 (0x72), b1 (0x71) must be empty and squares e1,d1,c1 not attacked
                    if(squares[0x73] == '.' && squares[0x72] == '.' && squares[0x71] == '.' &&
                       !isSquareAttacked(0x74, false) && !isSquareAttacked(0x73, false) && !isSquareAttacked(0x72, false)) {
                        std::string mv = "e1c1";
                        Board tmp = *this;
                        tmp.applyUciMove(mv);
                        if(!tmp.isInCheck(true)) moves.push_back(mv);
                    }
                }
            } else {
                if(castlingBlackKing) {
                    if(squares[0x65] == '.' && squares[0x66] == '.' &&
                       !isSquareAttacked(0x64, true) && !isSquareAttacked(0x65, true) && !isSquareAttacked(0x66, true)) {
                        std::string mv = "e8g8";
                        Board tmp = *this;
                        tmp.applyUciMove(mv);
                        if(!tmp.isInCheck(false)) moves.push_back(mv);
                    }
                }
                if(castlingBlackQueen) {
                    if(squares[0x63] == '.' && squares[0x62] == '.' && squares[0x61] == '.' &&
                       !isSquareAttacked(0x64, true) && !isSquareAttacked(0x63, true) && !isSquareAttacked(0x62, true)) {
                        std::string mv = "e8c8";
                        Board tmp = *this;
                        tmp.applyUciMove(mv);
                        if(!tmp.isInCheck(false)) moves.push_back(mv);
                    }
                }
            }
        }
    }
    return moves;
}
