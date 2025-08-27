#!/usr/bin/env python3

import math
import chess
import pyperclip
import sys


def get_ucis(fen: str | None = None, promo_to_queen_only=False) -> list[str]:
    board = chess.Board() if fen is None else chess.Board(fen)
    if not promo_to_queen_only:
        return [move.uci() for move in board.legal_moves]
    return list(
        {move.uci() for move in board.legal_moves if move.uci()[-1] not in "nbr"}
    )


def stringify_ucis(fen: str | None = None, ucis_per_row: int = 10, promo_to_queen_only=False) -> str:
    ucis_str = ""
    # sort to get deterministic output
    uci_list = list(sorted(get_ucis(fen=fen, promo_to_queen_only=promo_to_queen_only)))
    for row in range(math.ceil(len(uci_list) / ucis_per_row)):
        ucis_str += "      "
        for i in range(row * ucis_per_row, min((row + 1) * ucis_per_row, len(uci_list))):
            ucis_str += f'"{uci_list[i]}"' + (", " if len(uci_list) - 1 else "")
        ucis_str += "\n"
    return ucis_str[:-1]


if __name__ == "__main__":
    # replace with fen to generate ucis for
    assert len(sys.argv) == 2, "Invalid number of arguments!\nUsage: python [script_path] [fen_string]"
    stringified_ucis = stringify_ucis(fen=sys.argv[1], promo_to_queen_only=True)
    print(stringified_ucis)
    # copy stringified ucis to clipboard
    pyperclip.copy(stringified_ucis)
    print("\nCopied to clipboard!")
