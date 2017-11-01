#include <libdash.h>
#include <dash/Coarray.h>

#include <vector>

#include "cafparams.h"
#include "cafcore.h"
#include "cafclock.h"

#ifdef HAVE_MPI
#include <mpi.h>
#endif

using dash::coarray::this_image;
using dash::coarray::num_images;

void cafpingpong(
      int image1,
      int image2,
      const CafCore::mode cafmodetype,
      const CafCore::sync cafsynctype,
      int maxndata,
      bool docheck)
{
  using timepoint   = std::chrono::time_point<std::chrono::high_resolution_clock>;
  using duration_ms = std::chrono::milliseconds;
  using duration_us = std::chrono::microseconds;

  dash::Coarray<double[]> x(num_images());

  timepoint time1, time2, t0, t1;
  dash::Coarray<duration_us> time;

  int count, nrep, blksize, stride, ndata, nextent, trialnrep;
  int i, ierr;

  auto targettime = duration_us(
                      static_cast<long>( // p2ptargettime is in seconds
                        CafParams::p2ptargettime * 1000 * 1000));

  std::vector<int> neighbours(1);

  bool finished, oktime, active, lcheck, gcheck;

#ifdef HAVE_MPI
  MPI_Status mpi_status;
#endif

  gcheck = true;
  lcheck = true;

  trialnrep = 1;

  if(cafsynctype != CafCore::sync::cafsyncall){
    active = false;
  } else {
    active = true;
  }

  if(this_image() == image1){
    active = true;
    neighbours[0] = image2;
  }
  if(this_image() == image2){
    active = true;
    neighbours[0] = image1;
  }

  if (this_image() == 0) {
    std::cout << "----------------------------------"                  << std::endl
              << "image1, image2 = " <<  image1 << "," << image2       << std::endl
              << "benchmarking: " << CafCore::cafmodename(cafmodetype) << std::endl
              << "synchronisation: " <<
                  CafCore::cafsyncname(cafsynctype)<< std::endl
              << "maxndata: " <<  maxndata << std::endl;

    if (docheck) {
      std::cout << "verifying data" << std::endl;
    } else {
      std::cout << "NOT verifying data" << std::endl;
    }
    std::cout << "\n  count  blksize   stride    ndata  nextent     nrep  time[s]  latency[s] bwidth[mb/s]" 
              << std::endl << std::endl;
  }

  // init coarray
  x.allocate(maxndata);
  dash::fill(x.begin(), x.end(), 0);

  switch(cafmodetype){
    case CafCore::mode::cafmodeput:
    case CafCore::mode::cafmodesubput:
    case CafCore::mode::cafmodeget:
    case CafCore::mode::cafmodesubget:
    case CafCore::mode::cafmodeallput:
    case CafCore::mode::cafmodeallget:
    case CafCore::mode::cafmodesimplesubget:
    case CafCore::mode::cafmodesimplesubput:
    case CafCore::mode::cafmodempisend:
      count   = 1;
      blksize = 1;
      stride  = 1;
      break;
    case CafCore::mode::cafmodemput:
    case CafCore::mode::cafmodemsubput:
    case CafCore::mode::cafmodesubmput:
    case CafCore::mode::cafmodemget:
    case CafCore::mode::cafmodemsubget:
    case CafCore::mode::cafmodesubmget:
      count   = 1;
      blksize = maxndata;
      stride  = maxndata;
      break;
    case CafCore::mode::cafmodesmput:
    case CafCore::mode::cafmodesmget:
      count   = 1;
      blksize = maxndata/2;
      stride  = maxndata; 
      break;
    case CafCore::mode::cafmodesput:
    case CafCore::mode::cafmodessubput:
    case CafCore::mode::cafmodesget:
    case CafCore::mode::cafmodessubget:
      // set maximum stride
      stride  = CafParams::p2pmaxstride;
      count   = maxndata / stride;
      blksize = 1;
      break;
    default:
      if(this_image() == 0){
        std::cout << "Invalid mode: " << cafmodetype << std::endl;
      }
      return;
  }

  finished = false;

  while(!finished){
    ndata = count * blksize;
    nextent = blksize + (count-1)*stride;

    oktime = false;

    if (cafmodetype == CafCore::mode::cafmodeallput ||
        cafmodetype == CafCore::mode::cafmodeallget) {
      x.deallocate();
      x.allocate(ndata);
    }

    while(!oktime){
      dash::fill(x.begin(), x.end(), 0);

      if(this_image() == image1){
        // Force initialisation of source array outside of timing loop 
        // even if we do not check values within it

        CafCore::cafset(x, count, stride, blksize, 1.0, true);
      }
      x.sync_all();

      time1 = std::chrono::high_resolution_clock::now();
      
      for(int irep = 1; irep <= trialnrep; ++irep){
        switch(cafmodetype){
          case CafCore::mode::cafmodeput:
          case CafCore::mode::cafmodeallput:
          case CafCore::mode::cafmodemput:
          case CafCore::mode::cafmodesmput:
          case CafCore::mode::cafmodempisend:
            if(this_image() == image1){
              CafCore::cafset(x, count, stride, blksize,
                     static_cast<double>(irep), docheck);
            
              switch(cafmodetype){
                case CafCore::mode::cafmodeput:
                  if(ndata == 1){
                    x(image2)[0] = x[0];
                  } else {
                    dash::copy_async(x.lbegin(), x.lbegin()+ndata, x(image2).begin());
                  }
                  break;
                case CafCore::mode::cafmodeallput:
                  dash::copy_async(x.lbegin(), x.lend(), x(image2).begin());
                  break;
                case CafCore::mode::cafmodemput:
                  for(int i=1; i<=count; ++i){
                    const auto & sbeg = x.lbegin()+(i-1)*blksize;
                    const auto & send = x.lbegin()+i*blksize;
                    dash::copy_async(sbeg, send, x(image2).begin());
                  }
                  break;
                case CafCore::mode::cafmodesmput:
                  for(int i=1; i<=count; ++i){
                    const auto & sbeg = x.lbegin()+2*(i-1)*blksize;
                    const auto & send = x.lbegin()+(2*i-1)*blksize;
                    const auto & tbeg = x(image2).begin()+2*(i-1)*blksize;
                    dash::copy_async(sbeg, send, tbeg);
                  }
                  break;
                case CafCore::mode::cafmodempisend:
#ifdef HAVE_MPI
                  MPI_Send(x.lbegin(), ndata, MPI_DOUBLE, image2, 0,
                           MPI_COMM_WORLD);
#else
                  std::cerr << "cafpingpong: ERROR, MPI not enabled but cafmode = "
                            << cafmodetype << std::endl;
                  return;
#endif
                  break;
                // TODO: other cases
                default:break;
              }
            }
            if(this_image() == image2){
              if(cafmodetype == CafCore::mode::cafmodempisend){
#ifdef HAVE_MPI
                MPI_Recv(x.lbegin(), ndata, MPI_DOUBLE, image1, 0,
                         MPI_COMM_WORLD, &mpi_status);
#else
                std::cerr << "cafpingpong: ERROR, MPI not enabled but cafmode = "
                          << cafmodetype << std::endl;
                return;
#endif
              }
            }
            x.flush();
            CafCore::cafdosync(cafsynctype, active, neighbours);
            
            if(this_image() == image2){
              lcheck = CafCore::cafcheck(x, count, stride, blksize,
                                static_cast<double>(irep), docheck);
    
              CafCore::cafset(x, count, stride, blksize,
                     static_cast<double>(-irep), docheck);

              switch(cafmodetype){
                case CafCore::mode::cafmodeput:
                  dash::copy_async(x.lbegin(), x.lbegin()+ndata, x(image1).begin());
                  break;
                case CafCore::mode::cafmodeallput:
                  dash::copy_async(x.lbegin(), x.lend(), x(image1).begin());
                  break;
                 case CafCore::mode::cafmodemput:
                  for(int i=1; i<=count; ++i){
                    const auto & sbeg = x.lbegin()+(i-1)*blksize;
                    const auto & send = x.lbegin()+i*blksize;
                    dash::copy_async(sbeg, send, x(image1).begin());
                  }
                  break;
                case CafCore::mode::cafmodesmput:
                  for(int i=1; i<=count; ++i){
                    const auto & sbeg = x.lbegin()+2*(i-1)*blksize;
                    const auto & send = x.lbegin()+(2*i-1)*blksize;
                    const auto & tbeg = x(image1).begin()+2*(i-1)*blksize;
                    dash::copy_async(sbeg, send, tbeg);
                  }
                  break;
                case CafCore::mode::cafmodempisend:
#ifdef HAVE_MPI
                  MPI_Send(x.lbegin(), ndata, MPI_DOUBLE, image1, 0,
                           MPI_COMM_WORLD);
#else
                  std::cerr << "cafpingpong: ERROR, MPI not enabled but cafmode = "
                            << cafmodetype << std::endl;
                  return;
#endif
                  break;
                // TODO: other cases
                default:break;
              }
            }
            if(this_image() == image1){
              if(cafmodetype == CafCore::mode::cafmodempisend){
#ifdef HAVE_MPI
                MPI_Recv(x.lbegin(), ndata, MPI_DOUBLE, image2, 0,
                         MPI_COMM_WORLD, &mpi_status);
#else
                std::cerr << "cafpingpong: ERROR, MPI not enabled but cafmode = "
                          << cafmodetype << std::endl;
                return;
#endif
              }
            }

            if(docheck) {
              gcheck &= lcheck;
            }
            if(!gcheck) {
              if(this_image() == 0){
                std::cerr << "ERROR: put ping failed to validate" << std::endl;
                std::cerr << "ERROR: cnt, blk, str, dat, ext, rep = "
                          << count << "," << blksize << "," << stride << ","
                          << ndata << "," << nextent << "," << nrep
                          << std::endl;
              }
              return;
            }
            x.flush();
            CafCore::cafdosync(cafsynctype, active, neighbours);

            if(this_image() == image1){
              lcheck = CafCore::cafcheck(x, count, stride, blksize,
                                static_cast<double>(-irep), docheck);
            }
            if(docheck) {
              gcheck &= lcheck;
            }
            if(!gcheck) {
              if(this_image() == 0){
                std::cerr << "ERROR: put pong failed to validate" << std::endl;
                std::cerr << "ERROR: cnt, blk, str, dat, ext, rep = "
                          << count << "," << blksize << "," << stride << ","
                          << ndata << "," << nextent << "," << nrep
                          << std::endl;
              }
              return;
            }

            break;
          // --------------------------------
          // --------- get cases ------------
          // --------------------------------
          case CafCore::mode::cafmodeget:
          case CafCore::mode::cafmodeallget:
          case CafCore::mode::cafmodemget:
          case CafCore::mode::cafmodesmget:

            if(this_image() == image2){
              switch(cafmodetype){
                case CafCore::mode::cafmodeget:
                  if(ndata == 1){
                    x[0] = x(image1)[0];
                  } else {
                    dash::copy_async(x(image1).begin(),
                               x(image1).begin()+ndata,
                               x.lbegin());
                  }
                  break;
                case CafCore::mode::cafmodeallget:
                  dash::copy_async(x(image1).begin(),
                             x(image1).end(),
                             x.lbegin());
                  break;
                case CafCore::mode::cafmodemget:
                  for(int i=1; i<=count; ++i){
                    const auto & sbeg = x(image1).begin()+(i-1)*blksize;
                    const auto & send = x(image1).begin()+i*blksize;
                    dash::copy_async(sbeg, send, x.lbegin());
                  }
                  break;
                case CafCore::mode::cafmodesmget:
                  for(int i=1; i<=count; ++i){
                    const auto & sbeg = x(image1).begin()+2*(i-1)*blksize;
                    const auto & send = x(image1).begin()+(2*i-1)*blksize;
                    const auto & tbeg = x.lbegin()+2*(i-1)*blksize;
                    dash::copy_async(sbeg, send, tbeg);
                  }
                  break;
                default: break;
              }
              x.flush();
              lcheck = CafCore::cafcheck(x, count, stride, blksize,
                                static_cast<double>(irep), docheck);

              CafCore::cafset(x, count, stride, blksize,
                         static_cast<double>(-irep), docheck);
            }

            if(docheck){
              gcheck &= lcheck;
            }

            if(!gcheck){
              if(this_image() == 0){
                std::cerr << "ERROR: get ping failed to validate" << std::endl;
                std::cerr << "ERROR: cnt, blk, str, dat, ext, rep = "
                          << count << "," << blksize << "," << stride << ","
                          << ndata << "," << nextent << "," << nrep
                          << std::endl;
              }
              return;
            }
            CafCore::cafdosync(cafsynctype, active, neighbours);

            if(this_image() == image1){
              switch(cafmodetype){
                case CafCore::mode::cafmodeget:
                  if(ndata == 1){
                    x[0] = x(image2)[0];
                  } else {
                    dash::copy_async(x(image2).begin(),
                               x(image2).begin()+ndata,
                               x.lbegin());
                  }
                  break;
                case CafCore::mode::cafmodeallget:
                  dash::copy_async(x(image2).begin(),
                             x(image2).end(),
                             x.lbegin());
                  break;
                case CafCore::mode::cafmodemget:
                  for(int i=1; i<=count; ++i){
                    const auto & sbeg = x(image2).begin()+(i-1)*blksize;
                    const auto & send = x(image2).begin()+i*blksize;
                    dash::copy_async(sbeg, send, x.lbegin());
                  }
                  break;
                case CafCore::mode::cafmodesmget:
                  for(int i=1; i<=count; ++i){
                    const auto & sbeg = x(image2).begin()+2*(i-1)*blksize;
                    const auto & send = x(image2).begin()+(2*i-1)*blksize;
                    const auto & tbeg = x.lbegin()+2*(i-1)*blksize;
                    dash::copy_async(sbeg, send, tbeg);
                  }
                  break;
                default: break;
              }
              x.flush();
              lcheck = CafCore::cafcheck(x, count, stride, blksize,
                                static_cast<double>(-irep), docheck);

              CafCore::cafset(x, count, stride, blksize,
                         static_cast<double>(irep+1), docheck);
            }

            if(docheck){
              gcheck &= lcheck;
            }

            if(!gcheck){
              if(this_image() == 0){
                std::cerr << "ERROR: get pong failed to validate" << std::endl;
                std::cerr << "ERROR: cnt, blk, str, dat, ext, rep = "
                          << count << "," << blksize << "," << stride << ","
                          << ndata << "," << nextent << "," << nrep
                          << std::endl;
              }
              return;
            }
            CafCore::cafdosync(cafsynctype, active, neighbours);
            break;
          default: break;
        }
      }

      dash::coarray::sync_all();
 
      time2 = std::chrono::high_resolution_clock::now();
      time  = std::chrono::duration_cast<
                std::chrono::milliseconds>(time2 - time1);
 
      // Broadcast time from image 0
      dash::coarray::cobroadcast(time, dash::team_unit_t{0});
 
      nrep   = trialnrep;
      oktime = cafchecktime(trialnrep,
                 static_cast<duration_us>(time), targettime);

      if(!gcheck){
        // TODO
      }

    }
    if(this_image() == 0){
      if(gcheck) {
        const auto & time_local = static_cast<duration_us>(time);
        const auto time_in_ms   = std::chrono::duration_cast<duration_ms>(
                                    (time_local)).count();
        const double time_in_s  = static_cast<double>(time_in_ms) / 1000.0;

        // transmitted bytes
        const double transmit_B = 2.0 * static_cast<double>(nrep)
                                      * static_cast<double>(ndata)
                                      * static_cast<double>(sizeof(double));
        const double latency_s  = time_in_s / static_cast<double>(2*nrep);

        std::cout << std::setw(7) << count      << ", "
                  << std::setw(7) << blksize    << ", "
                  << std::setw(7) << stride     << ", "
                  << std::setw(7) << ndata      << ", "
                  << std::setw(7) << nextent    << ", "
                  << std::setw(7) << nrep       << ", "
                  << std::setw(7) << time_in_s  << ", "
                  << std::setw(10) << latency_s << ", " 
                  << std::setw(10) << (transmit_B / time_in_ms) / (1024)
                  << std::endl;
      } else {
        std::cout << "Verification failed: exiting this test" << std::endl;
      }
    }
    if(!gcheck) {
      finished = true;
      return;
    }

    switch(cafmodetype){
      case CafCore::mode::cafmodeput:
      case CafCore::mode::cafmodesubput:
      case CafCore::mode::cafmodeget:
      case CafCore::mode::cafmodesubget:
      case CafCore::mode::cafmodeallput:
      case CafCore::mode::cafmodeallget:
      case CafCore::mode::cafmodesimplesubput:
      case CafCore::mode::cafmodesimplesubget:
      case CafCore::mode::cafmodempisend:
        blksize*=2;
        stride*=2;
        if(blksize > maxndata){ finished = true; }
        break;
      case CafCore::mode::cafmodemput:
      case CafCore::mode::cafmodemget:
        count*=2;
        blksize/=2;
        stride/=2;
        if(count > maxndata) { finished = true; }
        break;
      case CafCore::mode::cafmodesmput:
      case CafCore::mode::cafmodesmget:
        count*=2;
        blksize/=2;
        stride/=2;
        if(stride < 2){ finished = true; }
        break;
      case CafCore::mode::cafmodesput:
      case CafCore::mode::cafmodesget:
        stride/=2;
        if(stride < 1){ finished = true; }
        break;
      default:
        if(this_image() == 0){
          std::cerr << "Invalid mode: " << cafmodetype << std::endl;
        };
    }
  }

  x.deallocate();
  if(this_image() == 0){
    std::cout << std::endl;
    if(docheck){
      if(gcheck){
        std::cout << "All results validated\n" << std::endl;
      } else {
        std::cout << "\n" << "ERROR: validation failed\n" << std::endl;
      }
    }
  }
}
