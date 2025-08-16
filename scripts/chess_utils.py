from chess import Board, LegalMoveGenerator, Move
from collections.abc import Iterable
import itertools
from math import ceil
import os
from pathlib import Path
import random
from typing import Any, Protocol, Iterator

NUM_CPUS = os.cpu_count() or 8 # default to 8 if os.cpu_count() is None

def chunk_board(board: Board, chunks: int=NUM_CPUS, **kwargs) -> Iterable[Iterable[str]]:
    """
    Split board into `chunks` chunks of fen positions reachable in one (legal) move,
    each with roughly the same number of moves.
    """
    moves = process_moves(board, sort_moves=kwargs.get("sort_moves", False),
                          filter_queen_promos=kwargs.get("filter_queen_promos", True),
                          filter_captures=kwargs.get("filter_captures", False))
    # randomly permute moves
    random.shuffle(moves) 
    # apply moves to board
    child_fens = []
    for move in moves:
        board.push(move)
        child_fens.append(board.fen())
        board.pop()
    # batch child boards
    batch_size = ceil(len(moves) / chunks)
    return itertools.batched(child_fens, batch_size)

def _filter_queen_promotions_only(moves: list[Move]) -> list[Move]:
    """
    Filter out all promotions to pieces other than queens for
    consistency with my chess engine that only considers queen
    promotions to reduce the potential search space as (queen
    promotions are optimal in over 95% of cases)
    """
    return [move for move in moves if move.uci()[-1] not in 'rbn']


def _sort_moves(moves: list[Move]) -> list[Move]:
    """
    Sort moves lexicographically by their uci (universal 
    chess interface, e.g. a1b2, d2d4) notation.
    """
    return sorted(moves, key=lambda move: move.uci())


def process_moves(board: Board, filter_queen_promos: bool=True,
                  sort_moves: bool=True, filter_captures: bool=False) -> list[Move]:
    """
    Process legal moves in board by optionally filtering out all non-queen promotions,
    sorting moves using uci as key, and/or filtering all non-captures out
    """
    move_list = list(
        board.generate_legal_captures() if filter_captures
        else board.generate_legal_moves())
    if filter_queen_promos:
        move_list = _filter_queen_promotions_only(move_list)
    if sort_moves:
        move_list = _sort_moves(move_list)
    return move_list


class GameTreeCallback(Protocol):
    """Protocol for game tree traversal callbacks with flexible argument handling."""
    def __call__(self, **kwargs: Any) -> None: ...

def game_tree_traverse(
        start_fen: str,
        depth: int,
        callback: GameTreeCallback,
        filter_queen_promos: bool=True
) -> None:
    """
    Process moves for game tree from starting position up to a 
    specified depth, executing the callback for each move applied. 
    Sort moves using uci as the key in order for deterministic
    traversal and readability.
    
    The callback can accept any combination of arguments.
    """
    def traverse_depth(board: Board, depth: int) -> int:
        if not depth:
            return 1
        node_count = 0
        for move in process_moves(board, filter_queen_promos=filter_queen_promos,
                                  sort_moves=True, filter_captures=False):
            old_fen = board.fen(en_passant="fen")
            board.push(move) # apply move
            callback(old_fen=old_fen, new_fen=board.fen(en_passant="fen"), move=move, depth=depth)
            node_count += traverse_depth(board, depth-1)
            board.pop() # undo move
        return node_count
    print("Node count:", traverse_depth(Board(start_fen), depth))


def ensure_directory_exists(dir: str) -> Path:
    """
    Create directory `dir` (in script dir, i.e. dir containing this script)
    if it doesnt exist and return the path
    """
    dir_path = Path(__file__).parent / dir
    Path.mkdir(dir_path, parents=False, exist_ok=True)
    return dir_path


def flag_to_arg(flag: str, args: tuple[str, ...]) -> str | AssertionError:
    """
    Convert short-format/long-format flags (-{arg[0]}, --{arg}) to corresponding {arg}.
    """
    if len(flag) == 2: # short format
        try:
            return next((arg for arg in args if flag[1] == arg[0]))
        except StopIteration:
            raise AssertionError(f"Invalid flag: {flag}!")
    return flag[2:] # long format, just shave off double-dash at beginning


def int_if(input: str|int, pred: bool) -> str|int:
    """
    Useful helper to conditionally parse an input string to an int if predicate is satisfied.
    """
    return int(input) if pred else input


def parse_chess_args(
    arg_list: list[str],
    valid_args: tuple[str, ...],
    required_args: tuple[str, ...]=tuple(),
    optional_defaults: dict[str, str]={}
) -> dict[str, str]:
    """
    Parse `arg_list` in form of (flag_1, arg_1 value, flag_2, arg_2 value, ...).
    Each flag_i is "-[arg_i[0]]" (short format) or "--[arg_i]" (long format).
    In the case of args having the same starting letter, long format MUST be used.
    It sets non-required args to default that must be specified (in the case that that they 
    are omitted from `arg_list`) in `optional_defaults`.
    Outputs a dict mapping args in `valid_args` to their respective values.
    """
    # check preconditions
    specified_args = {flag_to_arg(flag, valid_args) for flag in arg_list[::2]}
    missing_required_flags = set(required_args).difference(specified_args)
    unspecified_args = set(valid_args).difference(specified_args)
    missing_defaults = unspecified_args.difference(optional_defaults)

    assert (len(arg_list) % 2 == 0), f"length of `arg_list` ({arg_list}) must be even!"
    assert (len(specified_args) == len(arg_list) // 2), "cannot have duplicate flags!"
    assert (not missing_required_flags), f"missing required args: {missing_required_flags}!"
    assert (not missing_defaults), f"unspecified optional args are missing defaults: {missing_defaults}!"

    # output key-value mapping for args
    return {
        arg: (next((arg_list[i+1] for i in range(0, len(arg_list), 2)
                    if flag_to_arg(arg_list[i], valid_args) == arg),
                    optional_defaults.get(arg, "")))
        for arg in valid_args
    }


class FileManager:
    def __init__(self, out_file: str):
        self.file_path = ensure_directory_exists("output") / out_file

    def __enter__(self):
        """Opens the file stream when entering 'with' block"""
        self.__ostream = open(self.file_path, "w")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Closes the file stream when exiting 'with' block"""
        self.__ostream.close()
    
    def write(self, message: str):
        """Write to the output file stream"""
        self.__ostream.write(message)
    