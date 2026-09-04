#include <benchmark/benchmark.h>

#include "search/engine_controller.h"

// First search with parallelism > 1 constructs the thread pool. Run a cheap
// depth-1 search outside the timed loop so spawning workers is not attributed
// to search.
static void warmupThreadPool(EngineController& engine, int parallelism)
{
    if (parallelism > 1)
        (void)engine.search(SearchConfig::fixedDepth(1).setParallelism(parallelism));
}

static void BM_SearchBaseline_StartPos(benchmark::State& state)
{
    const int parallelism = static_cast<int>(state.range(0));
    const int depth = static_cast<int>(state.range(1));
    EngineController engine;
    auto config = SearchConfig::fixedDepth(depth).setParallelism(parallelism);
    warmupThreadPool(engine, parallelism);

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(engine.search(config));
    }
}

BENCHMARK(BM_SearchBaseline_StartPos)
    ->ArgsProduct({{1, 2, 4, 8}, {1, 2, 3, 4, 5, 6, 7}})
    ->ArgNames({"threads", "depth"});

static void BM_SearchBaseline_MidPos(benchmark::State& state)
{
    const int parallelism = static_cast<int>(state.range(0));
    const int depth = static_cast<int>(state.range(1));
    EngineController engine("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
    auto config = SearchConfig::fixedDepth(depth).setParallelism(parallelism);
    warmupThreadPool(engine, parallelism);

    for (auto _ : state)
    {
        benchmark::DoNotOptimize(engine.search(config));
    }
}

BENCHMARK(BM_SearchBaseline_MidPos)
    ->ArgsProduct({{1, 2, 4, 8}, {1, 2, 3, 4, 5}})
    ->ArgNames({"threads", "depth"});

BENCHMARK_MAIN();
