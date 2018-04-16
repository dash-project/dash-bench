SHELL=/bin/bash
#CXXFLAGS = -O3 -std=c++11 -I./intel -I. -DENABLE_LOGGING
CXXFLAGS = -O3 -std=c++11 -I./intel -I.
GB = $$((100 * 1024 * 1024))

all: bin/tbb-lowlevel.x bin/tbb-highlevel.x bin/openmp.x bin/gomp.x

run: all
	./bin/tbb-highlevel.x $(GB)
	./bin/tbb-lowlevel.x $(GB)
	./bin/gomp.x $(GB)
	./bin/openmp.x $(GB)

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

*.x: sortbench.cc

clean:
	rm -f bin/*.x
