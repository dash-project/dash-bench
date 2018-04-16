#include <mpi.h>

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "tbb/parallel_reduce.h"
#include "tbb/parallel_sort.h"
#include "tbb/task_scheduler_init.h"

#define NITER 10
#define BURN_IN 1

int main(int argc, char* argv[])
{
  size_t i;

  if (argc != 3) {
    printf("./main [number of items] [nthreads]\n");
    return 1;
  }

  MPI_Init(NULL, NULL);

  size_t mysize   = atoll(argv[1]);
  size_t nthreads = atoi(argv[2]);

  using value_t = int64_t;

  static std::uniform_int_distribution<value_t> distribution(-1E6, 1E6);
  static std::mt19937 generator(std::random_device{}());
  // see http://www.iro.umontreal.ca/~lecuyer/myftp/papers/lfsr04.pdf
  generator.discard(700000);

  std::vector<value_t> mydata_v;
  mydata_v.reserve(mysize);

  auto mydata = mydata_v.data();

  double     mb          = (mysize * sizeof(value_t) / (1 << 20));
  auto const parallelism = nthreads;
  // auto const parallelism = tbb::task_scheduler_init::default_num_threads();

  // init task scheduler with 28 threads
  tbb::task_scheduler_init init{parallelism};

  std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++\n";
  std::cout << "++++  MP-Sort Benchmark (Shared Memory, TBB) ++++\n";
  std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++\n";
  std::cout << std::setw(11) << "NThreads: " << parallelism << "\n";
  std::cout << std::setw(11) << "Size (MB): " << std::fixed
            << std::setprecision(2) << mb;
  std::cout << "\n\n";
  // Print the header
  std::cout << std::setw(4) << "#," << std::setw(24)
            << "tbb::parallel_sort,"
            //<< std::setw(15) << "parallel sort,"
            << std::setw(18) << "std::sort"
            << "\n";

  double tbb_time = 0, dsort_time = 0;

  for (size_t iter = 0; iter < NITER + BURN_IN; ++iter) {
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, mysize),
        [&mydata](const tbb::blocked_range<size_t>& r) {
          for (size_t i = r.begin(); i != r.end(); ++i)
            mydata[i] = distribution(generator);

        });

    using range_t = tbb::blocked_range<decltype(mydata)>;

    auto const truesum = tbb::parallel_reduce(
        range_t(mydata, std::next(mydata, mysize)),
        0,
        [](range_t const& r, value_t init) {
          return std::accumulate(std::begin(r), std::end(r), init);
        },
        std::plus<value_t>());

    {
      double start = MPI_Wtime();
      tbb::parallel_sort(mydata, std::next(mydata, mysize));
      double end = MPI_Wtime();
      tbb_time   = end - start;
    }

    auto const realsum = tbb::parallel_reduce(
        range_t(mydata, std::next(mydata, mysize)),
        0,
        [](range_t const& r, value_t init) {
          return std::accumulate(r.begin(), r.end(), init);
        },
        std::plus<value_t>());

    auto const diff = realsum - truesum;

    if (diff) {
      std::cerr << "checksum fail: (" << truesum << ", " << realsum << ")\n";
      abort();
    }

    tbb::parallel_for(
        tbb::blocked_range<size_t>(1, mysize),
        [&mydata](const tbb::blocked_range<size_t>& r) {
          for (size_t i = r.begin(); i != r.end(); ++i)
            if (mydata[i] < mydata[i - 1]) {
              std::cerr << "local order fail\n";
            }
        });

    if (iter >= BURN_IN) {
      std::cout << std::setw(3) << iter << "," << std::setw(23) << std::fixed
                << std::setprecision(8) << tbb_time << "," << std::setw(18)
                << std::fixed << std::setprecision(8) << dsort_time << "\n";
    }
  }

  MPI_Finalize();

  return 0;
}
