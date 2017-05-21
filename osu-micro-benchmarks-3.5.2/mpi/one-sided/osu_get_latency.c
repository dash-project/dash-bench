#define BENCHMARK "OSU MPI_Get%s latency Test"
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
void run_get_with_lock (int, int, WINDOW);
#if MPI_VERSION >= 3
void run_get_with_lock_all (int,int, WINDOW);
void run_get_with_flush (int,int,  WINDOW);
void run_get_with_flush_local (int, int, WINDOW);
void run_get_with_wait (int, int, WINDOW);
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

    switch (sync_type){
        case LOCK:
            run_get_with_lock(rank, target, win_type);
            break;
#if MPI_VERSION >= 3
        case FLUSH_LOCAL:
            run_get_with_flush_local(rank, target, win_type);
            break;
        case LOCK_ALL:
            run_get_with_lock_all(rank,target,  win_type);
            break;
        case WAIT:
            run_get_with_wait(rank,target,  win_type);
            break;
        default:
            run_get_with_flush(rank, target, win_type);
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

#if MPI_VERSION >= 3
/*Run Get with flush */
void run_get_with_flush(int rank, int target, WINDOW type)
{
    int size, i;
    MPI_Aint disp = 0;
    MPI_Win     win;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : size + 1)) {
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &sbuf, size, type, &win);

        if (type == WIN_DYNAMIC) {
            disp = disp_remote[target];
        }
        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            MPI_CHECK(MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win));
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime ();
                }
                MPI_CHECK(MPI_Get(rbuf, size, MPI_CHAR, target, disp, size, MPI_CHAR, win));
                MPI_CHECK(MPI_Win_flush(target, win));
            }
            t_end = MPI_Wtime ();
            MPI_CHECK(MPI_Win_unlock(target, win));
        }

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, win, rank);
    }
}

/*Run Get with flush local */
void run_get_with_flush_local(int rank, int target, WINDOW type)
{
    int size, i;
    MPI_Aint disp = 0;
    MPI_Win     win;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : size + 1)) {
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &sbuf, size, type, &win);

        if (type == WIN_DYNAMIC) {
            disp = disp_remote[target];
        }

        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            MPI_CHECK(MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win));
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime ();
                }
                MPI_CHECK(MPI_Get(rbuf, size, MPI_CHAR, target, disp, size, MPI_CHAR, win));
                MPI_CHECK(MPI_Win_flush_local(target, win));
            }
            t_end = MPI_Wtime ();
            MPI_CHECK(MPI_Win_unlock(target, win));
        }

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, win, rank);
    }
}

/*Run Get with Lock_all/unlock_all */
void run_get_with_lock_all(int rank, int target, WINDOW type)
{
    int size, i;
    MPI_Aint disp = 0;
    MPI_Win     win;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : size + 1)) {
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &sbuf, size, type, &win);

        if (type == WIN_DYNAMIC) {
            disp = disp_remote[target];
        }

        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime ();
                }
                MPI_CHECK(MPI_Win_lock_all(0, win));
                MPI_CHECK(MPI_Get(rbuf, size, MPI_CHAR, target, disp, size, MPI_CHAR, win));
                MPI_CHECK(MPI_Win_unlock_all(win));
            }
            t_end = MPI_Wtime ();
        }

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, win, rank);
    }
}

void run_get_with_wait(int rank, int target, WINDOW type)
{
    int size, i;
    MPI_Aint disp = 0;
    MPI_Win     win;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : size + 1)) {
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &sbuf, size, type, &win);

        if (type == WIN_DYNAMIC) {
            disp = disp_remote[target];
        }

        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            MPI_CHECK(MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win));
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime ();
                }
                MPI_Request req;
                MPI_CHECK(MPI_Rget(rbuf, size, MPI_CHAR, target, disp, size, MPI_CHAR, win, &req));
                MPI_CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));
            }
            t_end = MPI_Wtime ();
            MPI_CHECK(MPI_Win_unlock(target, win));
        }

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, win, rank);
    }
}
#endif

/*Run Get with Lock/unlock */
void run_get_with_lock(int rank, int target, WINDOW type)
{
    int size, i;
    MPI_Aint disp = 0;
    MPI_Win     win;

    for (size = 0; size <= MAX_SIZE; size = (size ? size * 2 : size + 1)) {
        allocate_memory(rank, sbuf_original, rbuf_original, &sbuf, &rbuf, &sbuf, size, type, &win);

#if MPI_VERSION >= 3
        if (type == WIN_DYNAMIC) {
            disp = disp_remote[target];
        }
#endif

        if(size > LARGE_MESSAGE_SIZE) {
            options.loop = LOOP_LARGE;
            options.skip = SKIP_LARGE;
        }

        if(rank == 0) {
            for (i = 0; i < options.skip + options.loop; i++) {
                if (i == options.skip) {
                    t_start = MPI_Wtime ();
                }
                MPI_CHECK(MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win));
                MPI_CHECK(MPI_Get(rbuf, size, MPI_CHAR, target, disp, size, MPI_CHAR, win));
                MPI_CHECK(MPI_Win_unlock(target, win));
            }
            t_end = MPI_Wtime ();
        }

        MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

        print_latency(rank, size);

        free_memory (sbuf, rbuf, win, rank);
    }
}
/* vi: set sw=4 sts=4 tw=80: */
