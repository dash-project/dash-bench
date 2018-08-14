#ifndef OPENMP__SORTBENCH_H__INCLUDED
#define OPENMP__SORTBENCH_H__INCLUDED
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <type_traits>

#include <intel/openmp/parallel_stable_sort.h>
#include "omp.h"

#include <util/benchdata.h>
#include <util/Logging.h>

void init_runtime(int argc, char* argv[]);
void fini_runtime();

template <class T>
std::unique_ptr<BenchData<T>> init_benchmark(size_t nlocal, size_t ntasks)
{
  return std::unique_ptr<BenchData<T>>(new BenchData<T>(
      nlocal, 0, ntasks));
}

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

#pragma omp parallel default(none) shared(begin)
  {
    auto const tid = static_cast<unsigned>(omp_get_thread_num());

    std::seed_seq seed{std::random_device{}(), tid};
    std::mt19937  rng(seed);

#pragma omp for
    for (std::size_t idx = 0; idx < n; ++idx) {
      auto it = begin + idx;
      *it     = g(n, idx, rng);
    }
  }
}

template <class T>
inline void preprocess(
    const BenchData<T>& data, size_t current_iteration, size_t n_iterations)
{
}

template <typename RandomIt, typename Compare>
inline void parallel_sort(RandomIt begin, RandomIt end, Compare cmp)
{
  if (omp_get_num_threads() > 1) {
    #pragma omp master
    pss::parallel_stable_sort(begin, end, cmp);
  } else {
    #pragma omp parallel
    #pragma omp master
    pss::parallel_stable_sort(begin, end, cmp);

  }
}

template <class T>
inline void postprocess(
    const BenchData<T>& params, size_t current_iteration, size_t n_iterations)
{
}

template <typename RandomIt, typename Compare>
inline bool parallel_verify(RandomIt begin, RandomIt end, Compare cmp)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  size_t nerror = 0;

#pragma omp parallel for reduction(+ : nerror)
  for (std::size_t idx = 1; idx < n; ++idx) {
    auto it = begin + idx;
    if (cmp(*it, *(it - 1))) {
      LOG("Failed sort order: {prev: " << *(it - 1) << ", cur: " << *it
                                       << "}");
      ++nerror;
    }
  }

  return nerror == 0;
}
#endif
