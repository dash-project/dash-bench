#ifndef UTIL__BENCH_DATA_H__INCLUDED
#define UTIL__BENCH_DATA_H__INCLUDED
#include <cstdlib>
#include <memory>

#ifdef USE_DASH
#include <dash/Array.h>
#endif

namespace detail {

template <class T>
static inline T* allocate_aligned(std::size_t nels)
{
  T* mem = nullptr;

  if (posix_memalign(
          reinterpret_cast<void**>(&mem), alignof(T), sizeof(T) * nels) !=
      0) {
    throw std::bad_alloc();  // or something
  }

  return mem;
}

}  // namespace detail

template <class T>
class BenchData {
#ifdef USE_DASH
  using storage_t         = dash::Array<T>;
  using reference_t       = dash::Array<T>&;
  using const_reference_t = dash::Array<T> const&;
#else
  // Do not use std::vector since it calls the constructor for all elements
  // which, in turn, breaks some NUMA effects
  using storage_t         = std::unique_ptr<T, decltype(std::free)*>;
  using reference_t       = T*;
  using const_reference_t = T const*;
#endif

private:
  std::size_t m_nlocal;
  std::size_t m_nglobal;
  int32_t     m_thisProc;
  std::size_t m_NTask;
  storage_t   m_data;

public:
  BenchData() = delete;

  explicit BenchData(std::size_t nlocal, int32_t ThisTask, size_t NTask)
    : m_nlocal(nlocal)

#if defined(USE_DASH) || defined(USE_MPI)
    , m_nglobal(NTask * m_nlocal)
#else
    , m_nglobal(m_nlocal)
#endif

    , m_thisProc(ThisTask)
    , m_NTask(NTask)

#ifdef USE_DASH
    , m_data(m_nglobal)
#else
    , m_data(detail::allocate_aligned<T>(m_nlocal), std::free)
#endif

  {
  }

  BenchData(const BenchData& other) = delete;

  BenchData(BenchData&& other) = default;

  BenchData& operator=(BenchData const& other) = delete;

  BenchData& operator=(BenchData&& other) = default;

  ~BenchData() = default;

  reference_t data()
  {
    return
#ifdef USE_DASH
        m_data
#else
        m_data.get()
#endif
        ;
  }

  const_reference_t data() const noexcept
  {
    return
#ifdef USE_DASH
        m_data
#else
        m_data.get()
#endif
        ;
  }

  int32_t thisProc() const noexcept
  {
    return m_thisProc;
  }

  size_t nTask() const noexcept
  {
    return m_NTask;
  }

  size_t nglobal() const noexcept
  {
    return m_nglobal;
  }

  size_t nlocal() const noexcept
  {
    return m_nlocal;
  }
};
#endif
