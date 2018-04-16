#if defined(USE_CILKPLUS)
#include "cilkplus/parallel_stable_sort.h"
#elif defined(USE_TBB_LOWLEVEL)
#include "tbb-lowlevel/parallel_stable_sort.h"
#elif defined(USE_TBB_HIGHLEVEL)
#include "tbb-highlevel/parallel_stable_sort.h"
#elif defined(USE_OPENMP)
#include "openmp/parallel_stable_sort.h"
#include "openmp/sortbench.h"
#endif

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <vector>

#include <IndexedValue.h>
#include <util/Logging.h>
#include <util/Timer.h>

#define GB (1 << 30)
#define MB (1 << 20)

#if 0
#ifdef ENABLE_LOGGING
  {
    std::map<key_t, size_t> hist{};
    for (size_t idx = 0; idx < n; ++idx) {
      ++hist[begin[idx]];
    }

    for (auto p : hist) {
      std::cout << std::setw(2) << p.first << ' ' << p.second << '\n';
    }
  }
#endif
#endif

//! Test sort for n items
template <typename T>
void Test(size_t n)
{
  using key_t = T;

#if defined(USE_DASH)
  // use dash array
#else
  LOG("Testing for n = " << n << "...");
  std::vector<key_t> keys(n);
  auto               begin = std::begin(keys);
#endif

  auto const                        min = -10;
  auto const                        max = 10;
  static std::normal_distribution<> dist{(min + max) / 2};

  auto const start = ChronoClockNow();

  parallel_rand(
      begin, begin + n, [](size_t total, size_t index, std::mt19937& rng) {
        // return raFailed sort order: nd() % (2 * total);
        // return index;
        // return total - index;
        return std::round(dist(rng));
      });

  auto const duration = ChronoClockNow() - start;

  LOG_TRACE_RANGE("before sort", begin, begin + n);

  parallel_sort(begin, begin + n, std::less<key_t>());

  LOG_TRACE_RANGE("after sort", begin, begin + n);

  auto const ret = parallel_verify(begin, begin + n, std::less<key_t>());

  if (!ret) {
    std::cerr << "validation failed! (n = " << n << ")\n";
  }
}

int main()
{
  using key_t        = int32_t;
  const size_t N_MAX = (static_cast<size_t>(2) * GB) / sizeof(key_t);
  for (size_t n = 0; n <= N_MAX; n = n < 10 ? n + 1 : n * 1.618f) {
    Test<key_t>(n);
  }
  printf("\n");
  return 0;
}
