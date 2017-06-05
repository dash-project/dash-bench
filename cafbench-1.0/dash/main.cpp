/**
Dummy C++ app as a startpoint for writing dash applications
*/
#include <iostream>

#include <libdash.h>
#include <dash/Coarray.h>

// forward decls
void cafpt2ptdriver();


using dash::coarray::this_image;
using dash::coarray::num_images;

int main(int argc, char** argv){

  bool dopt2pt = true;
  bool dosync  = false;
  bool dohalo  = false;

  // Initialize DASH
  dash::init(&argc, &argv);

  if(this_image() == 0){
    std::cout << "------------------" << std::endl
              << "CAF Benchmark 1.0 - DASH Version" << std::endl
              << std::endl
              << "numimages = " << num_images()    << std::endl
              << "------------------" << std::endl << std::endl;
  }

  if(dopt2pt){
    if(this_image() == 0){
      std::cout << "------------------" << std::endl
                << "Point-to-point"     << std::endl
                << "------------------" << std::endl << std::endl;
    }
    cafpt2ptdriver();
  }
  
  // quit dash
  dash::finalize();
}
