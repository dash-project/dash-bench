#include <cassert>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <type_traits>

#include <util/Logging.h>
#include "parallel_stable_sort.h"

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  tbb::parallel_for(
      tbb::blocked_range<size_t>(0, n),
      [begin](const tbb::blocked_range<size_t>& r) {
        for (size_t i = r.begin(); i != r.end(); ++i) {
          *(begin + i) = g(n, idx, rng);
        }
      });
}
