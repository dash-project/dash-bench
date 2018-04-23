#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
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

template <typename RandomIt>
void trace_histo(RandomIt begin, RandomIt end)
{
#ifdef ENABLE_LOGGING
  auto const n = std::distance(begin, end);
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
  std::cout << std::setw(10) << "NTasks,";
  std::cout << std::setw(10) << "Size (MB),";
  std::cout << std::setw(20) << "Time,";
  std::cout << std::setw(20) << "Test Case";
  std::cout << "\n";
}

//! Test sort for n items
template <typename RandomIt>
void Test(RandomIt begin, RandomIt end, int ThisTask, int NTask, std::string const & test_case)
{
  auto const n = static_cast<size_t>(std::distance(begin, end));
  LOG("N :" << n);

  using key_t = typename std::iterator_traits<RandomIt>::value_type;

#ifdef USE_MPI
  auto const mb = n * NTask * sizeof(key_t) / MB;
#else
  auto const mb = n * sizeof(key_t) / MB;
#endif

  //using dist_t = sortbench::NormalDistribution<key_t>;
  using dist_t = sortbench::UniformDistribution<key_t>;

  //dist_t dist{50, 10};
  dist_t dist{key_t{0}, key_t{(1 << 20)}};

  for (size_t iter = 0; iter < NITER + BURN_IN; ++iter) {
    parallel_rand(
        begin,
        begin + n,
        [&dist](size_t total, size_t index, std::mt19937& rng) {
          // return index;
          // return total - index;
          return dist(rng);
          // return static_cast<key_t>(std::round(dist(rng) * SIZE_FACTOR));
        });

    if (iter == 0) {
      trace_histo(begin, begin + n);
    }

#ifdef USE_DASH
    dash::util::TraceStore::on();
    dash::util::TraceStore::clear();

#endif

    auto const start = ChronoClockNow();

    parallel_sort(begin, begin + n, std::less<key_t>());

    auto const duration = ChronoClockNow() - start;

#ifdef USE_DASH
    if (iter == (NITER + BURN_IN - 1)) {
      dash::util::TraceStore::write(std::cout);
    }

    dash::util::TraceStore::off();
    dash::util::TraceStore::clear();
    begin.pattern().team().barrier();
#endif

    auto const ret = parallel_verify(begin, begin + n, std::less<key_t>());

    if (!ret) {
      std::cerr << "validation failed! (n = " << n << ")\n";
    }

    if (iter >= BURN_IN && ThisTask == 0) {
      //Iteration
      std::cout << std::setw(3) << iter << ",";
      //Ntasks
      std::cout << std::setw(9) << NTask << ",";
      //Size
      std::cout << std::setw(9) << std::fixed << std::setprecision(2) << mb;
      std::cout << ",";
      //Time (s)
      std::cout << std::setw(19) << std::fixed << std::setprecision(8);
      std::cout << duration << ",";
      //Test Case
      std::cout << std::setw(20) << test_case;
      std::cout << "\n";
    }
  }
}

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
  auto const ThisTask    = dash::myid();
#elif defined(USE_MPI)
  MPI_Init(&argc, &argv);
  int NTask;
  MPI_Comm_size(MPI_COMM_WORLD, &NTask);
  auto const gsize_bytes = mysize * NTask;
  auto const N           = nl;
  int        ThisTask;
  MPI_Comm_rank(MPI_COMM_WORLD, &ThisTask);
#else
  auto const NTask =
      (argc == 3) ? atoi(argv[2]) : std::thread::hardware_concurrency();
  assert(NTask > 0);
  auto const gsize_bytes = mysize;
  auto const N           = nl;
  auto const ThisTask    = 0;
#endif

#if defined(USE_TBB_HIGHLEVEL) || defined(USE_TBB_LOWLEVEL)
  tbb::task_scheduler_init init{static_cast<int>(NTask)};
#elif defined(USE_OPENMP)
  omp_set_num_threads(NTask);
#endif

  double mb = (gsize_bytes / MB);

  std::string const executable(argv[0]);
  auto const        base_filename =
      executable.substr(executable.find_last_of("/\\") + 1);

#if defined(USE_DASH)
  dash::Array<key_t> keys(N);
  auto               begin = keys.begin();
#else
  key_t* keys  = new key_t[N];
  auto   begin = keys;
#endif

  if (ThisTask == 0) {
#if defined(USE_DASH)
    dash::util::BenchmarkParams bench_params("bench.dash.sort");
    bench_params.set_output_width(72);
    bench_params.print_header();
    bench_params.print_pinning();
#endif
    print_header(base_filename, mb, NTask);
  }

  Test(begin, begin + N, ThisTask, NTask, base_filename);

#ifndef USE_DASH
  delete[] keys;
#endif

#if defined(USE_DASH)
  dash::finalize();
#elif defined(USE_MPI)
  MPI_Finalize();
#endif

  if (ThisTask == 0) {
    std::cout << "\n";
  }

  return 0;
}
