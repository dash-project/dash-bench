#ifndef CAFCORE_INCLUDED
#define CAFCORE_INCLUDED

#include <libdash.h>
#include <dash/Coarray.h>

#include <vector>
#include <string>

class CafCore {
public:
  // all modes for compatibility with fortran benchmark code
  // supported modes are in benchmodes
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

  const std::vector<mode> benchmodes {{
    mode::cafmodeput,
    mode::cafmodemput,
    mode::cafmodeallput,
    mode::cafmodesmput,

    mode::cafmodeget,
    mode::cafmodemget,
    mode::cafmodeallget,
    mode::cafmodesmget
  }};

public:

  /**
   * helper to return benchmodes, as static constexpr variant
   * might lead to linker errors
   */
  decltype(benchmodes) getBenchmodes() const noexcept;

  static std::string cafmodename(const mode & ) noexcept;

  static std::string cafsyncname(const sync & ) noexcept;

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
    using dash::coarray::this_image;
    using dash::coarray::num_images;

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
                  << " not expecting x(0) = "
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
      const std::vector<int> & neighbours);

  static std::vector<int> getneighs(
      const int  me,
      const int  numneigh,
      const sync cafsynctype) noexcept;

  static void getperm(std::vector<int> & perm) noexcept;

  static std::vector<int> getneighspair(int image) noexcept;

  static inline int getpartner(
      const int image,
      const int partner,
      const int numimages) noexcept
  {
    return (image+partner+1) % numimages;
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
      for(int i = icount * stride; i < icount * stride + blksize; ++i){
        //std::cout << " Set at " << i << " value " << value << std::endl;
        if(i < 0 || i >= x.local_size() || x.size() <= 0) {
          std::cerr << "cafset: internal error, i = " << i << std::endl;
          continue;
        }
        x[i] = value;
      }
    }
    x.flush();
  }

};
CafCore cafc();
#endif
