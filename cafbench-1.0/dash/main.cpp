/**
DASH version of the Fortran Coarray MicroBenchmark Suite - Version 1.0

Measures various metrics of the dash::Coarray implementation
*/
#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include <iostream>

#include <libdash.h>
#include <dash/Coarray.h>

#include <chrono>
#include <thread>

#include "cafparams.h"
#include "cafpt2ptdriver.h"
#include "cafsyncdriver.h"


using dash::coarray::this_image;
using dash::coarray::num_images;

int main(int argc, char** argv){

  bool dopt2pt = CafParams::p2pbench;
  bool dosync  = CafParams::syncbench;
  bool dohalo  = CafParams::halobench;

#ifdef HAVE_MPI
  MPI_Init(&argc, &argv);
#endif

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
#ifdef HAVE_MPI
              << "MPI Available"      << std::endl
#endif
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

  if(dosync){
    if(this_image() == 0){
      std::cout << "---------------" << std::endl
                << "Synchronisation" << std::endl
                << "---------------" << std::endl << std::endl;
    }
    cafsyncdriver();
  }
  
  // quit dash
  dash::finalize();

#ifdef HAVE_MPI
  MPI_Finalize();
#endif
}
