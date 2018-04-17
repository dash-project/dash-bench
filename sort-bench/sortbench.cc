#if defined(USE_TBB_LOWLEVEL)
#include "tbb-lowlevel/sortbench.h"
#elif defined(USE_TBB_HIGHLEVEL)
#include "tbb-highlevel/sortbench.h"
#elif defined(USE_OPENMP)
#include "openmp/sortbench.h"
#elif defined(USE_DASH)
#include <libdash.h>
#include "dash/sortbench.h"
#endif

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <thread>
#include <vector>

#include <IndexedValue.h>
#include <util/Logging.h>
#include <util/Timer.h>

#define GB (1 << 30)
#define MB (1 << 20)

template <typename RandomIt>
void trace_histo(RandomIt begin, RandomIt end)
{
#ifdef ENABLE_LOGGING
  auto const              n = std::distance(begin, end);
  std::map<key_t, size_t> hist{};
  for (size_t idx = 0; idx < n; ++idx) {
    ++hist[begin[idx]];
  }

  for (auto p : hist) {
    LOG(p.first << ' ' << p.second);
  }
#endif
}

static constexpr size_t BURN_IN = 1;
static constexpr size_t NITER   = 10;

void print_header(std::string const& app, double mb, int NTask)
{
  std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++\n";
  std::cout << "++              Sort Bench                     ++\n";
  std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++\n";
  std::cout << std::setw(20) << "NTasks: " << NTask << "\n";
  std::cout << std::setw(20) << "Size: " << std::fixed << std::setprecision(2)
            << mb << "\n";
#if defined(USE_DASH) || defined(USE_MPI)
  std::cout << std::setw(20) << "Size per Unit (MB): " << std::fixed
            << std::setprecision(2) << mb / NTask;
#endif
  std::cout << "\n\n";
  // Print the header
  std::cout << std::setw(4) << "#,";
  std::cout << std::setw(20) << app;
  std::cout << "\n";
}

//! Test sort for n items
template <typename RandomIt>
void Test(RandomIt begin, RandomIt end)
{
  auto const n = static_cast<size_t>(std::distance(begin, end));
#if defined(USE_DASH)
  auto const ThisTask = begin.pattern().team().myid();
#else
  auto const ThisTask = 0;
#endif

  LOG("N :" << n);

  static constexpr size_t           SIZE_FACTOR = 1E2;
  static std::normal_distribution<> dist{0};

  for (size_t iter = 0; iter < NITER + BURN_IN; ++iter) {
    parallel_rand(
        begin, begin + n, [](size_t total, size_t index, std::mt19937& rng) {
          // return raFailed sort order: nd() % (2 * total);
          // return index;
          // return total - index;
          using key_t = typename std::iterator_traits<RandomIt>::value_type;
          return static_cast<key_t>(std::round(dist(rng) * SIZE_FACTOR));
        });

    if (iter == 0) {
      trace_histo(begin, begin + n);
    }

    auto const start = ChronoClockNow();

    parallel_sort(begin, begin + n, std::less<key_t>());

    auto const duration = ChronoClockNow() - start;

    auto const ret = parallel_verify(begin, begin + n, std::less<key_t>());

    if (!ret) {
      std::cerr << "validation failed! (n = " << n << ")\n";
    }

    if (iter >= BURN_IN && ThisTask == 0) {
      std::cout << std::setw(3) << iter;
      std::cout << "," << std::setw(20) << std::fixed << std::setprecision(8);
      std::cout << duration;
      std::cout << "\n";
    }
  }
}

template <typename Initializer, typename Delete>
struct LibraryInitializer {
};

int main(int argc, char* argv[])
{
  using key_t = int32_t;

  if (argc < 2) {
    std::cout << std::string(argv[0]) << " [nbytes]\n";
    return 1;
  }

  // Size in Bytes
  auto const mysize = static_cast<size_t>(atoll(argv[1]));
  // Number of local elements
  auto const nl = mysize / sizeof(key_t);

#if defined(USE_DASH)
  dash::init(&argc, &argv);

  auto const NTask       = dash::size();
  auto const gsize_bytes = mysize * NTask;
  auto const N           = nl * NTask;
#else
  auto const NTask =
      (argc == 3) ? atoi(argv[2]) : std::thread::hardware_concurrency();
  auto const gsize_bytes = mysize;
  auto const N           = nl;
#endif

  double mb = (gsize_bytes / MB);

  std::string const executable(argv[0]);
  auto const        base_filename =
      executable.substr(executable.find_last_of("/\\") + 1);

#if defined(USE_DASH)
  dash::Array<key_t> keys(N);
  auto               begin = keys.begin();
#else
  key_t*     keys        = new key_t[N];
  auto       begin       = keys;
#endif

#ifdef USE_DASH
  if (dash::myid() == 0)
#endif
    print_header(base_filename, mb, NTask);

  Test(begin, begin + N);

  std::cout << "\n";

#ifndef USE_DASH
  delete[] keys;
#endif

#ifdef USE_DASH
  dash::finalize();
#endif

  return 0;
}
