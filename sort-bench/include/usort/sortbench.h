#include <mpi.h>
#include <cassert>
#include <iterator>
#include <random>

#include <usort/include/binUtils.h>
#include <usort/include/ompUtils.h>
#include <usort/include/parUtils.h>

#include <util/Logging.h>

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  int ThisTask;
  MPI_Comm_rank(MPI_COMM_WORLD, &ThisTask);

  std::seed_seq seed{std::random_device{}(), static_cast<unsigned>(ThisTask)};
  std::mt19937  rng(seed);

  for (size_t idx = 0; idx < n; ++idx) {
      auto it = begin + idx;
      *it     = g(n, idx, rng);
  }
}

template <typename Container, typename Cmp>
inline void parallel_sort(Container & c, Cmp cmp)
{
  auto begin = c.begin();
  auto end = c.end();
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

  return nerror == 0;
}
