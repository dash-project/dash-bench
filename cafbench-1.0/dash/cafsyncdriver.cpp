
#include "cafparams.h"
#include "cafcore.h"
#include "cafsyncdriver.h"
#include "cafsync.h"

#include <libdash.h>

#include <algorithm>
#include <chrono>

using dash::coarray::num_images;

void cafsyncdriver(){
  auto syncouter = CafParams::syncouter;
  auto syncinner = CafParams::syncinner;
  std::chrono::microseconds syncdelay(CafParams::syncdelay);

  auto syncmaxneigh = CafParams::syncmaxneigh;

  CafSync::cafsyncbench(syncouter, syncinner, syncdelay, CafCore::sync::cafsyncall, 1);

  CafSync::cafsyncbench(syncouter, syncinner, syncdelay, CafCore::sync::cafsyncpair, 1);

  int upper_limit = std::min(syncmaxneigh, static_cast<int>(num_images())-2);
  for (int ineigh = 2; ineigh <= upper_limit; ineigh+=2){
    CafSync::cafsyncbench(syncouter, syncinner, syncdelay, CafCore::sync::cafsyncring, ineigh);
    CafSync::cafsyncbench(syncouter, syncinner, syncdelay, CafCore::sync::cafsyncrand, ineigh);
  }

#if 0
  // currently not implemented
  if(num_images() > 9){
    cafsyncbench(syncouter, syncinner, syncdelay, cafsync3d, 6);
  }
#endif

  CafSync::cafsyncbench(syncouter, syncinner, syncdelay, CafCore::sync::cafsynclock, 1);
  CafSync::cafsyncbench(syncouter, syncinner, syncdelay, CafCore::sync::cafsynccrit, 1);
}
