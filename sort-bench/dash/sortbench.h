#ifndef DASH__SORTBENCH_H__INCLUDED
#define DASH__SORTBENCH_H__INCLUDED

#include <cassert>
#include <random>

#include <dash/Array.h>
#include <dash/algorithm/Generate.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Sort.h>

#include <util/Logging.h>
#include <util/benchdata.h>

void init_runtime(int argc, char* argv[]);
void fini_runtime();

static std::vector<dash::team_unit_t> trace_unit_samples;

template <class T>
std::unique_ptr<BenchData<T>> init_benchmark(size_t nlocal)
{
  assert(dash::is_initialized());

  return std::unique_ptr<BenchData<T>>(
      new BenchData<T>(nlocal, dash::myid(), dash::size()));
}

template <class T>
void preprocess(
    const BenchData<T>& params, size_t current_iteration, size_t n_iterations)
{
  if (current_iteration > 0) {
    return;
  }

  constexpr int nSamples = 250;

  // The DASH Trace does not really scale, so we select at most nSamples units
  // which trace
  trace_unit_samples.resize(nSamples);
  int id_stride = params.nTask() / nSamples;
  if (id_stride < 2) {
    std::iota(
        std::begin(trace_unit_samples), std::end(trace_unit_samples), 0);
  }
  else {
    dash::team_unit_t v_init{0};
    std::generate(
        std::begin(trace_unit_samples),
        std::end(trace_unit_samples),
        [&v_init, id_stride]() {
          auto val = v_init;
          v_init += id_stride;
          return val;
        });
  }
}

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  auto const l_range = dash::local_index_range(begin, end);

  auto       lbegin = begin.globmem().lbegin() + l_range.begin;
  auto       lend   = begin.globmem().lbegin() + l_range.end;
  auto const nl     = l_range.end - l_range.begin;

  auto const myid = begin.pattern().team().myid();

  std::seed_seq seed{std::random_device{}(), static_cast<unsigned>(myid)};
  std::mt19937  rng(seed);

  for (size_t idx = 0; idx < nl; ++idx) {
    auto it = lbegin + idx;
    *it     = g(n, idx, rng);
  }

  begin.pattern().team().barrier();
}

template <typename RandomIt, typename Cmp>
inline void parallel_sort(RandomIt begin, RandomIt end, Cmp cmp)
{
  assert(!(end < begin));

  dash::sort(begin, end);
}

template <class T>
void postprocess(
    const BenchData<T>& params,
    size_t              current_iteration,
    size_t              n_iterations)
{
  if (current_iteration + 1 < n_iterations) {
    return;
  }

  params.data().team().barrier();
  // if the id of this task is included in samples
  if ((std::find(
           std::begin(trace_unit_samples),
           std::end(trace_unit_samples),
           dash::team_unit_t{params.thisTask()}) !=
       std::end(trace_unit_samples))) {
    // dash::util::TraceStore::write(std::cout, false);
    dash::util::TraceStore::write(std::cout);
  }

  dash::util::TraceStore::off();
  dash::util::TraceStore::clear();
}

template <typename RandomIt, typename Cmp>
inline bool parallel_verify(RandomIt begin, RandomIt end, Cmp cmp)
{
  assert(!(end < begin));

  using value_t = typename dash::iterator_traits<RandomIt>::value_type;

  auto const n = static_cast<size_t>(std::distance(begin, end));

  auto const l_range = dash::local_index_range(begin, end);

  auto       lbegin = begin.globmem().lbegin() + l_range.begin;
  auto       lend   = begin.globmem().lbegin() + l_range.end;
  auto const nl     = l_range.end - l_range.begin;

  size_t nerror = 0;

  for (size_t idx = 1; idx < nl; ++idx) {
    auto const it = lbegin + idx;
    if (cmp(*it, *(it - 1))) {
      LOG("Failed local order: {prev: " << *(it - 1) << ", cur: " << *it
                                        << "}");
      ++nerror;
    }
  }

  if (nerror > 0) return false;

  auto const myid = begin.pattern().team().myid();

  if (myid > 0) {
    auto const gidx0    = begin.pattern().global(0);
    auto const prev_idx = gidx0 - 1;
    auto const prev     = static_cast<value_t>(*(begin + prev_idx));
    if (cmp(lbegin[0], prev)) {
      LOG("Failed global order: {prev: " << prev << ", cur: " << lbegin[0]
                                         << "}");
      ++nerror;
    }
  }

  begin.pattern().team().barrier();
  LOG("found " << nerror << " errors!");
  return nerror == 0;
}

#endif
