#include <util/Math.h>
#include <cstdint>

uint32_t nextPowerTwo(uint32_t n)
{
  --n;

  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;

  return n + 1;
}
