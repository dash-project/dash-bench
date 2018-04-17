#include <cassert>
#include <random>

#include <dash/Array.h>
#include <dash/algorithm/Generate.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Sort.h>

#include <util/Logging.h>

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

  //implicit barrier in dash::sort
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
