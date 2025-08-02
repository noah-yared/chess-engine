#include <benchmark/benchmark.h>

#include "position.h"

static void BM_EvaluationV1_StartPos(benchmark::State& state) {
  Position pos;
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      benchmark::DoNotOptimize(pos.evaluation<EvalV1>());
    }
  }
}

BENCHMARK(BM_EvaluationV1_StartPos)->Range(1, 256);

static void BM_EvaluationV2_StartPos(benchmark::State& state) {
  Position pos;
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      benchmark::DoNotOptimize(pos.evaluation<EvalV2>());
    }
  }
}

BENCHMARK(BM_EvaluationV2_StartPos)->Range(1, 256);

static void BM_EvaluationV3_StartPos(benchmark::State& state) {
  Position pos;
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      benchmark::DoNotOptimize(pos.evaluation<EvalV3>());
    }
  }
}

BENCHMARK(BM_EvaluationV3_StartPos)->Range(1, 256);

static void BM_EvaluationV1_MidPos(benchmark::State& state) {
  Position pos("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      benchmark::DoNotOptimize(pos.evaluation<EvalV1>());
    }
  }
}

BENCHMARK(BM_EvaluationV1_MidPos)->Range(1, 256);

static void BM_EvaluationV2_MidPos(benchmark::State& state) {
  Position pos("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      benchmark::DoNotOptimize(pos.evaluation<EvalV2>());
    }
  }
}

BENCHMARK(BM_EvaluationV2_MidPos)->Range(1, 256);

static void BM_EvaluationV3_MidPos(benchmark::State& state) {
  Position pos("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
  long long N = state.range(0);
  for (auto _ : state) {
    for (long long i = 0; i < N; ++i) {
      benchmark::DoNotOptimize(pos.evaluation<EvalV3>());
    }
  }
}

BENCHMARK(BM_EvaluationV3_MidPos)->Range(1, 256);

BENCHMARK_MAIN();
