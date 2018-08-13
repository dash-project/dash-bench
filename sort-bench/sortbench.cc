#include <chrono>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <thread>
#include <vector>

#if defined(USE_TBB_HIGHLEVEL) || defined(USE_TBB_LOWLEVEL)
#include <tbb/sortbench.h>
#include <tbb/task_scheduler_init.h>
#elif defined(USE_OPENMP)
#include <openmp/sortbench.h>
#elif defined(USE_DASH)
#include <dash/sortbench.h>
#include <libdash.h>
#elif defined(USE_MPI)
#include <mpi/sortbench.h>
#endif

#include <intel/IndexedValue.h>

#include <util/Logging.h>
#include <util/Random.h>
#include <util/Timer.h>

#define GB (1 << 30)
#define MB (1 << 20)

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
  std::cout << std::setw(10) << "NTasks,";
  std::cout << std::setw(10) << "Size (MB),";
  std::cout << std::setw(20) << "Time,";
  std::cout << std::setw(20) << "Test Case";
  std::cout << "\n";
}

//! Test sort for n items
template <typename T>
void Test(BenchData<T>& benchmarkData, std::string const& test_case)
{
#ifdef USE_DASH
  auto begin = benchmarkData.data().begin();
#else
  auto begin = benchmarkData.data();
#endif

  auto end = begin + benchmarkData.nglobal();

  auto const n = static_cast<size_t>(std::distance(begin, end));

  using key_t = typename std::iterator_traits<decltype(begin)>::value_type;

  auto const mb = n * benchmarkData.nglobal() * sizeof(key_t) / MB;

  // using dist_t = sortbench::NormalDistribution<key_t>;
  using dist_t = sortbench::UniformDistribution<key_t>;

  // dist_t dist{50, 10};
  static dist_t dist{key_t{0}, key_t{(1 << 20)}};

  for (size_t iter = 0; iter < NITER + BURN_IN; ++iter) {
    parallel_rand(
        begin, begin + n, [](size_t total, size_t index, std::mt19937& rng) {
          // return index;
          // return total - index;
          return dist(rng);
          // return static_cast<key_t>(std::round(dist(rng) * SIZE_FACTOR));
          // return std::rand();
        });

    preprocess(benchmarkData, iter, iter + BURN_IN);

    auto const start = ChronoClockNow();

    parallel_sort(begin, begin + n, std::less<key_t>());

    auto const duration = ChronoClockNow() - start;

    auto const ret = parallel_verify(begin, begin + n, std::less<key_t>());

    postprocess(benchmarkData, iter, iter + BURN_IN);

    if (!ret) {
      std::cerr << "validation failed! (n = " << n << ")\n";
    }

    if (iter >= BURN_IN && benchmarkData.thisTask() == 0) {
      std::ostringstream os;
      // Iteration
      os << std::setw(3) << iter << ",";
      // Ntasks
      os << std::setw(9) << benchmarkData.nTask() << ",";
      // Size
      os << std::setw(9) << std::fixed << std::setprecision(2) << mb;
      os << ",";
      // Time (s)
      os << std::setw(19) << std::fixed << std::setprecision(8);
      os << duration << ",";
      // Test Case
      os << std::setw(20) << test_case;
      os << "\n";
      std::cout << os.str();
    }

  }
}

int main(int argc, char* argv[])
{
  using key_t = int32_t;

  if (argc < 2) {
    std::cout << std::string(argv[0])
#if defined(USE_DASH) || defined(USE_MPI)
              << " [nbytes per task]\n";
#else
              << " [nbytes]\n";
#endif
    return 1;
  }

  // Size in Bytes
  auto const mysize = static_cast<size_t>(atoll(argv[1]));
  // Number of local elements
  auto const nl = mysize / sizeof(key_t);

  init_runtime(argc, argv);
  auto benchmarkData = init_benchmark<key_t>(nl);

  auto const mb = benchmarkData->nglobal() * sizeof(key_t) / MB;

  std::string const executable(argv[0]);
  auto const        base_filename =
      executable.substr(executable.find_last_of("/\\") + 1);

  if (benchmarkData->thisTask() == 0) {
    print_header(base_filename, mb, benchmarkData->nTask());
  }

  Test(*benchmarkData, base_filename);

  fini_runtime();

  if (benchmarkData->thisTask() == 0) {
    std::cout << "\n";
  }

  return 0;
}
