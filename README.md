# neko-meow

**neko-meow** is a C++20 chess engine implementing a real NNUE evaluation function (placeholder for now) and UCI protocol. This repository contains:

- A minimal **UCI engine** (`neko-meow`) that can parse standard UCI commands, set up the board, generate legal moves (including castling, en‑passant, promotion), and return a best move.
- A **0x88 board representation** with move generation and basic validation.
- An **NNUE stub** that loads a binary weight file (`nnue.bin`) at startup; the actual network inference is a future extension.
- A **smoke test** script (`scripts/smoke_test.sh`) that builds the engine, runs a short UCI session and verifies expected responses.

## Build

```bash
# Configure and build (Linux/macOS)
mkdir -p build && cd build
cmake ..
cmake --build .
```

The binary is placed at `build/neko-meow`.

## Run the Engine

```bash
./build/neko-meow
```

It will then accept UCI commands on stdin. Example session (shown by the smoke test):

```
uci
isready
ucinewgame
position startpos moves e2e4 e7e5
go depth 1
quit
```

## NNUE Weight File

The engine expects a binary NNUE weight file named `nnue.bin` in the working directory. The current stub only checks that the file can be opened and reports its size; the evaluation always returns `0`. Replace it with a proper quantized weight file and implement the forward pass for a functional NNUE.

## Smoke Test

```bash
./scripts/smoke_test.sh
```

The script builds the engine (if not already built), runs a minimal UCI handshake and checks that the engine responds with `id name`, `readyok` and a `bestmove` line.

## Next Steps

- Implement a full **NNUE forward pass** with quantized integer inference and an accumulator stack for incremental updates.
- Replace the trivial move selection with a proper **search** (iterative deepening alpha‑beta, pruning, multi‑threading, transposition table, time management).
- Add **training pipeline** (Python + PyTorch) to produce the NNUE weight file from PGN data.
- Expand the smoke test to cover en‑passant, castling and promotion positions.
- Provide a benchmark tool (`neko-meow bench`) and a GUI integration guide.

---

*Engine name*: `neko-meow`
*Author*: Vaibhav
