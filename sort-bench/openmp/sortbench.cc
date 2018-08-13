#include <openmp/sortbench.h>
#include <thread>

void init_runtime(int argc, char* argv[]) {

  auto const NTask =
      (argc == 3) ? atoi(argv[2]) : std::thread::hardware_concurrency();
  assert(NTask > 0);
  omp_set_num_threads(NTask);
}
void fini_runtime()
{
}
