import chess
from chess_utils import parse_chess_args, game_tree_traverse, FileManager
import sys
from typing import Any

def write_fen_callback(file_manager: FileManager):
    def callback(**kwargs: Any) -> None:
        file_manager.write(f"{",".join([
            kwargs["old_fen"], kwargs["move"].uci(), kwargs["new_fen"],
        ])}\n")
    return callback


def generate_move_application_data(arg_list: list[str]):
    kwargs = parse_chess_args(
        arg_list, ("depth", "out", "start_fen"),
        required_args=("depth",),
        optional_defaults={
            "start_fen": chess.STARTING_FEN,
            "out": "move_app_data.txt",
        }
    )

    with FileManager(kwargs["out"]) as file_manager:
        game_tree_traverse(
            kwargs["start_fen"],
            int(kwargs["depth"]),
            write_fen_callback(file_manager)
        )


if __name__ == "__main__":
    generate_move_application_data(sys.argv[1:])
