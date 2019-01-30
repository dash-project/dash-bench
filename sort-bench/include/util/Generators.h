#ifndef GENERATORS_H
#define GENERATORS_H

#include <random>
#include <type_traits>
#include <fstream>
#include <array>
#include <util/Random.h>

namespace sortbench {

template<typename key_t>
key_t normal(size_t total, size_t index, std::mt19937& rng) {
  using dist_t = NormalDistribution<key_t>;
  static dist_t dist{};
  // return index;
  // return total - index;
  return dist(rng) * 1E6;
  // return static_cast<key_t>(std::round(dist(rng) * SIZE_FACTOR));
  // return std::rand();
}

template<typename key_t>
key_t uniform(size_t total, size_t index, std::mt19937& rng) {
  using dist_t = UniformDistribution<key_t>;
  static dist_t dist{};
  // return index;
  // return total - index;
  return dist(rng) * 1E6;
  // return static_cast<key_t>(std::round(dist(rng) * SIZE_FACTOR));
  // return std::rand();
}

template<typename key_t>
key_t sorted(size_t total, size_t index, std::mt19937& rng) {
  return static_cast<key_t>(index);
}

template<typename key_t>
key_t reverse(size_t total, size_t index, std::mt19937& rng) {
  return static_cast<key_t>(total - index);
}

template<typename key_t>
key_t partial_sorted(size_t total, size_t index, std::mt19937& rng) {
  const double OFFS   = 0.2;
  const double THRESH = 0.5;

  if(index > total*OFFS && index < total*(OFFS+THRESH)) {
    return static_cast<key_t>(index);
  } else {
    using dist_t = NormalDistribution<key_t>;
    static dist_t dist{};
    return dist(rng) * 1E6;
  }
}

template<typename key_t>
key_t partial_sorted_in_place(size_t total, size_t index, std::mt19937& rng) {
  const double OFFS   = 0.2;
  const double THRESH = 0.5;

  using dist_t = UniformDistribution<key_t>;
  if(index <= total*OFFS) {
    static dist_t dist_low(0.0,total*OFFS);
    return dist_low(rng);
  }
  else if(index > total*OFFS && index < total*(OFFS+THRESH)) {
    return static_cast<key_t>(index);
  } else {
    static dist_t dist_high(total*(OFFS+THRESH), total);
    return dist_high(rng);
  }
}

template<class key_t>
class gensort_ascii {
public:
  gensort_ascii(std::string const & filename)
  : _filename(filename) {}

  key_t operator()(size_t total, size_t index, std::mt19937& rng) const {
    static std::ifstream ifs(_filename);
    ifs.seekg(index*sizeof(key_t));
    key_t ret;
    ifs.getline(ret.data(),ret.size());
    return std::move(ret);
  }

private:
  std::string const _filename;
};

template<class key_t>
class gensort_binary {
public:
  gensort_binary(std::string const & filename)
  : _filename(filename) {}

  key_t operator()(size_t total, size_t index, std::mt19937& rng) const {
    static std::ifstream ifs(_filename);
    ifs.seekg(index*sizeof(key_t));
    key_t ret;
    ifs.read(ret.data(),ret.size());
    return std::move(ret);
  }

private:
  std::string const _filename;
};

}  // namespace sortbench

#endif
