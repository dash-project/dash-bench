SHELL=/bin/bash

# Depending on the platform it may be also mpicxx --> specify as an environment
# parameter
MPICXX?=mpiCC

DASHROOT?=$(HOME)/opt/dash-0.3.0
DASHCXX=$(DASHROOT)/bin/dash-mpic++ -dash:verbose
DASHNVCC=$(DASHROOT)/bin/dash-nvcc -dash:verbose

DASHCXXFLAGS = -O3 -Iinclude -Iexternal -DNDEBUG
CXXFLAGS = $(DASHCXXFLAGS) -std=c++14

SIZE ?= $$((500 * (2**20)))

OS := $(shell uname)

ifeq ($(NPROCS),)
ifeq ($(OS),Linux)
	NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
endif
endif

NPROCS?=4

ENABLE_TRACE?=0


all: build/tbb-lowlevel.x build/tbb-highlevel.x build/openmp.x build/gomp.x build/dash.x build/mpi.x

run: all
	./build/tbb-highlevel.x $(SIZE) $(NPROCS)
	./build/tbb-lowlevel.x $(SIZE) $(NPROCS)
	./build/gomp.x $(SIZE) $(NPROCS)
	./build/openmp.x $(SIZE) $(NPROCS)
	DASH_ENABLE_TRACE=$(ENABLE_TRACE) mpirun -n $(NPROCS) ./build/dash.x $$(($(SIZE) / $(NPROCS)))
	mpirun -n $(NPROCS) ./build/mpi.x $$(($(SIZE) / $(NPROCS)))

build/tbb-lowlevel.x: include/tbb/sortbench.h sortbench.cc
	@mkdir -p build
	icpc $(CXXFLAGS) -o $@ -DUSE_TBB_LOWLEVEL sortbench.cc -ltbb

build/tbb-highlevel.x: include/tbb/sortbench.h sortbench.cc
	@mkdir -p build
	icpc $(CXXFLAGS) -o $@ -DUSE_TBB_HIGHLEVEL sortbench.cc -ltbb

build/openmp.x: include/openmp/sortbench.h sortbench.cc
	icpc $(CXXFLAGS) -o $@ -DUSE_OPENMP -qopenmp sortbench.cc

build/gomp.x: include/openmp/sortbench.h sortbench.cc
	@mkdir -p build
	g++ $(CXXFLAGS) -o $@  -DUSE_OPENMP -fopenmp sortbench.cc

build/dash.x: include/dash/sortbench.h sortbench.cc
	@mkdir -p build
	$(DASHCXX) $(DASHCXXFLAGS) -o $@ -DUSE_DASH sortbench.cc

build/dash.nv: include/dash/sortbench.h sortbench.cc
	@mkdir -p build
	$(DASHNVCC) $(DASHCXXFLAGS) -o $@ -DUSE_DASH sortbench.cu

build/mpi.x: include/mpi/sortbench.h sortbench.cc external/MP-sort/libmpsort-mpi.a external/MP-sort/libradixsort.a
	@mkdir -p build
	$(MPICXX) $(CXXFLAGS) -Iexternal -o $@ -DUSE_MPI sortbench.cc external/MP-sort/libmpsort-mpi.a external/MP-sort/libradixsort.a

external/MP-sort/libmpsort-mpi.a:
	make -C external/MP-sort libmpsort-mpi.a

external/MP-sort/libradixsort.a:
	make -C external/MP-sort libradixsort.a

*.x: sortbench.cc

clean:
	rm -f build/*.x
	rm -f build/*.nv
	make -C external/MP-sort clean
