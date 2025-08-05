#include <benchmark/benchmark.h>

#include "position.h"

static void BM_Evaluation_v1_StartPos(benchmark::State& state) {
  Position pos;
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      int eval = pos.evaluation_v1();
      benchmark::DoNotOptimize(eval);
      benchmark::ClobberMemory();
    }
  }
}

BENCHMARK(BM_Evaluation_v1_StartPos)->Range(1, 256);

static void BM_Evaluation_v2_StartPos(benchmark::State& state) {
  Position pos;
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      int eval = pos.evaluation_v2();
      benchmark::DoNotOptimize(eval);
      benchmark::ClobberMemory();
    }
  }
}

BENCHMARK(BM_Evaluation_v2_StartPos)->Range(1, 256);

static void BM_Evaluation_StartPos(benchmark::State& state) {
  Position pos;
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      int eval = pos.evaluation();
      benchmark::DoNotOptimize(eval);
      benchmark::ClobberMemory();
    }
  }
}

BENCHMARK(BM_Evaluation_StartPos)->Range(1, 256);

static void BM_Evaluation_v1_MidPos(benchmark::State& state) {
  Position pos("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      int eval = pos.evaluation_v1();
      benchmark::DoNotOptimize(eval);
      benchmark::ClobberMemory();
    }
  }
}

BENCHMARK(BM_Evaluation_v1_MidPos)->Range(1, 256);

static void BM_Evaluation_v2_MidPos(benchmark::State& state) {
  Position pos("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      int eval = pos.evaluation_v2();
      benchmark::DoNotOptimize(eval);
      benchmark::ClobberMemory();
    }
  }
}

BENCHMARK(BM_Evaluation_v2_MidPos)->Range(1, 256);

static void BM_Evaluation_MidPos(benchmark::State& state) {
  Position pos("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      int eval = pos.evaluation();
      benchmark::DoNotOptimize(eval);
      benchmark::ClobberMemory();
    }
  }
}

BENCHMARK(BM_Evaluation_MidPos)->Range(1, 256);

BENCHMARK_MAIN();
