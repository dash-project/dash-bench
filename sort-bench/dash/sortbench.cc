#include <dash/sortbench.h>
#include <dash/util/BenchmarkParams.h>

void init_runtime(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  dash::util::BenchmarkParams bench_params("bench.dash.sort");
  bench_params.set_output_width(72);
  bench_params.print_header();
  if (dash::size() < 200) {
    bench_params.print_pinning();
  }
}

void fini_runtime()
{
  dash::finalize();
}
