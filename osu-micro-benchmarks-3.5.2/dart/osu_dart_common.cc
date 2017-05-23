/*
 * Copyright (C) 2003-2016 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University.
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include "osu_dart_common.h"


char const *win_info[20] = {
    "dart_team_memalloc_aligned",
};

char const *sync_info[20] = {
    "dart_flush",
    "dart_flush_local",
    "dart_handle",
};

struct options_t options;

void
usage (char const * name)
{
    printf("Usage: %s [options] \n", name);

    printf("options:\n");

    printf("  -s <sync_option>\n");
    printf("            <sync_option> can be any of the follows:\n");
    printf("            flush             use dart_flush synchronization call\n");
    printf("            flush_local       use dart_flush_local synchronization call\n");
    printf("            blocking          use blocking variant of calls\n");
    printf("\n");
    printf("  -x ITER       number of warmup iterations to skip before timing"
            "(default 100)\n");
    printf("  -i ITER       number of iterations for timing (default 10000)\n");

    printf("  -h            print this help message\n");

    fflush(stdout);
}

int
process_options(int argc, char *argv[], WINDOW *win, SYNC *sync)
{
    extern char *optarg;
    extern int  optind;
    extern int opterr;
    int c;

    /*
     * set default options
     */
    options.rank0 = 'H';
    options.rank1 = 'H';
    options.loop = 10000;
    options.loop_large = 1000;
    options.skip = 100;
    options.skip_large = 10;

    char const * optstring = "+s:h:x:i:";

    while((c = getopt(argc, argv, optstring)) != -1) {
        switch (c) {
            case 'x':
                options.skip = atoi(optarg);
                break;
            case 'i':
                options.loop = atoi(optarg);
                break;
            case 's':
                    if (0 == strcasecmp(optarg, "flush")) {
                        *sync = FLUSH;
                    }
                    else if (0 == strcasecmp(optarg, "flush_local")) {
                        *sync = FLUSH_LOCAL;
                    }
                    else if (0 == strcasecmp(optarg, "blocking")) {
                        *sync = BLOCKING;
                    }
                    else if (0 == strcasecmp(optarg, "req_handle")) {
                        *sync = REQ_HANDLE;
                    }
                    else {
                        return po_bad_usage;
                    }
                break;

            case 'h':
                return po_help_message;
            default:
                return po_bad_usage;
        }
    }

    return po_okay;
}

void *
align_buffer (void * ptr, unsigned long align_size)
{
    return (void *)(((unsigned long)ptr + (align_size - 1)) / align_size *
            align_size);
}

void
allocate_memory(int rank, char *sbuf_orig, char *rbuf_orig, char **sbuf, char **rbuf,
    dart_gptr_t * gptr, int size, WINDOW type)
{
    int page_size;

    page_size = sysconf(_SC_PAGESIZE);
    assert(page_size <= MAX_ALIGNMENT);

    if (size) {
     *sbuf = (char *)align_buffer((void *)sbuf_orig, page_size);
     memset(*sbuf, 'a', size);
     *rbuf = (char *)align_buffer((void *)rbuf_orig, page_size);
     memset(*rbuf, 'b', size);
    }

    DART_CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, size, DART_TYPE_BYTE, gptr));
}

#if 0
void
allocate_atomic_memory(int rank, char *sbuf_orig, char *rbuf_orig, char *tbuf_orig,
            char *cbuf_orig, char **sbuf, char **rbuf, char **tbuf,
            char **cbuf, dart_gptr_t *gptr, int size, WINDOW type)
{
    int page_size;

    page_size = getpagesize();
    assert(page_size <= MAX_ALIGNMENT);

     *sbuf = (char *)align_buffer((void *)sbuf_orig, page_size);
     memset(*sbuf, 'a', size);
     *rbuf = (char *)align_buffer((void *)rbuf_orig, page_size);
     memset(*rbuf, 'b', size);
     *tbuf = (char *)align_buffer((void *)tbuf_orig, page_size);
     memset(*tbuf, 'c', size);
     if (cbuf != NULL) {
         *cbuf = (char *)align_buffer((void *)cbuf_orig, page_size);
         memset(*cbuf, 'a', size);
     }

    DART_CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, size, DART_TYPE_BYTE, gptr));
}

void
free_atomic_memory (void *sbuf, void *rbuf, void *tbuf, void *cbuf, dart_gptr_t gptr, int rank)
{
   // MPI_Win_free(&win);
  //dart_mem_free(...)
  dart_team_memfree(gptr);
}
#endif

void
free_memory (void *sbuf, void *rbuf, dart_gptr_t gptr, int rank)
{

  dart_team_memfree(gptr);

}

