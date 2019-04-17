#ifndef SORTBENCH_H__INCLUDED
#define SORTBENCH_H__INCLUDED
#include <mpi.h>
#include <cassert>
#include <iterator>
#include <random>

#include <usort/include/binUtils.h>
#include <usort/include/ompUtils.h>
#include <usort/include/parUtils.h>

#include <util/Logging.h>

namespace sortbench {
template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  for (size_t idx = 0; idx < n; ++idx) {
    auto it = begin + idx;
    *it     = g(n, idx);
  }
}

template <typename Container, typename Cmp>
inline void parallel_sort(Container& c, Cmp cmp)
{
  auto begin = c.begin();
  auto end   = c.end();
  assert(!(end < begin));

  using value_t = typename Container::value_type;

  auto const mysize = static_cast<size_t>(std::distance(begin, end));

  ::par::HyperQuickSort_kway(c, MPI_COMM_WORLD);
}

template <typename RandomIt, typename Cmp>
inline bool parallel_verify(RandomIt begin, RandomIt end, Cmp cmp)
{
  assert(!(end < begin));

  auto const n      = static_cast<size_t>(std::distance(begin, end));
  size_t     nerror = 0;

  for (size_t idx = 1; idx < n; ++idx) {
    auto it = begin + idx;
    if (cmp(*it, *(it - 1))) {
      LOG("Failed sort order: {prev: " << *(it - 1) << ", cur: " << *it
                                       << "}");
      ++nerror;
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  return nerror == 0;
}
}  // namespace sortbench
#endif
