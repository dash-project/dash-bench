#include "cafcore.h"

#include <libdash.h>
#include <dash/Coarray.h>

#include <vector>
#include <string>
#include <random>

using dash::coarray::this_image;
using dash::coarray::num_images;



decltype(CafCore::benchmodes) CafCore::getBenchmodes() const noexcept {
  return benchmodes;
}

std::string CafCore::cafmodename(const mode & m) noexcept {
  switch(m){
    case mode::cafmodeput:          return "put";
    case mode::cafmodesubput:       return "subput";
    case mode::cafmodesimplesubput: return "simple subput";
    case mode::cafmodeget:          return "get";
    case mode::cafmodesubget:       return "subget";
    case mode::cafmodesimplesubget: return "simple subget";
    case mode::cafmodemput:         return "many put";
    case mode::cafmodemget:         return "many get";
    case mode::cafmodesubmput:      return "sub manyput";
    case mode::cafmodesubmget:      return "sub manyget";
    case mode::cafmodemsubput:      return "many subput";
    case mode::cafmodemsubget:      return "many subget";
    case mode::cafmodesput:         return "strided put";
    case mode::cafmodesget:         return "strided get";
    case mode::cafmodesmput:        return "strided many put";
    case mode::cafmodesmget:        return "strided many get";
    case mode::cafmodessubput:      return "strided subput";
    case mode::cafmodessubget:      return "strided subget";
    case mode::cafmodeallput:       return "all put";
    case mode::cafmodeallget:       return "all get";
    default: break;
  }
  return "unknown";
}

std::string CafCore::cafsyncname(const sync & s) noexcept {
  switch(s){
    case sync::cafsyncall:   return "cafsyncall";
    case sync::cafsyncpt2pt: return "cafsyncpt2pt";
    case cafsyncring:        return "cafsyncring";
    case cafsyncrand:        return "cafsyncrand";
    case cafsync3d:          return "cafsync3d";
    case cafsyncpair:        return "cafsyncpair";
    case cafsynccrit:        return "cafsynccrit";
    case cafsynclock:        return "cafsynclock";
    case cafsyncnull:        return "cafsyncnull";
    default: break;

  }
  return "unknown";
}

void CafCore::cafdosync(
    const sync & cafsynctype,
    const bool active,
    const std::vector<int> & neighbours) {
  switch(cafsynctype){
    case sync::cafsyncall:
      if(!active){
        std::cerr << "image " << this_image()
                  << ":invalid active = " << active << std::endl;
        return;
      } else {
        dash::coarray::sync_all();
      }
      break;
#if 0
    case sync::cafsyncmpi:
      if(!active){
        std::cerr << "image " << this_image()
                  << ":invalid active = " << active << std::endl;
        return;
      } else {
        MPI_Barrier(MPI_COMM_WORLD);
      }
      break;
#endif
    case cafsyncpt2pt:
    case cafsyncring:
    case cafsyncrand:
    case cafsync3d:
    case cafsyncpair:
      dash::coarray::sync_images(neighbours);
      break;
    default: break;
  }
}

std::vector<int> CafCore::getneighs(
    const int  me,
    const int  numneigh,
    const sync cafsynctype) noexcept
{
  int n = numneigh;
  int j;
  int tmpneigh = 0;
  std::vector<int> neighs(n);
  std::vector<int> perm(num_images());


  if(n % 2 != 0 && cafsynctype != cafsyncpair){
    std::cerr << "getneighs: illegal number = " << n << std::endl;
    return std::vector<int>();
  }

  switch(cafsynctype){
    case cafsyncpair:
      neighs = getneighspair(this_image());
      break;
    case cafsyncrand:
    case cafsyncring:
      if(cafsynctype == cafsyncrand){
        getperm(perm);
      } else if(cafsynctype == cafsyncring) {
        std::iota(perm.begin(), perm.end(), 0);
      }
      for(int i=0; i<num_images(); ++i){
        for(int partner = 0; partner < n/2; ++partner){
          j = getpartner(i, partner, num_images());
          if(perm[i] == me){
            neighs[tmpneigh] = perm[j];
            ++tmpneigh;
          }

          if(perm[j] == me){
            neighs[tmpneigh] = perm[i];
            ++tmpneigh;
          }
        }
      }
      if (tmpneigh != n){
        std::cerr << "getneighs: internal error!" << std::endl;
      }
      break;
    default:
      std::cerr << "Illegal cafsynctype: " << cafsynctype << std::endl;
  }
#ifdef DEBUG
  std::this_thread::sleep_for(std::chrono::milliseconds(10*this_image()));
  std::cout << "Image " << this_image() << ":";
    for(auto & el : neighs){
      std::cout << el << ",";
    }
    std::cout << std::endl;
#endif
  return neighs;
}

void CafCore::getperm(std::vector<int> & perm) noexcept {
  std::random_device rd;
  std::mt19937 g(rd());

  std::iota(perm.begin(), perm.end(), 0);
  std::shuffle(perm.begin(), perm.end(), g);

}

std::vector<int> CafCore::getneighspair(int image) noexcept {
  if(image <= num_images()/2){
    return std::vector<int> {image + static_cast<int>(num_images())/2 -1};
  } else {
    return std::vector<int> {image - static_cast<int>(num_images())/2 -1};
  }
}

CafCore cafc();

