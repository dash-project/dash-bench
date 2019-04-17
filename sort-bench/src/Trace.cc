#include <algorithm>

#include <util/Trace.h>

#ifdef USE_DASH
#include <dash/util/Trace.h>
#endif

namespace sortbench {

void flush_trace(std::vector<unsigned> const& samples, int myrank)
{
#ifdef USE_DASH
  // if the id of this task is included in samples
  if (std::find(std::begin(samples), std::end(samples), myrank) !=
      std::end(samples)) {
    dash::util::TraceStore::write(std::cout, false);
  }

  dash::util::TraceStore::off();
  dash::util::TraceStore::clear();
#endif
}

void reset_trace()
{
#ifdef USE_DASH
  dash::util::TraceStore::on();
  dash::util::TraceStore::clear();
#endif
}

}  // namespace sortbench
