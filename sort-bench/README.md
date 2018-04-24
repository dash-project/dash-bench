# Sort Benchmarks

This is a benchmark suite to compare our `dash::sort` algorithm against various
state of the art implementations on both shared and distributed memory
machines.

## Platform

The platform is [SuperMUC Phase
2](https://www.lrz.de/services/compute/supermuc/systemdescription/) at the
[Leibnitz Supercomputing Center](https://www.lrz.de). A single node consists of
2 CPU sockets, each equipped with 14 cores which are interconnected in two NUMA
domains. Each NUMA domain provides 16 GB's of memory.

This results in 28 cores per node, each with 64 GBytes of memory which is
distributed among 4 NUMA clusters.

## Shared Memory

We compare `dash::sort` on shared memory against a collection of merge sort
implemenatations by
[Intel](https://software.intel.com/en-us/articles/a-parallel-stable-sort-using-c11-for-tbb-cilk-plus-and-openmp),
namely:

- OpenMP task-based merge sort (linked against Intel's openmp library)
- OpenMP task-based merge sort (linked against GNU's gomp library)
- High-Level TBB (merge sort with `tbb::parallel_invoke`)
- Low-Level TBB (merge sort with `tbb:task`)


### Methodology

We measure two variants. The first variant has a fixed size of tasks which are
either 28 or 56 (with hyperthreading) and scales the problem size
starting from 56 MB up to approx. 15 GB. The data to be sorted are 32-bit
signed integers which are initially generated based on a uniform distribution.

The second variant has a fixed size (approx. 2 GB) and scales the number of
cores on a single node. The purpuse of this study is to measure performance
with respect to NUMA effects. We start by occupying only a single NUMA domain
with 7 Tasks (14 with Hyperthreading) which are pinned to the corresponding
cores. Memory allocation is served by the NUMA domain as well.

We continue the experiment by adding another NUMA domain with 14 Tasks (28 with
Hyperthreading) until we finally occupy all 4 NUMA domains.


### Results

- Results for the first variant: [Size
  Scaling](benchmarks/plots/size-scaling.pdf)
- Results for the second variant: [NUMA
  Scaling](benchmarks/plots/numa-scaling.pdf)

## Distributed Memory

TODO...
