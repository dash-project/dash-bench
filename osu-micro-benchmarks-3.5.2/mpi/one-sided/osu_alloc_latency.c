#define BENCHMARK "OSU MPI_Alloc%s latency Test"
/*
 * Copyright (C) 2003-2016 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University.
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include "osu_1sc.h"

#define MAX_SIZE (1<<24)
#define MYBUFSIZE (MAX_SIZE + MAX_ALIGNMENT)

#define SKIP_LARGE  10
#define LOOP_LARGE  100
#define LARGE_MESSAGE_SIZE  8192

#ifdef PACKAGE_VERSION
#   define HEADER "# " BENCHMARK " v" PACKAGE_VERSION "\n"
#else
#   define HEADER "# " BENCHMARK "\n"
#endif

double  t_start = 0.0, t_end = 0.0;
char    sbuf_original[MYBUFSIZE];
char    rbuf_original[MYBUFSIZE];
char    *sbuf=NULL, *rbuf=NULL;

void print_header (int, WINDOW, SYNC);
void print_latency (int, int);
void run_win_create(int rank, int target, WINDOW type);
#if MPI_VERSION >= 3
void run_win_allocate(int rank, int target, WINDOW type);
void run_win_dynamic_shared_attach(int rank, int target, WINDOW type);
#endif

int main (int argc, char *argv[])
{
    int         rank,nprocs;
    int         po_ret = po_okay;
#if MPI_VERSION >= 3
    WINDOW      win_type = WIN_ALLOCATE;
    SYNC        sync_type = FLUSH;
#else
    WINDOW      win_type = WIN_CREATE;
    SYNC        sync_type = LOCK;
#endif

    po_ret = process_options(argc, argv, &win_type, &sync_type, all_sync);

    if (po_okay == po_ret && none != options.accel) {
        if (init_accel()) {
           fprintf(stderr, "Error initializing device\n");
            exit(EXIT_FAILURE);
        }
    }

    MPI_CHECK(MPI_Init(&argc, &argv));
    MPI_CHECK(MPI_Comm_size(MPI_COMM_WORLD, &nprocs));
    MPI_CHECK(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    if (0 == rank) {
        switch (po_ret) {
            case po_cuda_not_avail:
                fprintf(stderr, "CUDA support not enabled.  Please recompile "
                        "benchmark with CUDA support.\n");
                break;
            case po_openacc_not_avail:
                fprintf(stderr, "OPENACC support not enabled.  Please "
                        "recompile benchmark with OPENACC support.\n");
                break;
            case po_bad_usage:
            case po_help_message:
                usage(all_sync, "osu_get_latency");
                break;
        }
    }

    switch (po_ret) {
        case po_cuda_not_avail:
        case po_openacc_not_avail:
        case po_bad_usage:
            MPI_Finalize();
            exit(EXIT_FAILURE);
        case po_help_message:
            MPI_Finalize();
            exit(EXIT_SUCCESS);
        case po_okay:
            break;
    }

    print_header(rank, win_type, sync_type);
    int target = nprocs -1;

    switch (win_type){
        case WIN_CREATE:
          run_win_create(rank, target, win_type);
          break;
#if MPI_VERSION >= 3
        case WIN_ALLOCATE:
          run_win_allocate(rank, target, win_type);
          break;
        case WIN_DYNAMIC:
          run_win_dynamic_shared_attach(rank,target,  win_type);
          break;
        default:
          assert(0);
          break;
#endif
    }

    MPI_CHECK(MPI_Finalize());

    if (none != options.accel) {
        if (cleanup_accel()) {
            fprintf(stderr, "Error cleaning up device\n");
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

void print_header (int rank, WINDOW win, SYNC sync)
{
    if(rank == 0) {
        switch (options.accel) {
            case cuda:
                printf(HEADER, "-CUDA");
                break;
            case openacc:
                printf(HEADER, "-OPENACC");
                break;
            default:
                printf(HEADER, "");
                break;
        }

        fprintf(stdout, "# Window creation: %s\n",
                win_info[win]);
        fprintf(stdout, "# Synchronization: %s\n",
                sync_info[sync]);

        switch (options.accel) {
            case cuda:
            case openacc:
                printf("# Rank 0 Memory on %s and Rank 1 Memory on %s\n",
                        'D' == options.rank0 ? "DEVICE (D)" : "HOST (H)",
                        'D' == options.rank1 ? "DEVICE (D)" : "HOST (H)");
            default:
                fprintf(stdout, "%-*s%*s\n", 10, "# Size", FIELD_WIDTH, "Latency (us)");
                fflush(stdout);
        }
    }
}

void print_latency(int rank, int size)
{
    if (rank == 0) {
        fprintf(stdout, "%-*d%*.*f\n", 10, size, FIELD_WIDTH,
                FLOAT_PRECISION, (t_end - t_start) * 1.0e6 / options.loop);
        fflush(stdout);
    }
}


void run_win_create(int rank, int target, WINDOW type)
{
    int size, i;
    options.loop = LOOP_LARGE;
    options.skip = SKIP_LARGE;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : size + 1)) {


        for (i = 0; i < options.skip + options.loop; i++) {
            if (i == options.skip) {
                t_start = MPI_Wtime ();
            }
            char *baseptr = malloc(size);
            MPI_Win win;
            MPI_CHECK(MPI_Win_create(baseptr, size, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win));
            MPI_CHECK(MPI_Win_free(&win));
            free(baseptr);
        }
        t_end = MPI_Wtime ();

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        print_latency(rank, size);
    }
}

#if MPI_VERSION >= 3

void run_win_allocate(int rank, int target, WINDOW type)
{
    int size, i;

    options.loop = LOOP_LARGE;
    options.skip = SKIP_LARGE;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : size + 1)) {

        for (i = 0; i < options.skip + options.loop; i++) {
            if (i == options.skip) {
                t_start = MPI_Wtime ();
            }
            char *baseptr;
            MPI_Win win;
            MPI_CHECK(MPI_Win_allocate(size, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &baseptr, &win));
            MPI_CHECK(MPI_Win_free(&win));
        }
        t_end = MPI_Wtime ();

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        print_latency(rank, size);
    }
}

void run_win_dynamic_shared_attach(int rank, int target, WINDOW type)
{

  options.loop = LOOP_LARGE;
  options.skip = SKIP_LARGE;

  int size, i;
  int comm_size;
  MPI_Comm sharedmem_comm;
  MPI_Win  dynamic_win;
  MPI_Info win_info;
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  MPI_Info_create(&win_info);
  MPI_Info_set(win_info, "alloc_shared_noncontig", "true");

  MPI_CHECK(MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &dynamic_win));

  MPI_Group sharedmem_group, group_all;
  MPI_Comm_split_type(
    MPI_COMM_WORLD,
    MPI_COMM_TYPE_SHARED,
    1,
    MPI_INFO_NULL,
    &sharedmem_comm);

  for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : size + 1)) {


      for (i = 0; i < options.skip + options.loop; i++) {
          if (i == options.skip) {
              t_start = MPI_Wtime ();
          }
          char *baseptr;
          MPI_Aint* disp_set = malloc(comm_size * sizeof(MPI_Aint));
          MPI_Aint  disp_local;
          MPI_Win win;
          MPI_CHECK(MPI_Win_allocate_shared(size, 1, win_info, sharedmem_comm, &baseptr, &win));
          if (size > 0) {
            // it's erroneous to attach an empty window
            MPI_CHECK(MPI_Win_attach(dynamic_win, baseptr, size));
          }
          MPI_CHECK(MPI_Get_address(baseptr, &disp_local));
          MPI_CHECK(MPI_Allgather(&disp_local, 1, MPI_AINT, disp_set, 1, MPI_AINT, MPI_COMM_WORLD));
          if (size > 0) {
            MPI_CHECK(MPI_Win_detach(dynamic_win, baseptr));
          }
          MPI_CHECK(MPI_Win_free(&win));
          free(disp_set);
      }
      t_end = MPI_Wtime ();

      MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

      print_latency(rank, size);
  }
  MPI_CHECK(MPI_Info_free(&win_info));
  MPI_CHECK(MPI_Win_free(&dynamic_win));
  MPI_CHECK(MPI_Comm_free(&sharedmem_comm));
}
#endif
/* vi: set sw=4 sts=4 tw=80: */
