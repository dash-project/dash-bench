#include <mpi.h>
#include <cassert>
#include <iterator>
#include <random>

extern "C" {
#ifndef MPI_COMM_WORLD
#define MPI_COMM_WORLD
#endif
#include <MP-sort/mpsort.h>
}

#include <util/Logging.h>
#include <util/benchdata.h>

void init_runtime(int argc, char* argv[]);
void fini_runtime();

template <class T>
std::unique_ptr<BenchData<T>> init_benchmark(size_t nlocal, size_t ntasks)
{

#ifndef NDEBUG
  int flag;
  MPI_Initialized(&flag);
  assert(flag);
#endif

  int ThisTask, NTask;
  MPI_Comm_rank(MPI_COMM_WORLD, &ThisTask);
  MPI_Comm_size(MPI_COMM_WORLD, &NTask);

  return std::unique_ptr<BenchData<T>>(new BenchData<T>(nlocal, ThisTask, NTask));
}


template <typename T>
static void radix_value(const void* ptr, void* radix, void* arg)
{
  *reinterpret_cast<T*>(radix) = *reinterpret_cast<const T*>(ptr);
}

template <typename T>
static int compar_value(const void *a, const void*b)
{
  T const val_a = *reinterpret_cast<T const*>(a);
  T const val_b = *reinterpret_cast<T const*>(b);

  return (val_a > val_b) - (val_a < val_b);
}

template <class T>
void preprocess(
    const BenchData<T>& params, size_t current_iteration, size_t n_iterations)
{
}

template <class T>
void postprocess(
    const BenchData<T>& params, size_t current_iteration, size_t n_iterations)
{
}

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  int ThisTask;
  MPI_Comm_rank(MPI_COMM_WORLD, &ThisTask);

  std::seed_seq seed{std::random_device{}(), static_cast<unsigned>(ThisTask)};
  std::mt19937  rng(seed);

  for (size_t idx = 0; idx < n; ++idx) {
      auto it = begin + idx;
      *it     = g(n, idx, rng);
  }
}

template <typename RandomIt, typename Cmp>
inline void parallel_sort(RandomIt begin, RandomIt end, Cmp cmp)
{
  assert(!(end < begin));

  using value_t = typename std::iterator_traits<RandomIt>::value_type;

  auto const mysize = static_cast<size_t>(std::distance(begin, end));

  mpsort_mpi(
      begin,
      mysize,
      sizeof(value_t),
      radix_value<value_t>,
      compar_value<value_t>,
      sizeof(value_t),
      NULL,
      MPI_COMM_WORLD);
}

template <typename RandomIt, typename Cmp>
inline bool parallel_verify(RandomIt begin, RandomIt end, Cmp cmp)
{
  assert(!(end < begin));

  auto const n      = static_cast<size_t>(std::distance(begin, end));
  size_t     nerror = 0;

  for (size_t idx = 0; idx < n; ++idx) {
    auto it = begin + idx;
    if (cmp(*it, *(it - 1))) {
      LOG("Failed sort order: {prev: " << *(it - 1) << ", cur: " << *it
                                       << "}");
      ++nerror;
    }
  }

  return nerror == 0;
}
