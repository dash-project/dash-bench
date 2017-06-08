
#include <chrono>
#include <algorithm>

bool cafchecktime(
    int & nrep,
    const std::chrono::microseconds & time,
    const std::chrono::microseconds & targettime)
{
  using cus = std::chrono::microseconds;

  cus tmax, tmin;

  tmax = targettime * (cus(4) / cus(3));
  tmin = targettime * (cus(2) / cus(3));

  if(time > tmax){
    nrep = std::max(nrep/2, 1);
  } else {
    nrep*=2;
    return false;
  }
  return true;
}
