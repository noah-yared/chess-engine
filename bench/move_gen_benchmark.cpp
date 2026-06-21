#include <benchmark/benchmark.h>

#include "board/position.h"
#include "move/move_generator.h"

static void BM_MoveGenBaseline_StartPos(benchmark::State& state)
{
    Position pos{};
    for (auto _ : state)
    {
        MoveList moves{};
        MoveGenerator::pushLegalMoves<Color::WHITE>(pos, moves);
        benchmark::DoNotOptimize(moves);
    }
}

BENCHMARK(BM_MoveGenBaseline_StartPos);

static void BM_MoveGenBaseline_MidPos(benchmark::State& state)
{
    Position pos{"r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1"};
    for (auto _ : state)
    {
        MoveList moves{};
        MoveGenerator::pushLegalMoves<Color::WHITE>(pos, moves);
        benchmark::DoNotOptimize(moves);
    }
}

BENCHMARK(BM_MoveGenBaseline_MidPos);

BENCHMARK_MAIN();
