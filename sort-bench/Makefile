ifneq (,$(wildcard ./make.defs.local))
include make.defs.local
endif

SHELL=/bin/bash

# Depending on the platform it may be also mpicxx --> specify as an environment
# parameter
MPICXX?=mpiCC

DASHROOT?=$(HOME)/opt/dash-0.3.0
DASHCXX=$(DASHROOT)/bin/dash-mpic++ -dash:verbose

#CXXFLAGS = -O0 -std=c++11 -ggdb3 -I./external -I. -Wall -DENABLE_LOGGING
CXXFLAGS = -O3 -std=c++11 -I./external -I. -DNDEBUG

SIZE ?= $$((500 * (2**20)))

OS := $(shell uname)

ifeq ($(NPROCS),)
ifeq ($(OS),Linux)
	NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
endif
endif

NPROCS?=4

ENABLE_TRACE?=0

# LIKWID
LIBS+=-L$(LIKWID_LIB) -llikwid
INCS+=-I$(LIKWID_INC)

ifeq ($(ENABLE_LIKWID), 1)
CXXFLAGS+=-DLIKWID_PERFMON
endif

DEPS=util/Math.cc

all: build/tbb-lowlevel.x build/tbb-highlevel.x build/openmp.x build/gomp.x build/dash.x build/mpi.x

run: all
	./build/tbb-highlevel.x $(SIZE) $(NPROCS)
	./build/tbb-lowlevel.x $(SIZE) $(NPROCS)
	./build/gomp.x $(SIZE) $(NPROCS)
	./build/openmp.x $(SIZE) $(NPROCS)
	DASH_ENABLE_TRACE=$(ENABLE_TRACE) mpirun -n $(NPROCS) ./build/dash.x $$(($(SIZE) / $(NPROCS)))
	mpirun -n $(NPROCS) ./build/mpi.x $$(($(SIZE) / $(NPROCS)))

build/tbb-lowlevel.x: tbb/sortbench.h sortbench.cc
	@mkdir -p build
	icpc $(CXXFLAGS) $(INCS) $(LIBS) -L$(TBB_LIB) -ltbb -I$(TBB_INC) -o $@ -DUSE_TBB_LOWLEVEL sortbench.cc tbb/sortbench.cc $(DEPS) -ltbb

build/tbb-highlevel.x: tbb/sortbench.h sortbench.cc
	@mkdir -p build
	icpc $(CXXFLAGS) $(INCS) $(LIBS) -L$(TBB_LIB) -ltbb -I$(TBB_INC) -o $@ -DUSE_TBB_HIGHLEVEL sortbench.cc tbb/sortbench.cc $(DEPS) -ltbb

build/openmp.x: openmp/sortbench.h sortbench.cc
	icpc $(CXXFLAGS) $(INCS) $(LIBS) -o $@ -DUSE_OPENMP -qopenmp sortbench.cc openmp/sortbench.cc $(DEPS)

build/gomp.x: openmp/sortbench.h sortbench.cc
	@mkdir -p build
	g++ $(CXXFLAGS) $(INCS) $(LIBS) -o $@  -DUSE_OPENMP -fopenmp sortbench.cc openmp/sortbench.cc $(DEPS)

build/dash.x: dash/sortbench.h sortbench.cc dash/sortbench.cc
	@mkdir -p build
	$(DASHCXX) $(CXXFLAGS) $(INCS) $(LIBS) -o $@ -DUSE_DASH sortbench.cc dash/sortbench.cc $(DEPS)

build/mpi.x: mpi/sortbench.h sortbench.cc external/MP-sort/libmpsort-mpi.a external/MP-sort/libradixsort.a
	@mkdir -p build
	$(MPICXX) $(CXXFLAGS) -Iexternal $(INCS) $(LIBS) -o $@ -DUSE_MPI sortbench.cc mpi/sortbench.cc $(DEPS) external/MP-sort/libmpsort-mpi.a external/MP-sort/libradixsort.a

external/MP-sort/libmpsort-mpi.a:
	make -C external/MP-sort libmpsort-mpi.a

external/MP-sort/libradixsort.a:
	make -C external/MP-sort libradixsort.a

*.x: sortbench.cc

clean:
	rm -f build/*.x
	make -C external/MP-sort clean
