#include <mpi.h>

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include <dash/Algorithm.h>
#include <dash/Array.h>

#define NITER 10
#define BURN_IN 1

int main(int argc, char *argv[])
{
  size_t i;

  if (argc != 2) {
    printf("./main [number of items]\n");
    return 1;
  }

  dash::init(&argc, &argv);

  int ThisTask = dash::myid();
  int NTask    = dash::size();

  using value_t = int64_t;

  static std::uniform_int_distribution<value_t> distribution(-1E6, 1E6);
  static std::mt19937 generator(std::random_device{}() + ThisTask);
  // see http://www.iro.umontreal.ca/~lecuyer/myftp/papers/lfsr04.pdf
  generator.discard(700000);

  size_t               mysize = atoll(argv[1]);
  dash::Array<value_t> mydata(mysize * NTask);

  if (ThisTask == 0) {
    double mb = (mysize * NTask * sizeof(int64_t) / (1 << 20));
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++\n";
    std::cout << "++      MP-Sort Benchmark                      ++\n";
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++\n";
    std::cout << std::setw(20) << "NTasks: " << NTask << "\n";
    std::cout << std::setw(20) << "Size: " << std::fixed
              << std::setprecision(2) << mb << "\n";
    std::cout << std::setw(20) << "Size per Unit (MB): " << std::fixed
              << std::setprecision(2) << mb / NTask;
    std::cout << "\n\n";
    // Print the header
    std::cout << std::setw(4) << "#," << std::setw(15)
              << "mpsort,"
              //<< std::setw(15) << "parallel sort,"
              << std::setw(14) << "dash"
              << "\n";
  }

  double mpsort_time = 0.0, dsort_time = 0.0;

  value_t realsum, truesum;

  for (size_t iter = 0; iter < NITER + BURN_IN; ++iter) {
    std::generate(mydata.lbegin(), mydata.lend(), []() {
      return distribution(generator);
    });

    auto mysum = std::accumulate(
        &(mydata.local[0]), &(mydata.local[mysize]), static_cast<value_t>(0));

    MPI_Allreduce(&mysum, &truesum, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);

    {
      double start = MPI_Wtime();
      dash::sort(mydata.begin(), mydata.end());
      // implicit barrier in dash::sort
      double end = MPI_Wtime();
      dsort_time = end - start;
    }

    mysum = std::accumulate(
        &(mydata.local[0]), &(mydata.local[mysize]), static_cast<value_t>(0));

    static_assert(std::is_same<int64_t, long>::value, "invalid type");

    MPI_Allreduce(&mysum, &realsum, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);

    auto const diff = realsum - truesum;

    if (diff) {
      std::cerr << "checksum fail: (" << truesum << ", " << realsum
                << ", unit: " << ThisTask << ")\n";
    }

    for (i = 1; i < mysize; i++) {
      if (mydata.local[i] < mydata.local[i - 1]) {
        std::cerr << "local ordering fail\n";
      }
    }

    if (ThisTask > 0) {
      auto const gidx0    = mydata.pattern().global(0);
      auto const prev_idx = gidx0 - 1;
      auto const prev     = static_cast<value_t>(mydata[prev_idx]);
      if (mydata.local[0] < prev) {
        std::ostringstream os;

        os << "global order fail: "
           << "{task: " << ThisTask << ", prev_idx: " << prev_idx
           << ", prev: " << prev << ", local_idx: " << gidx0
           << ", local: " << mydata.local[0] << "}\n";
        std::cerr << os.str();
      }
    }

    if (iter >= BURN_IN && ThisTask == 0) {
      std::cout << std::setw(3) << iter << "," << std::setw(14) << std::fixed
                << std::setprecision(8) << mpsort_time
                << ","
                //<< std::setw(14) << std::fixed << std::setprecision(8) <<
                //psort_time << ","
                << std::setw(14) << std::fixed << std::setprecision(8)
                << dsort_time << "\n";
    }
  }

  dash::finalize();

  return 0;
}
