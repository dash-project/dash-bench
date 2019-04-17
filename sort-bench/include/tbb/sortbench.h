#ifndef SORTBENCH_H__INCLUDED
#define SORTBENCH_H__INCLUDED

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

namespace sortbench {

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  tbb::parallel_for(
      tbb::blocked_range<size_t>(0, n),
      [begin, n, g](const tbb::blocked_range<size_t>& r) {
        for (size_t idx = r.begin(); idx != r.end(); ++idx) {
          *(begin + idx) = g(n, idx);
        }
      });
}

template <typename Container, typename Cmp>
inline void parallel_sort(Container& c, Cmp cmp)
{
  auto begin = c.begin();
  auto end   = c.end();
  pss::parallel_stable_sort(begin, end, cmp);
}

template <typename RandomIt, typename Compare>
inline bool parallel_verify(RandomIt begin, RandomIt end, Compare cmp)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  size_t nerror = 0;

  tbb::parallel_for(
      tbb::blocked_range<size_t>(1, n),
      [begin, cmp, &nerror](const tbb::blocked_range<size_t>& r) {
        for (size_t idx = r.begin(); idx != r.end(); ++idx) {
          auto it = begin + idx;
          if (cmp(*it, *(it - 1))) {
            LOG("Failed sort order: {prev: " << *(it - 1) << ", cur: " << *it
                                             << "}");
            ++nerror;
          }
        }
      });

  return nerror == 0;
}
}  // namespace sortbench
#endif
