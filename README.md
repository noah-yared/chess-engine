# Chess Engine - Modern C++ Implementation

A high-performance chess engine built with modern C++17/20 features, demonstrating fundamental software engineering practices and efficient chess algorithms.

## 🚀 Recent Major Refactor

This project has evolved significantly from its initial implementation. The current version represents a complete architectural rewrite using modern C++17/20 features, resulting in:

- **Modern C++17/20** with `<bit>`, `<concepts>`, `<variant>`, constexpr, fold expressions, template specializations, etc.
- **Clean architecture** with proper separation of concerns
- **Type-safe design** throughout the codebase
- **Efficient bitboard operations** for performance

## ✨ Technical Highlights

### Modern C++ Features
- **Template metaprogramming** for type safety and compile-time optimization
- **C++20 concepts** and `requires` clauses for compile-time validation
- **Constexpr functions** for precomputed attack patterns
- **Variadic templates** for flexible attack detection
- **Template specializations** for compile-time dispatch
- **Advanced bit manipulation** with bitboards using <bit>


### Architecture
- **Clean separation of concerns** between move generation, attack detection, and state management
- **Type-safe move system** with template specializations for different move types
- **Efficient position representation** using bitboards
- **Zobrist hashing** for position identification and transposition tables
- **Comprehensive state management** with snapshot capabilities

### Performance Optimizations
- **Bitboard-centric design** for efficient piece operations
- **Precomputed attack patterns** for sliding pieces
- **Optimized move generation** pipeline
- **Memory-efficient transposition table** with packed bitfields

## 🏗️ Core Components

### Position Management (`position.h`)
- **Efficient move application and undo** with state snapshots
- **Comprehensive attack detection** with variadic templates
- **Type-safe move validation** and generation

### Bitboard Operations (`bitboards.h`, `bit_utils.h`)
- **Optimized bitboard manipulation** for chess piece operations
- **Precomputed attack patterns** for all piece types
- **Efficient filtering and clearing** operations

### Transposition Table (`transposition_table.h`)
- **Sophisticated bitfield design** with template metaprogramming
- **Memory-efficient storage** (12 bytes per entry)
- **Advanced field layout** with compile-time validation

### Move System (`move.h`, `move_list.h`)
- **Type-safe move representation** with template specializations
- **Comprehensive move types** (normal, castling, en passant, promotion)
- **Efficient move list management**

## 🔧 Current Status

### ✅ Completed
- **Core architecture** with modern C++20
- **Clean, maintainable codebase**
- **Efficient move generation** pipeline
- **Advanced attack detection** algorithms
- **Type-safe design** throughout

### 🔄 In Progress
- **Comprehensive test suite** development
- **Bug fixes** and edge case handling
- **Performance optimization** and benchmarking
- **Move ordering** improvements

### 📋 Planned
- **Quiescence search** implementation
- **Advanced evaluation** heuristics
- **Opening book** integration
- **UCI protocol** support

## 🛠️ Build & Run

### Prerequisites
- **C++20 compatible compiler** (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake 3.14+**

### Build Instructions
```bash
# Clone the repository
git clone https://github.com/noah-yared/chess-engine.git
cd chess-engine

# Configure the build
cmake -S . -B build

# Build the project
cd build
cmake --build .

# Run engine
./engine
```

## 🧪 Testing

The project includes comprehensive test suites covering (work in progress):
- **Move generation** for all piece types
- **Position state management** and validation
- **Attack detection** algorithms
- **Move application** and undo operations
- **Edge cases** and special moves

```bash
# If not already in build directory
cd ./path/to/build 
# Run all tests (work in progress)
ctest
```

## 📊 Performance

The refactored engine demonstrates significant improvements:
- **Faster move generation** through optimized bitboard operations
- **Reduced memory usage** with efficient data structures
- **Better cache locality** with modern C++ design patterns
- **Compile-time optimizations** through template metaprogramming
- **Addition of transposition table** for improved search performance
- **Compile-time dispatch** for move generation with template specializations

## 🎯 Development Philosophy

This project demonstrates:
- **Modern C++ best practices** and modern language features
- **Clean architecture** and separation of concerns
- **Performance-conscious design** with bitboard operations
- **Type safety** through template metaprogramming
- **Iterative improvement** and code quality focus


---

*This project represents a significant evolution in both chess engine development and modern C++ programming practices. The refactored codebase demonstrates fundamental software engineering principles while maintaining the core functionality of a competitive chess engine.* 
