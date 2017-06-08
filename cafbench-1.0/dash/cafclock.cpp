
#include <iostream>
#include <chrono>
#include <algorithm>

bool cafchecktime(
    int & nrep,
    const std::chrono::microseconds & time,
    const std::chrono::microseconds & targettime)
{
  // avoid integer division of durations
  using cus_dbl = std::chrono::duration<double, std::micro>;

  cus_dbl tmax, tmin;

  tmax = targettime * (cus_dbl(4.0) / cus_dbl(3.0));
  tmin = targettime * (cus_dbl(2.0) / cus_dbl(3.0));

//  std::cout << "tmax " << std::chrono::duration_cast<std::chrono::microseconds>(tmax).count() << std::endl;
//  std::cout << "tmin" << std::chrono::duration_cast<std::chrono::microseconds>(tmin).count() << std::endl;

  if(time > tmax){
    nrep = std::max(nrep/2, 1);
  } else {
    nrep*=2;
    return false;
  }
  return true;
}
