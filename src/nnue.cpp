// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vaibhav Agarwal

#include "nnue.hpp"
#include "board.hpp"
#include <fstream>
#include <iostream>

bool NNUE::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "[NNUE] Failed to open weight file: " << path << std::endl;
        return false;
    }
    // Read entire file into memory (placeholder implementation).
    in.seekg(0, std::ios::end);
    std::size_t size = static_cast<std::size_t>(in.tellg());
    in.seekg(0);
    data.resize(size);
    if (size > 0) {
        in.read(data.data(), size);
        if (!in) {
            std::cerr << "[NNUE] Error reading weight file." << std::endl;
            return false;
        }
    }
    std::cerr << "[NNUE] Loaded weight file '" << path << "' (" << size << " bytes)." << std::endl;
    return true;
}

int NNUE::evaluate(const Board& /*board*/) const {
    // Placeholder: return zero evaluation.
    return 0;
}
