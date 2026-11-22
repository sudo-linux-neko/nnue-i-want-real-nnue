// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vaibhav Agarwal

#pragma once

#include <string>
#include <vector>

class Board; // forward declaration

// Minimal NNUE placeholder.
// In a full implementation this would hold quantized weights, an accumulator stack, etc.
// Here we only implement load() to satisfy the requirement that the engine loads a weight file.
#include <cstdint>
#include <vector>
#include <string>
#include "board.hpp"
#include "move.hpp"

// NNUE (Efficiently Updatable Neural Network) implementation built from scratch.
// This follows the specification in the project prompt (Section 1).
// All arithmetic is performed in integer domain using the quantization scales stored in the weight file.

class NNUE {
public:
    // Architecture constants – can be changed at compile time.
    static constexpr int INPUT_FEATURES = 64 * 64 * 10; // king‑relative piece‑square features per perspective
    static constexpr int TRANSFORMER_WIDTH = 256;    // configurable width of first linear layer
    static constexpr int HIDDEN1 = 32;                // size of first hidden dense layer
    static constexpr int HIDDEN2 = 32;                // size of second hidden dense layer

    // Load a quantized NNUE weight file. Returns true on success.
    bool load(const std::string& path);

    // Initialise the accumulator for the given board (full recompute). Must be called before the first search.
    void initAccumulator(const Board& board);

    // Update the accumulator incrementally according to the move that was just made.
    // The board must already reflect the move (i.e. after makeUciMove). 'info' contains the move details.
    void updateAccumulator(const MoveInfo& info);

    // Undo the last accumulator update (pop snapshot).
    void popAccumulator();

    // Evaluate the current position using the accumulator that is on top of the stack.
    // Returns a centipawn evaluation from the side to move's perspective (positive = white advantage).
    int evaluate() const;

private:
    // Quantisation scale factors (read from file). They convert stored integer weights back to their original float values.
    float transformerScale = 1.0f;
    float denseScale = 1.0f; // same scale used for dense layers for simplicity
    float outputScale = 1.0f;

    // Weight storage (quantised). All vectors are stored in row‑major order.
    std::vector<int16_t> transformerWeight; // size INPUT_FEATURES * TRANSFORMER_WIDTH
    std::vector<int16_t> transformerBias;   // size TRANSFORMER_WIDTH
    std::vector<int8_t> denseW1;             // size (2*TRANSFORMER_WIDTH) * HIDDEN1
    std::vector<int8_t> denseB1;             // size HIDDEN1
    std::vector<int8_t> denseW2;             // size HIDDEN1 * HIDDEN2
    std::vector<int8_t> denseB2;             // size HIDDEN2
    std::vector<int8_t> denseWOut;           // size HIDDEN2
    std::vector<int16_t> denseBOut;          // size 1 (output bias)

    // Accumulator snapshots – one entry per search ply (including the root).
    // Each snapshot holds the concatenated accumulator vectors for White and Black perspectives (size 2*TRANSFORMER_WIDTH).
    std::vector<std::vector<int32_t>> accStack;

    // Helper functions.
    static int squareToIndex0x88(int sq0x88) {
        // Convert a 0x88 index to 0‑63 board index (only for valid squares).
        int file = sq0x88 & 0xF;
        int rank = sq0x88 >> 4;
        return rank * 8 + file;
    }
    static int kingSquare(const Board& board, bool white);
    static int pieceTypeFromChar(char c); // 0=pawn,1=knight,2=bishop,3=rook,4=queen
    static int featureIndex(int kingSq, int pieceSq, int pieceType, bool pieceWhite);
    void recomputeAccumulatorForSide(const Board& board, bool whiteSide, std::vector<int32_t>& outVec) const;
    // Clip ReLU for quantised values (clamp to [0, QUANT_MAX]) – QUANT_MAX corresponds to 1.0 in float.
    static constexpr int QUANT_CLIP = 127; // example; can be adjusted with scale.
    static int32_t reluClamp(int32_t v) { return v < 0 ? 0 : (v > QUANT_CLIP ? QUANT_CLIP : v); }
};

