from chess import Board, STARTING_FEN
from chess_utils import chunk_board, NUM_CPUS, parse_chess_args, process_moves
from collections import defaultdict
import itertools
import multiprocessing
import os
from pathlib import Path
import sys
from tqdm import tqdm
from typing import Any, Callable, Generator, Iterable, TypedDict


class RequiredPerftArgs(TypedDict):
    depth: int  # Required
    start_fen: str  # Required
    is_child_task: bool  # Required
    parent_task: tuple[int, str]  # Required


class PerftArgs(RequiredPerftArgs, total=False):
    captures_only: bool  # Optional


class PerftResult(TypedDict):
    depth: int
    start_fen: str
    node_count: int


class ChildPerftResult(PerftResult):
    is_child_task: bool
    parent_task: tuple[int, str]


def count_nodes_at_depth(
    board: Board,
    depth: int,
    captures_only: bool = False,
) -> int:
    moves = process_moves(
        board,
        sort_moves=False,
        filter_captures=captures_only and (depth == 1),
    )
    if depth == 1:
        return len(moves)
    node_count = 0
    for move in moves:
        board.push(move)
        node_count += count_nodes_at_depth(board, depth - 1)
        board.pop()
    return node_count


def aggregate_child_perft_results(
    results: Iterable[ChildPerftResult], tasks: Iterable[tuple[int, str]]
) -> list[PerftResult]:
    """Combine perft results with same parent task"""
    results_map = defaultdict(list)
    for result in results:
        results_map[result["parent_task"]].append(result["node_count"])
    return [
        {"depth": task[0], "start_fen": task[1], "node_count": sum(results)}
        for task, results in results_map.items()
    ]


def perft_child(
    depth: int,
    start_fen: str,
    parent_task: tuple[int, str],
    captures_only: bool = False,
    **kwargs: Any,
) -> ChildPerftResult:
    return {
        "depth": depth,
        "start_fen": start_fen,
        "node_count": count_nodes_at_depth(
            Board(start_fen),
            depth,
            captures_only=captures_only,
        ),
        "is_child_task": True,
        "parent_task": parent_task,
    }


def perft(
    depth: int,
    start_fen: str,
    captures_only: bool,
    **kwargs: Any,
) -> PerftResult:
    return {
        "depth": depth,
        "start_fen": start_fen,
        "node_count": count_nodes_at_depth(
            Board(start_fen),
            depth,
            captures_only=captures_only,
        ),
    }


def chunk_task(kwargs: PerftArgs, cutoff_depth: int = 3) -> Iterable[PerftArgs]:
    if kwargs["depth"] <= cutoff_depth:
        return [kwargs]
    return itertools.chain.from_iterable(
        itertools.chain.from_iterable(
            chunk_task(
                {
                    "start_fen": fen,
                    "depth": kwargs["depth"] - 1,
                    "is_child_task": True,
                    "parent_task": kwargs["parent_task"],
                }
            )
            for fen in boards_chunk
        )
        for boards_chunk in chunk_board(Board(kwargs["start_fen"]))
    )


def filter_duplicate_tasks(kwargs_list: list[PerftArgs]) -> list[PerftArgs]:
    existing_tasks: set[tuple[int, str]] = set()
    unique_kwargs = []
    for kwargs in kwargs_list:
        if (kwargs["depth"], kwargs["start_fen"]) in existing_tasks:
            continue
        unique_kwargs.append({**kwargs})
        existing_tasks.add((kwargs["depth"], kwargs["start_fen"]))
    return unique_kwargs


def perft_child_handler(kwargs):
    """Handler function for perft_child that can be pickled for multiprocessing"""
    return perft_child(**kwargs)


def perft_parallel(
    kwargs_list: list[PerftArgs], display_progress: bool = False
) -> list[PerftResult]:
    """Execute multiple calls to perft in parallel"""
    unique_kwargs = filter_duplicate_tasks(kwargs_list)
    compiled_child_tasks = itertools.chain.from_iterable(
        chunk_task(kwargs) for kwargs in unique_kwargs
    )

    with multiprocessing.Pool(processes=NUM_CPUS) as pool:
        if display_progress:
            child_task_list = list(compiled_child_tasks)
            results = tqdm(
                pool.imap_unordered(perft_child_handler, child_task_list),
                total=len(child_task_list),
                desc="Processing perft tasks",
            )
        else:
            results = pool.imap_unordered(perft_child_handler, compiled_child_tasks)
        return aggregate_child_perft_results(
            results,
            ((kwargs["depth"], kwargs["start_fen"]) for kwargs in unique_kwargs),
        )


