#ifndef UTIL__BENCH_DATA_H__INCLUDED
#define UTIL__BENCH_DATA_H__INCLUDED
#include <cstdlib>
#include <cstring>

#include <memory>
#ifdef ENABLE_MEMKIND
#include <hbwmalloc.h>
#endif

#ifdef USE_DASH
#include <dash/Array.h>
#endif

#include <util/Logging.h>
#include <util/Math.h>

namespace detail {

template <class T>
T* allocate_aligned(std::size_t nels)
{
  void* mem = nullptr;

  auto alignment = std::max(sizeof(void*), alignof(T));

  alignment = ::nextPowerTwo(alignment);

  int error;

  LOG("allocating memory: " << sizeof(T) * nels << " bytes");

  if ((error = posix_memalign(&mem, alignment, sizeof(T) * nels)) != 0) {
    LOG("posix_memalign failed (" << strerror(error)
                                  << ") --> falling back to malloc");
    mem = std::malloc(sizeof(T) * nels);

    if (mem == nullptr) {
      LOG("malloc failed for" << nels * sizeof(T) << " bytes");
      throw std::bad_alloc();  // or something
    }
  }

  return reinterpret_cast<T*>(mem);
}

#ifdef ENABLE_MEMKIND
template <class T>
T * allocate_aligned_hbw(std::size_t nels) {
  if (nels == 0) return nullptr;


  if (::hbw_check_available()) {
    LOG("allocating from HBM node");
    T *ptr;

    auto alignment = std::max(alignof(T), sizeof(void*));

    auto const is_power_of_two = (alignment & (alignment - 1)) == 0;

    if (!is_power_of_two) {
      alignment = ::nextPowerTwo(alignment);
    }

    int ret = ::hbw_posix_memalign(reinterpret_cast<void**>(&ptr), alignment, sizeof(T) * nels);

    if (ret == ENOMEM) {
      throw std::bad_alloc();
    }
    else if (ret == EINVAL) {
      throw std::invalid_argument(
          "Invalid requirements for hbw_posix_memalign");
    }
    return ptr;
  }
  else {
    LOG("falling back to malloc...");
    return allocate_aligned<T>(nels);
  }
}
#endif

void deallocate(void * p);

}  // namespace detail

template <class T>
class BenchData {
#ifdef USE_DASH
  using index_t           = dash::default_index_t;
  using pattern_t         = dash::BlockPattern<1, dash::ROW_MAJOR, index_t>;
#ifdef ENABLE_MEMKIND
  using memory_space_t    = dash::HBWSpace;
  using storage_t         = dash::Array<T, index_t, pattern_t, memory_space_t>;
#else
  using storage_t         = dash::Array<T, index_t, pattern_t>;
#endif

  using reference_t       = storage_t &;
  using const_reference_t = storage_t const&;
#else
  // Do not use std::vector since it calls the constructor for all elements
  // which, in turn, breaks some NUMA effects
  using storage_t         = std::unique_ptr<T, decltype(detail::deallocate)*>;
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
#elif defined(ENABLE_MEMKIND)
    , m_data(detail::allocate_aligned_hbw<T>(m_nlocal), detail::deallocate)
#else
    , m_data(detail::allocate_aligned<T>(m_nlocal), detail::deallocate)
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
