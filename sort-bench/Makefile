SHELL=/bin/bash

# Depending on the platform it may be also mpicxx --> specify as an environment
# parameter
MPICXX?=mpiCC

DASHROOT?=$(HOME)/opt/dash-0.3.0
#DASHCXX=$(DASHROOT)/bin/dash-mpic++ -dash:verbose -L/opt/intel.2018/compilers_and_libraries_2018.0.128/linux/mpi/intel64/lib/release_mt -lmpi
DASHCXX=$(DASHROOT)/bin/dash-mpic++ -dash:verbose

#CXXFLAGS = -O3 -std=c++11 -I./external -I. -Wall -DENABLE_LOGGING
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


all: bin/tbb-lowlevel.x bin/tbb-highlevel.x bin/openmp.x bin/gomp.x bin/dash.x

run: all
	./bin/tbb-highlevel.x $(SIZE) $(NPROCS)
	./bin/tbb-lowlevel.x $(SIZE) $(NPROCS)
	./bin/gomp.x $(SIZE) $(NPROCS)
	./bin/openmp.x $(SIZE) $(NPROCS)
	DASH_ENABLE_TRACE=$(ENABLE_TRACE) mpirun -n $(NPROCS) ./bin/dash.x $$(($(SIZE) / $(NPROCS)))
	#mpirun -n $(NPROCS) ./bin/mpi.x $$(($(SIZE) / $(NPROCS)))

bin/tbb-lowlevel.x: tbb/sortbench.h sortbench.cc
	@mkdir -p bin
	icpc $(CXXFLAGS) -o $@ -DUSE_TBB_LOWLEVEL sortbench.cc -ltbb

bin/tbb-highlevel.x: tbb/sortbench.h sortbench.cc
	@mkdir -p bin
	icpc $(CXXFLAGS) -o $@ -DUSE_TBB_HIGHLEVEL sortbench.cc -ltbb

bin/openmp.x: openmp/sortbench.h sortbench.cc
	icpc $(CXXFLAGS) -o $@ -DUSE_OPENMP -qopenmp sortbench.cc

bin/gomp.x: openmp/sortbench.h sortbench.cc
	@mkdir -p bin
	g++ $(CXXFLAGS) -o $@  -DUSE_OPENMP -fopenmp sortbench.cc

bin/dash.x: dash/sortbench.h sortbench.cc
	@mkdir -p bin
	$(DASHCXX) $(CXXFLAGS) -o $@ -DUSE_DASH sortbench.cc

bin/mpi.x: mpi/sortbench.h sortbench.cc external/MP-sort/libmpsort-mpi.a external/MP-sort/libradixsort.a
	@mkdir -p bin
	$(MPICXX) $(CXXFLAGS) -Iexternal -o $@ -DUSE_MPI sortbench.cc external/MP-sort/libmpsort-mpi.a external/MP-sort/libradixsort.a

external/MP-sort/libmpsort-mpi.a:
	make -C external/MP-sort libmpsort-mpi.a

external/MP-sort/libradixsort.a:
	make -C external/MP-sort libradixsort.a

*.x: sortbench.cc

clean:
	rm -f bin/*.x
	make -C external/MP-sort clean
