#ifndef SORTBENCH_H__INCLUDED
#define SORTBENCH_H__INCLUDED

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <type_traits>

#include <util/Logging.h>

#include <intel/openmp/parallel_stable_sort.h>
#include "omp.h"

namespace sortbench {

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

#pragma omp parallel default(none) shared(begin)
  {
    auto const tid = static_cast<unsigned>(omp_get_thread_num());

#pragma omp for
    for (std::size_t idx = 0; idx < n; ++idx) {
      auto it = begin + idx;
      *it     = g(n, idx);
    }
  }
}

template <typename Container, typename Cmp>
inline void parallel_sort(Container& c, Cmp cmp)
{
  auto begin = c.begin();
  auto end   = c.end();
  if (rand() & 0x100) {
#pragma omp parallel
#pragma omp master
    // Check that sort works when already in a parallel region.
    pss::parallel_stable_sort(begin, end, cmp);
  }
  else {
    pss::parallel_stable_sort(begin, end, cmp);
  }
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
}  // namespace sortbench
#endif
