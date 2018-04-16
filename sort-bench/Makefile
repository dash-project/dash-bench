#CXXFLAGS = -O2 -std=c++11 -I./intel -I. -DENABLE_LOGGING
CXXFLAGS = -O2 -std=c++11 -I./intel -I.

all: tbb-lowlevel.x tbb-highlevel.x cilkplus.x openmp.x gomp.x

run: all
	./tbb-highlevel.x
	./tbb-lowlevel.x
	./cilkplus.x
	./gomp.x
	./openmp.x

tbb-lowlevel.x: ../tbb-lowlevel/parallel_stable_sort.h
	@mkdir -p bin
	icc $(CXXFLAGS) -o $@ -DUSE_TBB_LOWLEVEL sortbench.cc -ltbb

tbb-highlevel.x: ../tbb-highlevel/parallel_stable_sort.h
	icc $(CXXFLAGS) -o bin/$@ -DUSE_TBB_HIGHLEVEL sortbench.cc -ltbb

cilkplus.x: ../cilkplus/parallel_stable_sort.h
	icc $(CXXFLAGS) -o $@ -DUSE_CILKPLUS sortbench.cc -ltbb

openmp.x: ../openmp/parallel_stable_sort.h
	icc $(CXXFLAGS) -o $@ -DUSE_OPENMP -qopenmp sortbench.cc

gomp.x: intel/openmp/parallel_stable_sort.h intel/openmp/sortbench.h
	@mkdir -p bin
	g++ $(CXXFLAGS) -o bin/$@  -DUSE_OPENMP -fopenmp sortbench.cc

*.x: ../test.cpp

clean:
	rm -f *.x
