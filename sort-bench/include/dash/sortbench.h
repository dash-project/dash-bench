#ifndef SORTBENCH_H__INCLUDED
#define SORTBENCH_H__INCLUDED

#include <cassert>
#include <random>

#include <dash/Array.h>
#include <dash/algorithm/Generate.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Sort.h>

#include <util/Logging.h>
#include <util/Random.h>

namespace sortbench {

template <typename RandomIt, typename Gen>
inline void parallel_rand(RandomIt begin, RandomIt end, Gen const g)
{
  assert(!(end < begin));

  auto const n = static_cast<size_t>(std::distance(begin, end));

  auto const l_range = dash::local_index_range(begin, end);

  using pointer = typename std::iterator_traits<RandomIt>::pointer;

  auto* lbegin = dash::local_begin(
      static_cast<pointer>(begin), begin.pattern().team().myid());
  auto*      lend = std::next(lbegin, l_range.end);
  auto const nl   = l_range.end - l_range.begin;

  for (size_t idx = 0; idx < nl; ++idx) {
    auto       it   = lbegin + idx;
    auto const gidx = begin.pattern().global(idx);
    *it             = g(n, gidx);
  }

  begin.pattern().team().barrier();
}

template <typename Container, typename Cmp>
inline void parallel_sort(Container& c, Cmp cmp)
{
  auto begin = c.begin();
  auto end   = c.end();
  assert(!(end < begin));

  dash::sort(begin, end);

  // implicit barrier in dash::sort
}

template <typename RandomIt, typename Cmp>
inline bool parallel_verify(RandomIt begin, RandomIt end, Cmp cmp)
{
  assert(!(end < begin));

  using value_t = typename dash::iterator_traits<RandomIt>::value_type;

  auto const n = static_cast<size_t>(std::distance(begin, end));

  auto const l_range = dash::local_index_range(begin, end);

  using pointer = typename std::iterator_traits<RandomIt>::pointer;

  auto* lbegin = dash::local_begin(
      static_cast<pointer>(begin), begin.pattern().team().myid());
  auto*      lend = std::next(lbegin, l_range.end);
  auto const nl   = l_range.end - l_range.begin;

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
}  // namespace sortbench
#endif
