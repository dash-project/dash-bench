#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <type_traits>

#include <tbb/parallel_for.h>

#include <util/Logging.h>

#ifdef USE_TBB_HIGHLEVEL
#include <intel/tbb-highlevel/parallel_stable_sort.h>
#elif defined(USE_TBB_LOWLEVEL)
#include <intel/tbb-lowlevel/parallel_stable_sort.h>
#endif


void init_runtime(int argc, char* argv[]);
void fini_runtime();

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  std::seed_seq seed{std::random_device{}()};
  std::mt19937  rng(seed);

  tbb::parallel_for(
    tbb::blocked_range<size_t>(0, n),
    [begin, n, g, &rng](const tbb::blocked_range<size_t>& r)
  {
    for (size_t idx = r.begin(); idx != r.end(); ++idx)
    {
      *(begin + idx) = g(n, idx, rng);
    }
  });
}

template <typename RandomIt, typename Compare>
inline void parallel_sort(RandomIt begin, RandomIt end, Compare cmp)
{
  pss::parallel_stable_sort(begin, end, cmp);
}

template <class T>
void preprocess(
    const BenchData<T>& params, size_t current_iteration, size_t n_iterations)
{
}

template <class T>
void postprocess(
    const BenchData<T>& params, size_t current_iteration, size_t n_iterations)
{
}

template <typename RandomIt, typename Compare>
inline bool parallel_verify(RandomIt begin, RandomIt end, Compare cmp)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  size_t nerror = 0;

  tbb::parallel_for(
    tbb::blocked_range<size_t>(1, n),
    [begin, cmp, &nerror](const tbb::blocked_range<size_t>& r)
  {
    for (size_t idx = r.begin(); idx != r.end(); ++idx)
    {
      auto it = begin + idx;
      if (cmp(*it, *(it - 1)))
      {
        LOG("Failed sort order: {prev: " << * (it - 1) << ", cur: " << *it
            << "}");
        ++nerror;
      }
    }
  });

  return nerror == 0;
}
