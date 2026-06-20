#include <benchmark/benchmark.h>

#include "search/engine.h"

static void BM_SearchBaseline_StartPos(benchmark::State& state)
{
    SearchEngine engine;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(engine.search(state.range(0)));
    }
}

BENCHMARK(BM_SearchBaseline_StartPos)->DenseRange(1, 7);

static void BM_SearchBaseline_MidPos(benchmark::State& state)
{
    SearchEngine engine("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(engine.search(state.range(0)));
    }
}

BENCHMARK(BM_SearchBaseline_MidPos)->DenseRange(1, 5);

BENCHMARK_MAIN();
