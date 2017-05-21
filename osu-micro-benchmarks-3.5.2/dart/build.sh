#!/bin/bash

DART_ROOT=$HOME/opt/dash-0.3.0/
DART_ROOT_NOSHW=$HOME/opt/dash-0.3.0-noshw/


OMPI_CC=$CC
OMPI_CXX=$CXX

mpiCC -std=c++11 -I$DART_ROOT/include/ osu_dart_put_latency.cc osu_dart_common.cc -o osu_dart_put_latency -L$DART_ROOT/lib -ldash-mpi -ldart-mpi -ldart-base -lnuma $HWLOC_SHLIB $PAPI_SHLIB
mpiCC -std=c++11 -I$DART_ROOT/include/ osu_dart_get_latency.cc osu_dart_common.cc -o osu_dart_get_latency -L$DART_ROOT/lib -ldash-mpi -ldart-mpi -ldart-base -lnuma $HWLOC_SHLIB $PAPI_SHLIB

mpiCC -std=c++11 -I$DART_ROOT_NOSHW/include/ osu_dart_put_latency.cc osu_dart_common.cc -o osu_dart_put_latency_noshw -L$DART_ROOT_NOSHW/lib -ldash-mpi -ldart-mpi -ldart-base -lnuma $HWLOC_SHLIB $PAPI_SHLIB
mpiCC -std=c++11 -I$DART_ROOT_NOSHW/include/ osu_dart_get_latency.cc osu_dart_common.cc -o osu_dart_get_latency_noshw -L$DART_ROOT_NOSHW/lib -ldash-mpi -ldart-mpi -ldart-base -lnuma $HWLOC_SHLIB $PAPI_SHLIB
