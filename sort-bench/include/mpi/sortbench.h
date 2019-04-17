#ifndef SORTBENCH_H__INCLUDED
#define SORTBENCH_H__INCLUDED

#include <mpi.h>
#include <cassert>
#include <iterator>
#include <random>

extern "C" {
#ifndef MPI_COMM_WORLD
#define MPI_COMM_WORLD
#endif
#include <MP-sort/mpsort.h>
}

#include <util/Logging.h>

namespace sortbench {

template <typename T>
static void radix_value(const void* ptr, void* radix, void* arg)
{
  *reinterpret_cast<T*>(radix) = *reinterpret_cast<const T*>(ptr);
}

template <typename T>
static int compar_value(const void* a, const void* b)
{
  T const val_a = *reinterpret_cast<T const*>(a);
  T const val_b = *reinterpret_cast<T const*>(b);

  if (val_a < val_b) return -1;
  if (val_a > val_b) return 1;

  return 0;
}

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  int ThisTask;
  MPI_Comm_rank(MPI_COMM_WORLD, &ThisTask);

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

  auto* ptr = std::addressof(*begin);

  mpsort_mpi(
      ptr,
      mysize,
      sizeof(value_t),
      radix_value<value_t>,
      compar_value<value_t>,
      sizeof(value_t),
      NULL,
      MPI_COMM_WORLD);

  LOG_TRACE_RANGE("Mp_sort", begin, end);
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
