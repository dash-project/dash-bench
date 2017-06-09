#ifndef CAFPT2PTDRIVER_INCLUDED
#define CAFPT2PTDRIVER_INCLUDED

#include "cafcore.h"

void cafpingpong(
       int image1,
       int image2,
       const CafCore::mode cafmodetype,
       const CafCore::sync cafsynctype,
       int nmax,
       bool docheck);

void cafpt2ptdriver();

#endif

