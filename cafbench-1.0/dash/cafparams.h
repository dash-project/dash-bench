#ifndef CAFPARAMS_INCLUDED
#define CAFPARAMS_INCLUDED

#include <array>

class CafParams {
public:

  // Global parameters
  // -----------------
  // 
  // p2pbench:  whether or not to perform the point-to-point benchmarks
  // syncbench: whether or not to perform the synchronisation benchmarks
  // halobench: whether or not to perform the halo swapping benchmarks

  static const bool p2pbench  = true;
  static const bool syncbench = true;
  static const bool halobench = false;

  // Individual configuration parameters for each of the three benchmarks
  // --------------------------------------------------------------------
  // 
  // 
  // Point-to-point parameters
  // -------------------------
  // 
  // p2pcheck: do we verify the results (impacts performance but should be
  //           done occasionally as compilers can go wrong!)
  // 
  // p2psingle: perform ping-pong between first and last image
  // p2pmulti:  perform ping-pong between all images (in pairs)
  // p2pcross:  as above but half the pairs send as the other half receive
  // 
  // p2pnmax     : maximum array size for all access patterns
  // p2pmaxstride: maximum stride for strided access patterns
  // 
  // p2ptargettime: minimum acceptable time (secs) for a test (code self-adjusts)
  // 

  static const bool p2pcheck    = false;

  static const bool p2psingle   = true;
  static const bool p2pmulti    = false;
  static const bool p2pcross    = false;

  static const int p2pnmax      = 4*1024*1024;
  static const int p2pmaxstride = 128;

  static constexpr double p2ptargettime = 0.5;


  // Synchronisation parameters
  // --------------------------
  //
  // syncouter: number of times each test is run
  // syncinner: number of repetitions for each test
  // syncdelay: sleep for n microseconds 
  //
  // syncmaxneigh: maximum neighbours for each point-to-point pattern
  //

  static const int syncouter =   100;
  static const int syncinner =   200;
  static const int syncdelay =   100;

  static const int syncmaxneigh = 12;


  //  Halo swapping  parameters
  //  -------------------------
  // 
  //  halocheck: do we verify the results (impacts performance but should be
  //             done occasionally as compilers can go wrong!)
  // 
  //  nhalosize: how many different array sizes are tested
  //  halosize:  array sizes (default is cubic although actual code is general)
  // 
  //  halotargettime: minimum acceptable time for a test (the code self-adjusts)
  // 

  static const bool halocheck = false;

  static const int  nhalosize = 4;
  static constexpr std::array<int,4> halosize {{10, 50, 100, 150}};

  static constexpr double halotargettime = 0.5;
};
#endif
