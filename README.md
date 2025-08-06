# Chess Engine

A performant chess engine built with modern C++17/20 features.

## Technical Highlights

- **Template metaprogramming** for compile-time optimizations
- **Advanced bitwise operations** with bitboards for efficient piece manipulation
- **Memory-efficient transposition table** with sophisticated bitfield design
- **Type-safe move system** with template specializations
- **Alpha-beta search** with iterative deepening

## Build & Run

### Prerequisites
- **C++20 compatible compiler** (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake 3.14+**

### Build Instructions
```bash
git clone https://github.com/noah-yared/chess-engine.git
cd chess-engine

# Create build directory
mkdir build && cd build

# Configure build for release mode
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build the engine
make
```

## Usage

```bash
# Show help
./engine --help

# Generate legal moves from a FEN string
./engine --legal-moves "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

# Find best move with depth 6
./engine --find-best "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" --depth 6

# Apply move to a FEN string
./engine --make-move --fen "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" --move "e2e4"

# Simulate self-play from the default starting FEN for 50 moves with search depth 6, dumping positions to "out.txt"
./engine --simulate --num-moves 50 --depth 6 --output out.txt
```

## Testing

```bash
# Run move generation tests
ctest -R "MoveGeneration"

# Run check detection tests
ctest -R "CheckDetection"

# Run move apply/undo tests
ctest -R "Move(Apply|Undo)"

# Run all tests
ctest
```

## Benchmarks

```bash
# Run move generation benchmark
./bench_move_generation

# Run static evaluation benchmark
./bench_static_eval

# Run move apply/undo benchmark
./bench_move_apply

# Run search benchmark
./bench_search

# Run all benchmarks
for exe in ./bench_*; do
  "$exe"
done
```
