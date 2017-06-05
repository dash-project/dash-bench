#include <libdash.h>
#include <dash/Coarray.h>

#include <vector>

#include "cafcore.h"

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
  dash::Coarray<double[]> x(num_images());

  double time1, time2, time, t0, t1;
  int count, nrep, blksize, stride, ndata, nextent, trialnrep;
  int i, ierr;

  double targettime;
  std::vector<int> neighbours;

  bool finished, oktime, active, lcheck, gcheck;

  gcheck = true;
  lcheck = true;

//targettime = p2ptargettime;
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
    std::cout << "\ncount   blksize stride  ndata   nextent nrep    time      latency   bwidth"
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
      //stride  = p2pmaxstride; // TODO
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

      // time1 = caftime(); // TODO start timer
      
      for(int irep = 0; irep < trialnrep; ++irep){
        switch(cafmodetype){
          case CafCore::mode::cafmodeput: // TODO add other modes
            if(this_image() == image1){
              CafCore::cafset(x, count, stride, blksize,
                     static_cast<double>(irep), docheck);
            
              switch(cafmodetype){
                case CafCore::mode::cafmodeput:
                  dash::copy(x.lbegin(), x.lbegin()+ndata, x(image2).begin());
                  break;
                // TODO: other cases
                default:break;
              }
            }
            CafCore::cafdosync(cafsynctype, active, neighbours);
            
            if(this_image() == image2){
              lcheck = CafCore::cafcheck(x, count, stride, blksize,
                                static_cast<double>(irep), docheck);
    
              CafCore::cafset(x, count, stride, blksize,
                     static_cast<double>(-irep), docheck);

              switch(cafmodetype){
                case CafCore::mode::cafmodeput:
                  dash::copy(x.lbegin(), x.lbegin()+ndata, x(image1).begin());
                  break;
                // TODO: other cases
                default:break;
              }
            }
            if(docheck) {
              gcheck &= lcheck;
            }
            if(!gcheck) {
              // Validation error in ping TODO
            }

            CafCore::cafdosync(cafsynctype, active, neighbours);

            if(this_image() == image1){
              lcheck = CafCore::cafcheck(x, count, stride, blksize,
                                static_cast<double>(-irep), docheck);
            }
            if(docheck) {
              gcheck &= lcheck;
            }
            if(!gcheck) {
              // Validation error in pong TODO
            }

            break;
          // --------------------------------
          // --------- get cases ------------
          // --------------------------------
          case CafCore::mode::cafmodeget:
            // TODO
            break;
          default: break;
        }
      }

      if(this_image() == 0){
        if(gcheck) {
          std::cout << "HEADER"
                    << count << ","
                    << blksize << ","
                    << stride << ","
                    << ndata << ","
                    << nextent << ","
                    << nrep << ","
                    << time << ","
                    << time/static_cast<double>(ndata) << ","
                    << (2.0*static_cast<double>(nrep)*static_cast<double>(sizeof(double)) / (static_cast<double>(1024*1024) * time)) << std::endl;
        } else {
          std::cout << "Verification failed: exiting this test" << std::endl;
        }
      }
      if(!gcheck) {
        finished = true;
        return;
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
}
