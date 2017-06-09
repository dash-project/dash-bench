#ifndef CAFSYNC_INCLUDED
#define CAFSYNC_INCLUDED

#include "cafcore.h"
#include "cafclock.h"

#include <chrono>
#include <algorithm>
#include <thread>

#include <dash/Coarray.h>
#include <dash/Comutex.h>

using dash::coarray::this_image;
using dash::coarray::num_images;
using dash::coarray::sync_all;

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
      const int numneigh)
  {
    int innerreps;

    std::vector<int> neighbours;
    bool active = true;
    std::vector<duration> time(outerreps);

    timepoint tstart, tstop;
    duration  totaltime, reftime;
    double refsd;

    int upimage, dnimage, imageshift;

    int i,j,k;

    dash::Mutex caflock;

    if(this_image() == 0){
      std::cout << "cafsync: Synchronisation type: "
                << CafCore::cafsyncname(cafsynctype)
                << std::endl
                << "cafsync: outerreps, innercount, delay = "
                << outerreps << ", " << innercount << ", " << 
                dl.count() << std::endl;
    }

    switch(cafsynctype){
      case CafCore::sync::cafsyncring:
      case CafCore::sync::cafsyncrand:
      case CafCore::sync::cafsync3d:
      case CafCore::sync::cafsyncpair:
        if(numneigh > num_images()){
          std::cerr << "cafsync: numneigh too large!" << std::endl;
          return;
        } else if (this_image() == 0){
          std::cout << "cafsync: Number of neighbours is " << numneigh
                    << std::endl;
        }

        neighbours = std::move(
                       CafCore::getneighs(this_image(),
                                          numneigh,
                                          cafsynctype));
        break;
      case CafCore::sync::cafsyncall:
      case CafCore::sync::cafsynccrit:
      case CafCore::sync::cafsynclock:
        break;
      default:
        if(this_image() == 0){
          std::cerr << "cafsync: Illegal cafsynctype: " << cafsynctype
                    << std::endl;
        }
        return;
    }

    innerreps = innercount;

    if(cafsynctype == CafCore::sync::cafsynclock
       || cafsynctype == CafCore::sync::cafsynccrit){
      // cope with the fact that we scale innerreps by numimages for
      // serialised synchronisation.
      
      innerreps = std::max(2, innercount / static_cast<int>(num_images()));
      innerreps*=num_images();

      if(innerreps != innercount){
        if(this_image() == 0){
          std::cout << "cafsync: rescaling reference innercount to "
                    << innerreps << std::endl;
        }
      }
    }
    dash::coarray::sync_all();

    // Reference time - always discard iteration 0

    for(int k=0; k<=outerreps; ++k){
      tstart = std::chrono::high_resolution_clock::now();

      for(int j=1; j<=innerreps; ++j){
        std::this_thread::sleep_for(dl);
      }

      tstop = std::chrono::high_resolution_clock::now();

      time[k] = std::chrono::duration_cast<duration>(
                  (tstop - tstart) / innerreps);
    }

    totaltime = std::accumulate(time.begin()+1, time.end(), duration(0))*innerreps;
   
    auto res = calcstats(time, outerreps);

    reftime = res.meantime;
    refsd   = res.sd;

    if(this_image() == 0){
      const auto & reftime_us = reftime.count();
      std::cout << "cafsync: reference time is " << reftime_us
                << " +/- " << refsd << "usec" << std::endl;
      std::cout << "cafsync: number of outliers was " << res.nout << std::endl;
    }

    dash::coarray::sync_all();

    // Now do actual sync - always discard iteration 0
    
    for(k=0; k<=outerreps; ++k){
      dash::coarray::sync_all();

      tstart = std::chrono::high_resolution_clock::now();

      switch(cafsynctype){
        case CafCore::sync::cafsyncall:
        case CafCore::sync::cafsyncring:
        case CafCore::sync::cafsyncrand:
        case CafCore::sync::cafsync3d:
        case CafCore::sync::cafsyncpair:
          innerreps = innercount;
          for(int j=1; j<=innerreps; ++j){
            std::this_thread::sleep_for(dl);
            CafCore::cafdosync(cafsynctype, active, neighbours);
          }
          break;
        case CafCore::sync::cafsynccrit:
          innerreps = std::max(2, innercount/static_cast<int>(num_images()));

          for(int j=1; j<=innerreps; ++j){
            std::lock_guard<dash::Mutex> lg(caflock);
            std::this_thread::sleep_for(dl);
          }
          break;
        case CafCore::sync::cafsynclock:
          innerreps = std::max(2, innercount/static_cast<int>(num_images()));
          for(int j=1; j<=innerreps; ++j){
            std::lock_guard<dash::Mutex> lg(caflock);
            std::this_thread::sleep_for(dl);
          }
          break;
        default:
          std::cerr << "cafsync: Illegal cafsynctype: " << cafsynctype
                    << std::endl;
          return;
      }
      sync_all();

      tstop = std::chrono::high_resolution_clock::now();

      time[k] = std::chrono::duration_cast<duration>((tstop - tstart) / innercount);
    }

    totaltime = std::accumulate(time.begin(), time.end(), duration(0)) * innercount;
    
    auto ref = calcstats(time, outerreps);

    if(this_image() == 0){
      auto totaltime_s = std::chrono::duration_cast<std::chrono::seconds>(totaltime);
      std::cout << "cafsync: elapsed   is " << totaltime_s.count() << " seconds" << std::endl
                << "cafsync: loop time is " << ref.meantime.count()  << " +/- " << ref.sd
                << "usec" << std::endl
                << "cafsync: number of outliers was " << ref.nout << std::endl
                << "cafsync: sync time is " << (ref.meantime - reftime).count() 
                << " +/- " << conf95*(ref.sd+refsd) << "usec\n" << std::endl;
    }
  }

  static stats calcstats(
      const std::vector<duration> & time,
      const int  outerreps)
  {
    stats results;

    duration totaltime;
    duration mintime;
    duration maxtime;
    
    double sumsq;

    mintime   = *(std::min_element(time.begin()+1, time.end()));
    maxtime   = *(std::max_element(time.begin()+1, time.end()));
    totaltime = std::accumulate(time.begin()+1, time.end(), duration(0));
    sumsq     = 0.0;

    results.meantime = totaltime / outerreps;
    
    std::for_each(time.begin(), time.end(),
                [&sumsq,&results](const duration & t){
                  auto diff = t - results.meantime;
                  sumsq += static_cast<double>(diff.count() * diff.count()); 
                });

    results.sd = std::sqrt(sumsq/static_cast<double>(outerreps-1));

    auto cutoff = 3.0 * results.sd;

    for(int i=1; i<=outerreps; ++i){
      if(std::abs((time[i]-results.meantime).count()) > cutoff) {
        ++(results.nout);
      }
    }
    
    return results;
  }
};
#endif