def parse_args_from_stdin(captures_only: bool) -> Generator[PerftArgs, None, None]:
    for line in sys.stdin:
        if not line.strip():  # skip empty lines
            continue
        args = line.strip().split(",")
        yield parse_cmdline_args(args, captures_only)


def parse_cmdline_args(
    args: list[str],
    captures_only: bool,
    boolean_flags_to_strip: tuple[str, ...] = (
        "captures-only",
        "priority",
        "write",
        "report-progress",
    ),
) -> PerftArgs:
    """Parse list of cmdline args for a single call to perft"""
    truncated_args = strip_boolean_flags(args, boolean_flags_to_strip)
    kwargs = parse_chess_args(
        truncated_args,
        ("depth", "start_fen"),
        required_args=("depth",),
        optional_defaults={"start_fen": STARTING_FEN},
    )
    return {
        "start_fen": kwargs["start_fen"],
        "depth": int(kwargs["depth"]),
        "captures_only": captures_only,
        "is_child_task": False,
        "parent_task": (int(kwargs["depth"]), kwargs["start_fen"]),
    }


def status_logger_file() -> Path:
    """Default logger file"""
    log_dir = Path(__file__).resolve().parent / "logs"
    Path.mkdir(log_dir, parents=False, exist_ok=True)
    return log_dir / "perft_log.txt"


def results_logger_file() -> Path:
    """Default results file"""
    out_dir = Path(__file__).resolve().parent / "output"
    Path.mkdir(out_dir, parents=False, exist_ok=True)
    return out_dir / "perft.out"


def log_results(results: PerftResult, log_func: Callable[[str], None]):
    log_func(
        f"Depth: {results['depth']}, FEN: \"{results['start_fen']}\", Nodes: {results['node_count']}\n"
    )


def setup_results_logging(args: list[str]) -> tuple[Any, bool, Any]:
    """Setup logging based on args"""
    write_to_file = "-w" in args or "--write" in args
    if write_to_file:
        print(f"Logging results to {results_logger_file()}!")
        logger = open(results_logger_file(), "w")
        return logger.write, True, logger
    print("Logging results to console!\n")
    return lambda res: print(res, end=""), False, None


def strip_boolean_flags(args: list[str], flags: tuple[str, ...]) -> list[str]:
    return [
        arg
        for arg in args
        if arg not in [opt for flag in flags for opt in (f"-{flag[0]}", f"--{flag}")]
    ]


def exists_non_boolean_arg(args: list[str]) -> bool:
    return next((arg for arg in args if arg[0] != "-"), None) is not None


def main(
    always_parallel: bool = True,
    high_priority: bool = False,
    captures_only: bool = False,
):
    args = sys.argv[1:]
    log_func, write_to_file, logger = setup_results_logging(args)
    display_progress = "-r" in args or "--report-progress" in args

    sys.stderr.write(f"Executing perft {"with" if high_priority else "without"} priority...\n")
    sys.stderr.write(f"Logging results to {f"file ({results_logger_file()})" if write_to_file else "console"}\n")
    if display_progress:
        sys.stderr.write("Progress display enabled\n")

    if exists_non_boolean_arg(args):
        parsed_args = parse_cmdline_args(args, captures_only)
        if always_parallel:
            log_results(perft_parallel([parsed_args], display_progress)[0], log_func)
        else:
            log_results(perft(**parsed_args), log_func)
    else:
        sys.stderr.write("No arguments provided... reading from stdin\n")
        for result in perft_parallel(
            list(parse_args_from_stdin(captures_only=captures_only)),
            display_progress,
        ):
            log_results(result, log_func)

    try:
        logger.close()
        sys.stderr.close()
    except (AttributeError, OSError):
        pass


def try_to_execute_with_priority() -> bool:
    attempt_execute_w_priority = ("--priority" in sys.argv) or ("-p" in sys.argv)
    successful = False
    if attempt_execute_w_priority:
        try:
            os.nice(-20)
            successful = True
            sys.stderr.write(
                "Successfully set nice value to -20, which maximizes cpu scheduling priority "
                "for this process and its child processes.\n"
            )
        except:
            sys.stderr.write(
                "Was not able to set nice value to -20, could be permission issues or platform "
                "incompatibility.\nContinuing with standard process scheduling priority\n"
            )
    return successful


def redirect_stderr_for_status_logging() -> None:
    sys.stderr = open(status_logger_file(), "w")


if __name__ == "__main__":
    redirect_stderr_for_status_logging()
    main(
        always_parallel=True,
        high_priority=try_to_execute_with_priority(),  # ITS MY CPU TIME!!! •`_´•
        captures_only=("-c" in sys.argv) or ("--captures-only" in sys.argv),
    )
