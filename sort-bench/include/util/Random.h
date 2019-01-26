#ifndef RANDOM_H
#define RANDOM_H

#include <random>
#include <type_traits>

namespace sortbench {

template <typename T, bool isFP = std::is_floating_point<T>::value>
struct ChooseUniformDist {
  typedef std::uniform_real_distribution<T> type;
};

template <typename T>
struct ChooseUniformDist<T, false> {
  typedef std::uniform_int_distribution<T> type;
};

template <typename T>
struct UniformDistribution {
  using dist_type   = typename ChooseUniformDist<T>::type;
  using result_type = typename dist_type::result_type;

public:
  UniformDistribution() = default;

  UniformDistribution(T min, T max)
    : dist_{min, max}
  {
  }

  template <typename Gen>
  result_type operator()(Gen& gen)
  {
    return dist_(gen);
  }

private:
  dist_type dist_{};
};

template <
    typename T,
    bool is_floating_point = std::is_floating_point<T>::value>
struct NormalDistribution {
  using dist_type   = std::normal_distribution<T>;
  using result_type = typename dist_type::result_type;

public:
  NormalDistribution() = default;

  NormalDistribution(T mean, T stddev)
    : dist_{mean, stddev}
  {
  }

  template <typename Gen>
  result_type operator()(Gen& gen)
  {
    return dist_(gen);
  }

private:
  dist_type dist_{};
};

template <typename T>
struct NormalDistribution<T, false> {
  using dist_type   = std::normal_distribution<float>;
  using result_type = T;

  static_assert(
      sizeof(typename dist_type::result_type) <= sizeof(T),
      "sizeof(float) must be less than equals sizeof(T)");

public:
  NormalDistribution() = default;

  NormalDistribution(float mean, float stddev)
    : dist_{mean, stddev}
  {
  }

  template <typename Gen>
  result_type operator()(Gen& gen)
  {
    return std::round(dist_(gen));
  }

private:
  dist_type dist_{};
};


struct random_seed_seq {
  template <typename It>
  void generate(It begin, It end)
  {
    for (; begin != end; ++begin) {
      *begin = device();
    }
  }

  static random_seed_seq& get_instance()
  {
    static thread_local random_seed_seq result;
    return result;
  }

private:
  std::random_device device;
};


}  // namespace sortbench

#endif
