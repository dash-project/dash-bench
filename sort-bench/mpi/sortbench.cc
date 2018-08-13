#include <mpi/sortbench.h>

void init_runtime(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);
}
void fini_runtime()
{
  MPI_Finalize();
}
