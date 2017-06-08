#include <libdash.h>
#include <dash/Coarray.h>

#include <iostream>

#include "cafcore.h"
#include "cafparams.h"

using dash::coarray::this_image;
using dash::coarray::num_images;

void cafpingpong(
       int image1,
       int image2,
       const CafCore::mode cafmodetype,
       const CafCore::sync cafsynctype,
       int nmax,
       bool docheck);

void cafpt2ptdriver(){
  bool docheck  = CafParams::p2pcheck;

  bool dosingle = CafParams::p2psingle;
  bool domulti  = CafParams::p2pmulti;
  bool docross  = CafParams::p2pcross;

  int image1, image2, tmpimage, nmax;
  int cafsynctype, cafmodetype;

  nmax = CafParams::p2pnmax;

  image1 = 0;
  image2 = 0;

  for(int iloop=0; iloop<2; ++iloop){
    if(iloop == 0 && dosingle){

      image1 = 0;
      image2 = num_images() - 1;

      if (this_image() == 0) {
        std::cout << "--------------------" << std::endl
                  << " Single ping-pong"    << std::endl
                  << "--------------------" << std::endl << std::endl;
      }

    } else if(iloop == 1 && domulti){

      if (num_images() > 2 && (num_images() % 2) == 0) {
  
        if(this_image() <= num_images()/2) {
          image1 = this_image();
          image2 = this_image() + num_images()/2;
        } else {
          image2 = this_image();
          image1 = this_image() - num_images()/2;
        }
  
        if(this_image() == 0){
          std::cout << "--------------------" << std::endl
                    << " Multiple ping-pong"  << std::endl
                    << "--------------------" << std::endl << std::endl;
        }

      } else {
        if(this_image() == 0){
          std::cout << "-----------------------------------------------------\n"
                    << " Cannot do multiple pingpong with num_images() = "
                    << num_images() << std::endl
                    << "-----------------------------------------------------"
                    << std::endl;
        }
  
        domulti = false;

      }

    } else if (iloop == 2 && docross) {

        if ((num_images() % 4) == 0) {

           // First pair them up as before

           if (this_image() <= num_images()/2) {
              image1 = this_image();
              image2 = this_image() + num_images()/2;
           }  else
              image2 = this_image();
              image1 = this_image() - num_images()/2;
           }

           // Assuming an even number of pairs we can swap all the even ones

           if ((this_image() % 2) == 0) {

              tmpimage = image1;
              image1   = image2;
              image2   = tmpimage;

           }

           if (this_image() == 0) {
              std::cout << "-----------------------" << std::endl
                        << "  Crossing ping-pong"    << std::endl
                        << "-----------------------" << std::endl;
           }
           
        } else {

           if (this_image() == 0) {
             std::cout << "-----------------------------------------------------\n"
                       << " Cannot do crossing pingpong with num_images() = "
                       << num_images() << std::endl
                       << "-----------------------------------------------------"
                       << std::endl;
           }
           
           docross = false;
 
        }

    if ( (iloop == 0 && dosingle) ||
         (iloop == 1 && domulti ) ||
         (iloop == 2 && docross)) {
      for (int sloop=0; sloop < 2; ++sloop){
        if (sloop == 0) cafsynctype = CafCore::sync::cafsyncall;
        if (sloop == 2) cafsynctype = CafCore::sync::cafsyncpt2pt;

        for (auto cafmodetype : CafCore().getBenchmodes())
        {
//          if (cafmodetype == CafCore::mode::cafmodempisend) {
//            cafsynctype = CafCore::mode::cafsyncnull;
//          }
          cafpingpong(image1, image2,
              static_cast<CafCore::mode>(cafmodetype),
              static_cast<CafCore::sync>(cafsynctype),
              nmax, docheck);
        }
      }
    }
  }
}

