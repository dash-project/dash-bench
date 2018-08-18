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

#ifdef ENABLE_MEMKIND
#include <memkind.h>
#endif

#include <intel/IndexedValue.h>

#include <util/Logging.h>
#include <util/Random.h>
#include <util/Timer.h>
#include <util/benchdata.h>

#include <util/likwid.h>

#define GB (1 << 30)
#define MB (1 << 20)

static constexpr size_t BURN_IN = 1;
static constexpr size_t NITER   = 10;

template<class T>
void print_header(BenchData<T> const & benchmarkData, std::string const& app)
{
  auto const mb = benchmarkData.nglobal() * sizeof(T) / MB;
  auto const NTask = benchmarkData.nTask();

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
  auto end = begin + benchmarkData.nglobal();
#else
  auto begin = benchmarkData.data();
  auto end = begin + benchmarkData.nlocal();
#endif

#if defined(USE_TBB_HIGHLEVEL) || defined(USE_TBB_LOWLEVEL)
  tbb::task_scheduler_init init(benchmarkData.nTask());
#endif


  using key_t = typename std::iterator_traits<decltype(begin)>::value_type;

  auto const mb = benchmarkData.nglobal() * sizeof(key_t) / MB;

  // using dist_t = sortbench::NormalDistribution<key_t>;
  using dist_t = sortbench::UniformDistribution<key_t>;

  // dist_t dist{50, 10};
  //static dist_t dist{key_t{0}, key_t{(1 << 20)}};
  static dist_t dist{key_t(-1E6), key_t(1E6)};

  LIKWID_MARKER_INIT;

#ifdef USE_OPENMP
  #pragma omp parallel
    {
        // Each thread must add itself to the Marker API, therefore must be
        // in parallel region
        LIKWID_MARKER_THREADINIT;
        // Optional. Register region name
        LIKWID_MARKER_REGISTER("sort");
    }
#else
  // Optional. Register region name
  LIKWID_MARKER_REGISTER("sort");
#endif

  for (size_t iter = 0; iter < NITER + BURN_IN; ++iter) {
    auto const last_iter = (iter == NITER + BURN_IN - 1);
  LOG("parallel rand");
    parallel_rand(
        begin, end, [](size_t total, size_t index, std::mt19937& rng) {
          // return index;
          // return total - index;
          return dist(rng);
          // return static_cast<key_t>(std::round(dist(rng) * SIZE_FACTOR));
          // return std::rand();
        });

    LOG("preprocess");
    preprocess(benchmarkData, iter, NITER + BURN_IN);

    auto const start = ChronoClockNow();

    LOG("parallel sort");
#ifdef USE_OPENMP
#pragma omp parallel
{
#endif
  if (last_iter) {
    LIKWID_MARKER_START("sort");
  }

    parallel_sort(begin, end, std::less<key_t>());

  if (last_iter) {
    LIKWID_MARKER_STOP("sort");
  }
#ifdef USE_OPENMP
}
#endif

    auto const duration = ChronoClockNow() - start;

    LOG("parallel verify");
    auto const ret = parallel_verify(begin, end, std::less<key_t>());
    LOG("postprocess");
    postprocess(benchmarkData, iter, NITER + BURN_IN);

    if (!ret) {
      std::cerr << "validation failed! (n = " << std::distance(begin, end) << ")\n";
    }

    if (iter >= BURN_IN && benchmarkData.thisProc() == 0) {
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

  LIKWID_MARKER_CLOSE;
}

int main(int argc, char* argv[])
{
  using key_t = double;

  if (argc < 2) {
    std::cout << std::string(argv[0])
#if defined(USE_DASH) || defined(USE_MPI)
              << " [nbytes per task]\n";
#else
              << " [nbytes]\n";
#endif
    return 1;
  }

#ifdef ENABLE_MEMKIND
  auto const res = hbw_set_policy(HBW_POLICY_BIND);
  assert(res == 0);
#endif


  // Size in Bytes
  auto const nbytes = static_cast<size_t>(atoll(argv[1]));
  // This is only relevant for OpenMP / TBB
  // DASH and MPI use the number of units / procs
  auto const ntasks = (argc == 3) ? static_cast<size_t>(atoll(argv[2])) :
    std::thread::hardware_concurrency();
  // Number of local elements
  auto const nlocal = nbytes / sizeof(key_t);

  LOG("init runtime");
  //Load Runtime
  init_runtime(argc, argv);

  LOG("init benchmark data");
  // setup benchmark
  auto benchmarkData = init_benchmark<key_t>(nlocal, ntasks);

  std::string const executable(argv[0]);
  auto const        base_filename =
      executable.substr(executable.find_last_of("/\\") + 1);

  if (benchmarkData->thisProc() == 0) {
    print_header(*benchmarkData, base_filename);
  }

  LOG("running benchmark");
  Test(*benchmarkData, base_filename);

  fini_runtime();

  if (benchmarkData->thisProc() == 0) {
    std::cout << "\n";
  }

  return 0;
}
