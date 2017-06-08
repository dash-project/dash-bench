/**
Dummy C++ app as a startpoint for writing dash applications
*/
#include <iostream>

#include <libdash.h>
#include <dash/Coarray.h>

#include <chrono>
#include <thread>

#include "cafparams.h"

// forward decls
void cafpt2ptdriver();


using dash::coarray::this_image;
using dash::coarray::num_images;

int main(int argc, char** argv){

  bool dopt2pt = CafParams::p2pbench;
  bool dosync  = CafParams::syncbench;
  bool dohalo  = CafParams::halobench;

  // Initialize DASH
  dash::init(&argc, &argv);

  bool test = false;
  while(test){
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

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
