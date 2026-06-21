#include <benchmark/benchmark.h>

#include "board/position.h"
#include "move/move_generator.h"

static void BM_CopyPosition(benchmark::State& state)
{
    Position pos;
    MoveList ml{};
    MoveGenerator::pushLegalMoves<Color::WHITE>(pos, ml);
    long long N = state.range(0);
    for (auto _ : state)
    {
        for (long long i = 0; i < N; ++i)
        {
            auto tmp = pos;
            auto move = ml[i % ml.size()];
            std::visit([&](auto&& arg) { tmp.applyMove(arg); }, move);
            int eval = tmp.evaluation();
            benchmark::DoNotOptimize(tmp);
            benchmark::DoNotOptimize(eval);
            benchmark::ClobberMemory();
        }
    }
}

BENCHMARK(BM_CopyPosition)->Range(8, 256);

static void BM_ModifyPositionInPlace(benchmark::State& state)
{
    Position pos;
    MoveList ml{};
    MoveGenerator::pushLegalMoves<Color::WHITE>(pos, ml);
    long long N = state.range(0);
    for (auto _ : state)
    {
        for (long long i = 0; i < N; ++i)
        {
            auto move = ml[i % ml.size()];
            std::visit([&](auto&& arg) { pos.applyMove(arg); }, move);
            int eval = pos.evaluation();
            std::visit([&](auto&& arg) { pos.undoMove(arg, pos.getStateSnapshot()); }, move);
            benchmark::DoNotOptimize(eval);
            benchmark::ClobberMemory();
        }
    }
}

BENCHMARK(BM_ModifyPositionInPlace)->Range(8, 256);

BENCHMARK_MAIN();
