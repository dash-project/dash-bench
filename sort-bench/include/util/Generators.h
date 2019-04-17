#ifndef GENERATORS_H__INCLUDED
#define GENERATORS_H__INCLUDED

#include <util/Random.h>
#include <random>
#include <type_traits>

namespace sortbench {

constexpr double min = -1E6;
constexpr double max = 1E6;

static thread_local std::mt19937_64 generator(
    sortbench::random_seed_seq::get_instance());

template <typename key_t>
key_t normal(size_t total, size_t index)
{
  using dist_t = NormalDistribution<key_t>;
  static thread_local dist_t dist{};
  // return index;
  // return total - index;
  return dist(generator) * max;
  // return static_cast<key_t>(std::round(dist(generator) * SIZE_FACTOR));
  // return std::rand();
}

template <typename key_t>
key_t uniform(size_t total, size_t index)
{
  using dist_t = UniformDistribution<key_t>;
  static thread_local dist_t dist{key_t{min}, key_t{max}};
  // return index;
  // return total - index;
  return dist(generator);
  // return static_cast<key_t>(std::round(dist(generator) * SIZE_FACTOR));
  // return std::rand();
}

template <typename key_t>
key_t sorted(size_t total, size_t index)
{
  return static_cast<key_t>(index);
}

template <typename key_t>
key_t reverse(size_t total, size_t index)
{
  return static_cast<key_t>(total - index);
}

template <typename key_t>
key_t partial_sorted(size_t total, size_t index)
{
  using dist_t = NormalDistribution<key_t>;
  static thread_local dist_t dist{};

  const double OFFS   = 0.2;
  const double THRESH = 0.5;

  if (index > total * OFFS && index < total * (OFFS + THRESH)) {
    return static_cast<key_t>(index);
  }
  else {
    return dist(generator) * max;
  }
}

template <typename key_t>
key_t partial_sorted_in_place(size_t total, size_t index)
{
  using dist_t        = UniformDistribution<key_t>;
  const double OFFS   = 0.2;
  const double THRESH = 0.5;

  if (index <= total * OFFS) {
    static thread_local dist_t dist_low{key_t{0}, key_t{total * OFFS}};
    return dist_low(generator);
  }
  else if (index > total * OFFS && index < total * (OFFS + THRESH)) {
    return static_cast<key_t>(index);
  }
  else {
    static thread_local dist_t dist_high(
        key_t{total * (OFFS + THRESH)}, key_t{total});
    return dist_high(generator);
  }
}

}  // namespace sortbench

#endif
