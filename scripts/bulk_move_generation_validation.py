from multiprocessing import Pool, Manager
import os
from pathlib import Path
import subprocess
import sys
from time import perf_counter
from threading import Lock
from tqdm import tqdm
from typing import Generator

import chess

# Get abs path to directory containing script
SCRIPT_DIR = Path(__file__).resolve().parent

# Size of fen test batch (configurable)
DEFAULT_FEN_TEST_BATCH_SIZE = 1_000_000

# define paths relative to directory
ENGINE_BUILD_PATH = SCRIPT_DIR.parent / "build"
ENGINE_EXECUTABLE = ENGINE_BUILD_PATH / "engine"

# modified for new fens file (consider switching to passing as cl arg)
FEN_DATA_PATH = SCRIPT_DIR/ "output" / "fens.txt"

LOG_DIR = SCRIPT_DIR / "logs"

LOG_FILE = SCRIPT_DIR / "logs" / "mvgen_errors.log"
OUT_FILE = SCRIPT_DIR / "logs" / "mvgen_out.log"


def print_to_err_log(message: str) -> None:
    with open(LOG_FILE, "a") as f:
        f.write(message)


def print_to_out_log(message: str) -> None:
    with open(OUT_FILE, "a") as f:
        f.write(message)


def print_board_to_err_log(fen: str) -> None:
    pieces, turn, castling, enpassant, *_ = fen.split(" ")
    print_to_err_log(
        f"turn:      {turn}\n"
        f"castling:  {castling}\n"
        f"enpassant: {enpassant}\n"
    )
    for row in pieces.split("/"):
        for c in row:
            print_to_err_log(c if c.isalpha else int(c) * ".")
        print_to_err_log("\n")
    print_to_err_log(
        f"#####################################\n"
    )


def get_num_fens() -> int:
    # Try ripgrep first (much faster for large files)
    try:
        result = subprocess.run(
            ["rg", "--count", r"\w", FEN_DATA_PATH],
            capture_output=True,
            text=True,
            timeout=30,
        )
        if result.returncode == 0:
            return int(result.stdout.strip())
    except (subprocess.TimeoutExpired, FileNotFoundError, ValueError):
        pass

    # Fallback to Python (slower but more portable)
    try:
        with open(FEN_DATA_PATH, "r") as f:
            return sum(1 for line in f if line.strip())
    except FileNotFoundError:
        print(f"Error: FEN data file not found at {FEN_DATA_PATH}")
        sys.exit(1)


def fens(
    lock: Lock, fen_test_batch_size: int = DEFAULT_FEN_TEST_BATCH_SIZE
) -> Generator[tuple[str, Lock], None, None]:
    try:
        with open(FEN_DATA_PATH, "r") as f:
            consumed_fens = 0
            for line in f:
                if data := line.strip():  # Skip empty lines
                    yield data, lock
                    consumed_fens += 1
                if consumed_fens >= fen_test_batch_size:
                    return  # close generator
    except FileNotFoundError:
        print(f"Error: FEN data file not found at {FEN_DATA_PATH}")
        sys.exit(1)


def build_engine():
    try:
        subprocess.run(
            ["cmake", "--build", ENGINE_BUILD_PATH],
            capture_output=True,
            text=True,
            check=True,
        )
    except subprocess.CalledProcessError as e:
        print(f"Error while building engine: {e}")
        print(f"Build stdout: {e.stdout}")
        print(f"Build stderr: {e.stderr}")


def get_engine_output(fen: str) -> str:
    try:
        result = subprocess.run(
            [ENGINE_EXECUTABLE, "--legal-moves", fen],
            capture_output=True,
            text=True,
            timeout=10,  # Add timeout to prevent hanging
        )
        if result.returncode != 0:
            print(f"Warning: Engine returned non-zero exit code for FEN: {fen}")
            return ""
        return result.stdout.strip()
    except subprocess.TimeoutExpired:
        print(f"Warning: Engine timed out for FEN: {fen}")
        return ""
    except FileNotFoundError:
        print(f"Error: Engine executable not found at {ENGINE_EXECUTABLE}")
        sys.exit(1)


def parse_engine_output(engine_output: str) -> set[str]:
    return set(
        move.strip()
        for move in engine_output.strip("MoveList()").split(",")
        if move.strip()
    )


def actual_generated_moves(fen: str) -> set[str]:
    return parse_engine_output(get_engine_output(fen))


def expected_generated_moves(fen: str):
    try:
        board = chess.Board(fen)
        return {move.uci() for move in board.legal_moves if move.uci()[-1] not in "rnb"}
    except ValueError as e:
        print_to_err_log(f"\nWarning: Invalid FEN, got error {e}\n")
        return set()


def compare_generated_moves(args: tuple[str, Lock]) -> None:
    fen, lock = args

    actual = actual_generated_moves(fen)
    expected = expected_generated_moves(fen)

    extraneous_moves = actual.difference(expected)
    missing_moves = expected.difference(actual)

    if missing_moves or extraneous_moves:
        # lock debugging output, so that printed expressions are not garbled
        with lock:
            print_to_out_log(f"Failed for {fen}!\n")
            print_board_to_err_log(fen)
            print_to_err_log(f"Fen: {fen}")
            if extraneous_moves:
                print_to_err_log(f"Extraneous moves: {extraneous_moves}\n")
            if missing_moves:
                print_to_err_log(f"Missing moves: {missing_moves}\n")
            print_to_err_log(f"#####################################\n")


def validate_move_generation(fen_test_batch_size: int | None = None):
    with Manager() as manager:
        lock = manager.Lock()
        fen_generator = (
            fens(lock)
            if fen_test_batch_size is None
            else fens(lock, fen_test_batch_size)
        )
        with Pool() as pool:
            for _ in tqdm(
                pool.imap_unordered(compare_generated_moves, fen_generator),
                total=fen_test_batch_size,
            ):
                pass


if __name__ == "__main__":

    if os.path.exists(LOG_FILE):
        LOG_FILE.unlink()
        print(f"Deleted file: {LOG_FILE}")

    if os.path.exists(OUT_FILE):
        OUT_FILE.unlink()
        print(f"Deleted file: {OUT_FILE}")

    LOG_DIR.mkdir(parents=True, exist_ok=True)

    if not os.path.exists(ENGINE_EXECUTABLE):
        print("Executable not found.")
        print("Rebuilding engine...")
        build_engine()

    print(f"Testing up to {DEFAULT_FEN_TEST_BATCH_SIZE} FEN positions...")

    start = perf_counter()
    validate_move_generation()
    elapsed_time = perf_counter() - start
    print(f"Finished bulk validation! Took {elapsed_time:.2f}s!")
