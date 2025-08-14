# Scripts

This directory contains utility and testing scripts used to support development of this engine.

---

## Core Utilities

### `chess_utils.py`
A shared utility module providing common functionality for chess-related scripts:
- **`game_tree_traverse()`**: Traverses a chess game tree to a specified depth, executing callbacks for each move
- **`parse_chess_args()`**: Parses command-line arguments in flag-value format (e.g., `--depth 5 --out file.txt`)
- **`FileManager`**: Context manager for file output operations
- **`processed_moves()`**: Sorts and optionally filters moves (e.g., queen promotions only)

---

## PGN / FEN Utilities

### `parse_pgn_db.py`
Parses a large zstd-compressed pgn database and extracts the unique FENs of all the games.

> Note: the pgn database is not included in this repo due to its size (200+ MB). Feel free to download it [here](https://lichess/database.lichess.org/standard/lichess_db_standard_rated_2015-01.pgn.zst). Place the downloaded file into the `scripts/data/` directory and keep the filename as is or update the script to use the new filename.

### `fen_generator.py`
Generates FEN positions by traversing a chess game tree to a specified depth. Useful for creating test datasets.

---

## Engine Testing

### `bulk_move_generation_validation.py`
Compares engine-generated moves against `python-chess` over many FENs to validate correctness using FENs parsed by `parse_pgn_db.py`.

### `generate_legal_moves.py`
Takes a FEN and prints all legal moves nicely formatted using the `python-chess` library, used for writing legal move generation tests for the engine.

### `move_application_generator.py`
Generates test data for validating the engine's move application functionality. Creates CSV-formatted data with before-FEN, move, and after-FEN.

### `move_application_test.sh`
Tests the engine's move application by comparing expected vs actual board states after moves. Automatically builds the engine if needed and provides error reporting.

---

## Hashing Utilities

### `find_perfect_hasher.py`
Finds a custom perfect hash function for a given set of KEYS and VALUES, used for finding a way to perfectly hash 'k', 'q', 'K', 'Q' (representing black kingside, black queenside, white kingside, white queenside castling rights) to the respective final bitboard indices of the king after castling : 57, 61, 1, 5, respectively. This allows us to avoid branching on castling rights in the move generation code.

### `check_hashers.py`
Used for verifying the correctness of the perfect hashing functions computed by `find_perfect_hasher.py`.

---

## Miscellaneous Utilities

### `print_ascii_board.py`
Renders a FEN as an ASCII board (good for CLI debugging).

### `snake_case.py`
Used to rename all files in the `include/` and `src/` directories as well as all the include directives in those files to snake_case from camelCase.

---

## Requirements

### Prerequisites
- **Python 3.7+** (required for `python-chess` library)
- **pip** (Python package installer)

### Setup
Some scripts require external libraries. Create a virtual environment and install dependencies:

```bash
# Create virtual environment
python -m venv venv

# Activate virtual environment
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt
```
