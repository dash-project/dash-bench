#ifndef CAFSYNC_INCLUDED
#define CAFSYNC_INCLUDED

#include "cafcore.h"

#include <chrono>

class CafSync {
public:
  using timepoint     = std::chrono::time_point<std::chrono::high_resolution_clock>;
  using duration      = std::chrono::microseconds;
  using duration_dbl  = std::chrono::duration<double, std::micro>;

private:
  static constexpr double conf95 = 1.96;

  struct stats {
    duration meantime = duration(0);
    double   sd       = 0;
    int      nout     = 0;
  };

public:
  static void cafsyncbench(
      const int outerreps,
      const int innercount,
      const duration dl,
      const CafCore::sync cafsynctype,
      const int numneigh);

  static stats calcstats(
      const std::vector<duration> & time,
      const int  outerreps);

};
#endif
