#define BENCHMARK "OSU DART_Get%s Latency Test"
/*
 * Copyright (C) 2003-2016 the Network-Based Comgeting Laboratory
 * (NBCL), The Ohio State University.
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include "osu_dart_common.h"

#define MAX_SIZE (1<<24) // 16MB
#define MYBUFSIZE (MAX_SIZE + MAX_ALIGNMENT)

#define SKIP_LARGE  10
#define LOOP_LARGE  100
#define LARGE_MESSAGE_SIZE  8192 //8kB

#ifdef PACKAGE_VERSION
#   define HEADER "# " BENCHMARK " v" PACKAGE_VERSION "\n"
#else
#   define HEADER "# " BENCHMARK "\n"
#endif

static double  t_end, t_start;
char    * sbuf_original = nullptr;
char    * rbuf_original = nullptr;
char    *sbuf=nullptr, *rbuf=nullptr;

void print_header (int, int, WINDOW, SYNC);
void print_latency (int, int);
void run_get_with_flush (int, int, WINDOW);
void run_get_with_flush_local (int, int, WINDOW);
void run_get_blocking (int, int, WINDOW);
void run_get_with_handle (int, int, WINDOW);


int main (int argc, char *argv[])
{
    size_t         nprocs;
    dart_global_unit_t    myid;
    static dart_unit_t rank;

    int         po_ret = po_okay;
    WINDOW      win_type=DART_ALLOCATE;
    SYNC        sync_type=FLUSH;

    po_ret = process_options(argc, argv, &win_type, &sync_type);

    dash::init(&argc, &argv);
    DART_CHECK(dart_size(&nprocs));
    DART_CHECK(dart_myid(&myid));
    rank = myid.id;

    if (0 == rank) {
        switch (po_ret) {
            case po_bad_usage:
            case po_help_message:
                usage("osu_get_latency");
                break;
        }
    }

    switch (po_ret) {
        case po_bad_usage:
            dart_exit();
            exit(EXIT_FAILURE);
        case po_help_message:
            dart_exit();
            exit(EXIT_SUCCESS);
        case po_okay:
            break;
    }

#if 0
    if(nprocs != 2) {
        if(rank == 0) {
            fprintf(stderr, "This test requires exactly two processes\n");
        }

        DART_CHECK(dart_exit());

        return EXIT_FAILURE;
    }
#endif


  dash::util::BenchmarkParams bench_params("OSU DART_Get Latency Test");
  bench_params.print_header();
  bench_params.print_pinning();

  auto uloc = dash::util::UnitLocality(dash::myid());
  auto tloc = dash::util::TeamLocality(dash::Team::All());

  auto node_domains = tloc.domain().scope_domains(dash::util::Locality::Scope::Node);

  dart_global_unit_t u_target{dash::size() - 1};
/*
  if (node_domains.size() > 1) {
      u_target = node_domains[1].leader_unit();
  } else {
      u_target = node_domains[0].units().back();
  }

*/
  int target = u_target.id;

  print_header(rank, target, win_type, sync_type);

  if (rank == 0 || rank == target) {
      sbuf_original = new char[MYBUFSIZE];
      rbuf_original = new char[MYBUFSIZE];
  }

    switch (sync_type){
        case FLUSH_LOCAL:
            run_get_with_flush_local(rank, target, win_type);
            break;
        case BLOCKING:
            run_get_blocking(rank, target, win_type);
            break;
        case REQ_HANDLE:
            run_get_with_handle(rank, target, win_type);
            break;
        default:
            run_get_with_flush(rank, target, win_type);
            break;
    }

    delete[] sbuf_original;
    delete[] rbuf_original;
    dash::finalize();

    return EXIT_SUCCESS;
}

void print_header (int rank, int target, WINDOW win, SYNC sync)
{
    if(rank == 0) {
        fprintf(stdout, "# Window creation: %s\n",
                win_info[win]);
        fprintf(stdout, "# Synchronization: %s\n",
               sync_info[sync]);

        auto loc = dash::util::UnitLocality(dart_global_unit_t{target});

        fprintf(stdout, "# Target Unit { id: %d }\n",
               loc.unit_id().id);

        printf("%-*s%*s\n", 10, "# Size", FIELD_WIDTH, "Latency (us)");
        fflush(stdout);
    }
}

void print_latency(int rank, int size)
{
    if (rank == 0) {
        fprintf(stdout, "%-*d%*.*f\n", 10, size, FIELD_WIDTH,
                FLOAT_PRECISION,(t_end - t_start) * 1.0e6 / options.loop);
        fflush(stdout);
    }
}

/*Run get with flush_local */
void run_get_with_flush_local (int rank, int target, WINDOW type)
{
    int size, i;
    dart_gptr_t gptr;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : 1)) {
        auto const sz = (rank == 0 || rank == target) ? size : 0;
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &gptr, sz, type);
        dart_gptr_setunit(&gptr, dart_create_team_unit(target));
        gptr.addr_or_offs.offset = 0;

        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime();
                }
                DART_CHECK(dart_get(rbuf, gptr, size, DART_TYPE_BYTE));
                DART_CHECK(dart_flush_local(gptr));
            }
            t_end = MPI_Wtime();
        }

        DART_CHECK(dart_barrier(DART_TEAM_ALL));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, gptr, rank);
    }
}

/*Run get_blocking */
void run_get_blocking (int rank, int target, WINDOW type)
{
    int size, i;
    dart_gptr_t gptr;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : 1)) {
        auto const sz = (rank == 0 || rank == target) ? size : 0;
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &gptr, sz, type);
        dart_gptr_setunit(&gptr, dart_create_team_unit(target));
        gptr.addr_or_offs.offset = 0;

        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime();
                }
                DART_CHECK(dart_get_blocking(rbuf, gptr, size, DART_TYPE_BYTE));
            }
            t_end = MPI_Wtime();
        }

        DART_CHECK(dart_barrier(DART_TEAM_ALL));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, gptr, rank);
    }
}


void run_get_with_handle (int rank, int target, WINDOW type)
{
    int size, i;
    dart_gptr_t gptr;
    dart_handle_t handle;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : 1)) {
        auto const sz = (rank == 0 || rank == target) ? size : 0;
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &gptr, sz, type);
        dart_gptr_setunit(&gptr, dart_create_team_unit(target));
        gptr.addr_or_offs.offset = 0;

        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime();
                }
                DART_CHECK(dart_get_handle(rbuf, gptr, size, DART_TYPE_BYTE, &handle));
                DART_CHECK(dart_wait(handle));
            }
            t_end = MPI_Wtime();
        }

        DART_CHECK(dart_barrier(DART_TEAM_ALL));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, gptr, rank);
    }
}

/*Run get with flush */
void run_get_with_flush (int rank, int target, WINDOW type)
{
    int size, i;
    dart_gptr_t gptr;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : 1)) {
        auto const sz = (rank == 0 || rank == target) ? size : 0;
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &gptr, sz, type);
        dart_gptr_setunit(&gptr, dart_create_team_unit(target));
        gptr.addr_or_offs.offset = 0;

        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime();
                }
                DART_CHECK(dart_get(rbuf, gptr, size, DART_TYPE_BYTE));
                DART_CHECK(dart_flush(gptr));
            }
            t_end = MPI_Wtime();
        }

        DART_CHECK(dart_barrier(DART_TEAM_ALL));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, gptr, rank);
    }
}

/* vi: set sw=4 sts=4 tw=80: */
