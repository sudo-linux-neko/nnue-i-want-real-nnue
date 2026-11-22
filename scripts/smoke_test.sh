#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Vaibhav Agarwal

set -e

# Path to the built engine binary (relative to script location)
BIN="$(dirname "$0")/../build/neko-meow"

if [[ ! -x "$BIN" ]]; then
    echo "Engine binary not found at $BIN"
    exit 1
fi

# Run the engine with a set of UCI commands.
output=$("$BIN" <<EOF
uci
isready
ucinewgame
position startpos moves e2e4 e7e5
go depth 1
position fen rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3
go depth 1
quit
EOF
)

# Verify essential UCI responses.
if echo "$output" | grep -q "id name neko-meow" && \
   echo "$output" | grep -q "readyok" && \
   echo "$output" | grep -q "bestmove"; then
    echo "Smoke test passed"
    exit 0
else
    echo "Smoke test failed"
    echo "Engine output:"
    echo "$output"
    exit 1
fi
