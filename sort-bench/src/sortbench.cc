#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <thread>
#include <vector>
#include <algorithm>

#if defined(USE_TBB_HIGHLEVEL) || defined(USE_TBB_LOWLEVEL)
#include <tbb/sortbench.h>
#include <tbb/task_scheduler_init.h>
#elif defined(USE_OPENMP)
#include <openmp/sortbench.h>
#elif defined(USE_DASH)
#ifdef DASH_ENABLE_PSTL
#include <tbb/task_scheduler_init.h>
#endif
#include <dash/sortbench.h>
#include <dash/util/BenchmarkParams.h>
#include <omp.h>
#elif defined(USE_MPI)
#include <mpi/sortbench.h>
#elif defined(USE_USORT)
#include <usort/sortbench.h>
#endif

#include <intel/IndexedValue.h>

#include <util/Generators.h>
#include <util/Logging.h>
#include <util/Random.h>
#include <util/Timer.h>
#include <util/Trace.h>

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

void print_header(std::string const& app, double mb, int P)
{
  std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++\n";
  std::cout << "++              Sort Bench                     ++\n";
  std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++\n";
  std::cout << std::setw(20) << "NTasks: " << P << "\n";
  std::cout << std::setw(20) << "Size: " << std::fixed << std::setprecision(2)
            << mb << "\n";
#if defined(USE_DASH) || defined(USE_MPI) || defined(USE_USORT)
  std::cout << std::setw(20) << "Size per Unit (MB): " << std::fixed
            << std::setprecision(2) << mb / P;
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

constexpr size_t max_samples = 250;
auto             sample_units(size_t P)
{
  // The DASH Trace does not really scale, so we select at most nSamples units
  auto nSamples = std::min(P, max_samples);

  std::vector<unsigned> trace_unit_samples(nSamples);
  int                   id_stride = P / nSamples;
  if (id_stride < 2) {
    std::iota(
        std::begin(trace_unit_samples), std::end(trace_unit_samples), 0);
  }
  else {
    unsigned v_init{0};
    std::generate(
        std::begin(trace_unit_samples),
        std::end(trace_unit_samples),
        [&v_init, id_stride]() {
          auto val = v_init;
          v_init += id_stride;
          return val;
        });
  }

  return trace_unit_samples;
}

//! Test sort for n items
template <class Container>
void Test(
    Container& c, size_t N, int r, size_t P, std::string const& test_case)
{
  LOG("N :" << N);

  using key_t = typename Container::value_type;

  auto const mb = N * sizeof(key_t) / MB;

  auto trace_unit_samples = sample_units(P);

  for (size_t iter = 0; iter < NITER + BURN_IN; ++iter) {
    sortbench::reset_trace();

    sortbench::parallel_rand(c.begin(), c.end(), sortbench::normal<key_t>);

    auto const start = ChronoClockNow();

    sortbench::parallel_sort(c, std::less<key_t>());

    auto const duration = ChronoClockNow() - start;

    auto const ret =
        sortbench::parallel_verify(c.begin(), c.end(), std::less<key_t>());

    if (!ret) {
      std::cerr << "validation failed! (n = " << N << ")\n";
    }

    // c.begin().pattern().team().barrier();
    if (iter == (NITER + BURN_IN - 1)) {
      sortbench::flush_trace(trace_unit_samples, r);
    }

    if (iter >= BURN_IN && r == 0) {
      std::ostringstream os;
      // Iteration
      os << std::setw(3) << iter << ",";
      // Ntasks
      os << std::setw(9) << P << ",";
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
  using key_t = double;

  if (argc < 2) {
    std::cout << std::string(argv[0])
#if defined(USE_DASH) || defined(USE_MPI) || defined(USE_USORT)
              << " [nbytes per rank]\n";
#else
              << " [nbytes]\n";
#endif
    return 1;
  }

  // Size in Bytes
  auto const mysize = static_cast<size_t>(atoll(argv[1]));
  // Number of local elements
  auto const nl = mysize / sizeof(key_t);
  // Number of threads
  auto const T = (argc == 3) ? atoi(argv[2]) : 0;

#if defined(USE_DASH)
  dash::init(&argc, &argv);

  auto const P           = dash::size();
  auto const gsize_bytes = mysize * P;
  auto const N           = nl * P;
  auto const r           = dash::myid();
#elif defined(USE_MPI) || defined(USE_USORT)
  MPI_Init(&argc, &argv);
  int P;
  MPI_Comm_size(MPI_COMM_WORLD, &P);
  auto const gsize_bytes = mysize * P;
  auto const N = nl * P;
  int r;
  MPI_Comm_rank(MPI_COMM_WORLD, &r);
#else
  auto const P = T ? T : std::thread::hardware_concurrency();
  assert(P > 0);
  auto const gsize_bytes = mysize;
  auto const N           = nl;
  auto const r           = 0;
#endif

#if defined(USE_TBB_HIGHLEVEL) || defined(USE_TBB_LOWLEVEL)
  tbb::task_scheduler_init init{static_cast<int>(P)};
#elif defined(USE_OPENMP)
  omp_set_num_threads(P);
#elif defined(USE_USORT)
  if (T) {
    omp_set_num_threads(T);
  }
#endif

  double mb = (gsize_bytes / MB);

  std::string const executable(argv[0]);
  auto const        base_filename =
      executable.substr(executable.find_last_of("/\\") + 1);

#if defined(USE_DASH)
  dash::Array<key_t> keys(N);
#else
  std::vector<key_t> keys(nl);
#endif

  if (r == 0) {
#if defined(USE_DASH)
    dash::util::BenchmarkParams bench_params("bench.dash.sort");
    bench_params.set_output_width(72);
    bench_params.print_header();
    if (dash::size() < 200) {
      bench_params.print_pinning();
    }
#endif
    print_header(base_filename, mb, P);
  }

  Test(keys, N, r, P, base_filename);

#if defined(USE_DASH)
  dash::finalize();
#elif defined(USE_MPI) || defined(USE_USORT)
  MPI_Finalize();
#endif

  if (r == 0) {
    std::cout << "\n";
  }

  return 0;
}
