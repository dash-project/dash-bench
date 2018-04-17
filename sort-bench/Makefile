SHELL=/bin/bash
#CXXFLAGS = -O3 -std=c++11 -I./intel -I. -DENABLE_LOGGING
CXXFLAGS = -O3 -std=c++11 -I./intel -I. -DNDEBUG

SIZE ?= $$((500 * (2**20)))

OS := $(shell uname)

ifeq ($(NPROCS),)
ifeq ($(OS),Linux)
	NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
endif
endif

NPROCS?=4

DASHROOT?=$(HOME)/opt/dash-0.3.0
DASHCXX=$(DASHROOT)/bin/dash-mpic++ -dash:verbose

all: bin/tbb-lowlevel.x bin/tbb-highlevel.x bin/openmp.x bin/gomp.x bin/dash.x

run: all
	./bin/tbb-highlevel.x $(SIZE) $(NPROCS)
	./bin/tbb-lowlevel.x $(SIZE) $(NPROCS)
	./bin/gomp.x $(SIZE) $(NPROCS)
	./bin/openmp.x $(SIZE) $(NPROCS)
	mpirun -n $(NPROCS) ./bin/dash.x $$(($(SIZE) / $(NPROCS)))

bin/tbb-lowlevel.x: intel/tbb-lowlevel/parallel_stable_sort.h intel/tbb-lowlevel/sortbench.h sortbench.cc
	@mkdir -p bin
	icpc $(CXXFLAGS) -o $@ -DUSE_TBB_LOWLEVEL sortbench.cc -ltbb

bin/tbb-highlevel.x: intel/tbb-highlevel/parallel_stable_sort.h intel/tbb-highlevel/sortbench.h sortbench.cc
	@mkdir -p bin
	icpc $(CXXFLAGS) -o $@ -DUSE_TBB_HIGHLEVEL sortbench.cc -ltbb

bin/openmp.x: intel/openmp/parallel_stable_sort.h intel/openmp/sortbench.h sortbench.cc
	icpc $(CXXFLAGS) -o $@ -DUSE_OPENMP -qopenmp sortbench.cc

bin/gomp.x: intel/openmp/parallel_stable_sort.h intel/openmp/sortbench.h sortbench.cc
	@mkdir -p bin
	g++ $(CXXFLAGS) -o $@  -DUSE_OPENMP -fopenmp sortbench.cc

bin/dash.x: dash/sortbench.h sortbench.cc
	@mkdir -p bin
	$(DASHCXX) $(CXXFLAGS) -o $@ -DUSE_DASH sortbench.cc

*.x: sortbench.cc

clean:
	rm -f bin/*.x
