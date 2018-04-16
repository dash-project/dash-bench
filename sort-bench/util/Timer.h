#ifndef TIMERS_H
#define TIMERS_H

#include <chrono>

template <bool HighResIsSteady = std::chrono::high_resolution_clock::is_steady>
struct ChooseSteadyClock {
  typedef std::chrono::high_resolution_clock type;
};

template <>
struct ChooseSteadyClock<false> {
  typedef std::chrono::steady_clock type;
};

struct ChooseClockType {
  typedef ChooseSteadyClock<>::type type;
};

inline double ChronoClockNow() {
  typedef ChooseClockType::type ClockType;
  using duration_t = std::chrono::duration<double, std::chrono::seconds::period>;
  return duration_t(ClockType::now().time_since_epoch()).count();
}

#endif
