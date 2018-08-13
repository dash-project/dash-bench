#include <dash/Array.h>
#include <cassert>
#include <cstdlib>

template <class T>
class BenchParams {
private:
  std::size_t    m_nlocal;
  std::size_t    m_nglobal;
  int32_t        m_NTask;
  dash::Array<T> m_data;

public:
  BenchParams() = delete;

  explicit BenchParams(std::size_t nlocal)
    : m_nlocal(nlocal)
    , m_nglobal(dash::size() * m_nlocal)
    , m_NTask(dash::size())
    , m_data(m_nglobal)
  {
    assert(m_data.size() == m_nglobal);
  }

  BenchParams(const BenchParams& other) = delete;

  BenchParams(BenchParams&& other) = default;

  BenchParams& operator=(BenchParams const& other) = delete;

  BenchParams& operator=(BenchParams&& other) = default;

  ~BenchParams() = default;

  dash::Array<T>& data()
  {
    return m_data;
  }

  int32_t ntask() const noexcept
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
