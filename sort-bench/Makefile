include make.defs.local
SHELL=/bin/bash

# Depending on the platform it may be also mpicxx --> specify as an environment
# parameter
MPICXX?=mpiCC

DASHROOT?=$(HOME)/opt/dash-0.3.0
DASHCXX=$(DASHROOT)/bin/dash-mpic++ -dash:verbose
DASHNVCC=$(DASHROOT)/bin/dash-nvcc -dash:verbose

#CXXFLAGS = -O0 -g -std=c++14 -Iinclude -Iexternal -DENABLE_LOGGING
CXXFLAGS+= -O3 -Iinclude -Iexternal -DNDEBUG -std=c++14

DASHCXXFLAGS = $(CXXFLAGS)

ifeq ($(NPROCS),)
ifeq ($(shell uname),Linux)
	NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
endif
endif

SRC_PATH := src
COMMON_DEPS := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))

SIZE ?= $$((500 * (2**20)))
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

build/tbb-lowlevel.x: $(COMMON_DEPS)
	@mkdir -p build
	icpc $(CXXFLAGS) -o $@ -DUSE_TBB_LOWLEVEL -ltbb $^

build/tbb-highlevel.x: $(COMMON_DEPS)
	@mkdir -p build
	icpc $(CXXFLAGS) -o $@ -DUSE_TBB_HIGHLEVEL -ltbb $^

build/openmp.x:
	icpc $(CXXFLAGS) -o $@ -DUSE_OPENMP -qopenmp $^

build/gomp.x: $(COMMON_DEPS)
	@mkdir -p build
	g++ $(CXXFLAGS) -o $@  -DUSE_OPENMP -fopenmp $^

build/dash.x: $(COMMON_DEPS)
	@mkdir -p build
	$(DASHCXX) $(DASHCXXFLAGS) -o $@ -DUSE_DASH $^

build/dash.nv: $(COMMON_DEPS)
	@mkdir -p build
	$(DASHNVCC) $(DASHCXXFLAGS) -o $@ -DUSE_DASH $^

build/mpi.x: $(COMMON_DEPS) external/MP-sort/libmpsort-mpi.a external/MP-sort/libradixsort.a
	@mkdir -p build
	$(MPICXX) $(CXXFLAGS) -Iexternal -o $@ -DUSE_MPI $^

# NOTE: for USORT we use a k-way Hypercube-Mergesort, they recommend to set CFLAGS="-DKWAY=4"
build/usort.x: $(COMMON_DEPS) external/usort/binUtils.o external/usort/parUtils.o
	@mkdir -p build
	$(MPICXX) $(CXXFLAGS) $(CFLAGS) -Iexternal -o $@ -fopenmp -DUSE_USORT $^

external/MP-sort/libmpsort-mpi.a:
	make -C external/MP-sort libmpsort-mpi.a

external/MP-sort/libradixsort.a:
	make -C external/MP-sort libradixsort.a

external/usort/%.o:
	make -C external/usort CFLAGS="$(CFLAGS)" $*.o

*.x: sortbench.cc

clean:
	rm -f build/*.x
	rm -f build/*.nv
	make -C external/MP-sort clean
	make -C external/usort clean
