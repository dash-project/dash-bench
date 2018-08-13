#include <thread>
#include <tbb/sortbench.h>

void init_runtime(int argc, char* argv[]) {
  auto const NTask =
      (argc == 3) ? atoi(argv[2]) : std::thread::hardware_concurrency();
  assert(NTask > 0);

  tbb::task_scheduler_init init{static_cast<int>(NTask)};
}

void fini_runtime()
{
}
