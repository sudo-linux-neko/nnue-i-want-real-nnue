#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Vaibhav Agarwal

"""Placeholder static evaluation tool.

Runs neko-meow on a given PGN file and prints best moves for positions.
"""
import argparse
import subprocess

def main():
    parser = argparse.ArgumentParser(description="Static evaluation using neko-meow (placeholder)")
    parser.add_argument("--engine", default="../build/neko-meow", help="Path to engine binary")
    parser.add_argument("--pgn", required=True, help="PGN file with positions to evaluate")
    parser.add_argument("--depth", type=int, default=10, help="Search depth")
    args = parser.parse_args()
    print(f"[static_eval] Would evaluate positions from {args.pgn} using {args.engine} at depth {args.depth}.")

if __name__ == "__main__":
    main()
