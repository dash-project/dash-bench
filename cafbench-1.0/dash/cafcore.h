#ifndef CAFCORE_INCLUDED
#define CAFCORE_INCLUDED

#include <libdash.h>
#include <dash/Coarray.h>

#include <array>
#include <string>

using dash::coarray::this_image;

class CafCore {
public:
  enum mode : unsigned char {
    cafmodeput          =  1,
    cafmodesubput       =  2,
    cafmodemput         =  3,
    cafmodesubmput      =  4,
    cafmodemsubput      =  5,
    cafmodesput         =  6,
    cafmodessubput      =  7,
    cafmodeallput       =  8,
    cafmodesimplesubput =  9,
    cafmodesmput        = 10,

    cafmodeget          = 11,
    cafmodesubget       = 12,
    cafmodemget         = 13,
    cafmodesubmget      = 14,
    cafmodemsubget      = 15,
    cafmodesget         = 16,
    cafmodessubget      = 17,
    cafmodeallget       = 18,
    cafmodesimplesubget = 19,
    cafmodesmget        = 20,
//    cafmodempisend      = 21,

    maxcafmode          = 21
  };

  enum sync : unsigned char {
    cafsyncall   =  1,
    cafsyncpt2pt =  2,
    cafsyncring  =  3,
    cafsyncrand  =  4,
    cafsync3d    =  5,
    cafsyncpair  =  6,
    cafsynccrit  =  7,
    cafsynclock  =  8,
    cafsyncnull  =  9,

    maxcafsync   = 10
  };
//  tstart // todo: timer

public:
  static std::string cafmodename(const mode & m) {
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

  static std::string cafsyncname(const sync & s){
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

  template<
    typename CO_A>
  static bool cafcheck(
      const CO_A & x,
      const int count,
      const int stride,
      const int blksize,
      const typename CO_A::value_type value,
      const bool docheck)
  {
    using value_type = typename CO_A::value_type;

    if(docheck){
      for(int icount = 0; icount < count; ++icount){
        for(int i=icount*stride; i<icount*stride+blksize;++i){
          if(x[i] != value){
            std::cout << "ERROR: image " << this_image()
                      << ", x(" << i << ") = " << x[i] << std::endl;
            std::cout << "ERROR: image " << this_image()
                      << ", expected x = " << value << std::endl;
            return false; 
          } 
        }
      }
    } else {
      if(x[0] == 0){
        std::cout << "ERROR: image " << this_image()
                  << " not expecting x(1) = "
                  << x[0]
                  << std::endl;
        return false;
      }
    }
    return true;
  }

  static void cafdosync(
      const sync & cafsynctype,
      const bool active,
      const std::vector<int> & neighbours){
    switch(cafsynctype){
      case sync::cafsyncall: dash::coarray::sync_all(); break;
      default: break;
    }
  }

  template<
    typename CO_A,
    typename CO_B>
  static inline void cafput(
      CO_A & target,
      const CO_A & source,
      const int disp,
      const int count,
      const int image)
  {
    if(count == 1){
      target(image)[disp] = source[disp];
    } else {
      auto & begit = source.lbegin()+disp;
      auto & endit = begit+count;
      dash::copy(begit, endit, target(image).begin());
    }
  }

  template<
    typename CO_A,
    typename CO_B>
  static inline void cafget(
      CO_A & target,
      const CO_A & source,
      const int disp,
      const int count,
      const int image)
  {
    if(count == 1){
      target[disp] = source(image)[disp];
    } else {
      auto & begit = source(image).begin()+disp;
      auto & endit = begit+count;
      dash::copy(begit, endit, target.lbegin());
    }
  }

  template<
    typename CO_A,
    typename CO_B>
  static inline void cafsimpleput(
      CO_A & target,
      const CO_A & source,
      const int n,
      const int image)
  {
    auto & begit = source.lbegin();
    auto & endit = source.lend();
    dash::copy(begit, endit, target(image).begin());
  }

  template<
    typename CO_A,
    typename CO_B>
  static inline void cafsimpleget(
      CO_A & target,
      const CO_A & source,
      const int n,
      const int image)
  {
    auto & begit = source(image).begin();
    auto & endit = source(image).end();
    dash::copy(begit, endit, target.lbegin());
  }

  template<
    typename CO_A>
  static void cafset(
      CO_A & x,
      const int count,
      const int stride,
      const int blksize,
      const typename CO_A::value_type value,
      bool docheck)
  {
    using value_type = typename CO_A::value_type;

    dash::fill(x.begin(), x.end(), static_cast<value_type>(0));

    for(int icount = 0; icount < count; ++icount){
      for(int i = icount * stride; icount * stride + blksize; ++i){
        x[i] = value;
      }
    }
  }

};
#endif
