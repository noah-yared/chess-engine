#!/usr/bin/env python3

import sys


def print_fen(fen: str, emptyFill: str = ".") -> None:
    piecesPart = fen[: fen.index(" ")]
    rows = piecesPart.split("/")
    for rank, row in zip("87654321", rows):
        print(rank, "|", end=" ")
        for char in row:
            print(char if char.isalpha() else " ".join(int(char) * emptyFill), end=" ")
        print()
    starting_ws = 4 * " "
    print(starting_ws, "-".join(8 * "-"), sep="")
    print(starting_ws, " ".join("abcdefgh"), sep="")


if __name__ == "__main__":
    args = sys.argv[1:]

    try:
        (fen,) = args
        print(f"Printing ascii board for {fen=}...")
        print_fen(fen)

    except ValueError:
        print("Usage: python3 [SCRIPT_PATH] [FEN_STRING]")
        exit(1)
