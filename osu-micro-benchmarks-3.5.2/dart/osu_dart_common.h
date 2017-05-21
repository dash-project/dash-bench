/*
  bench_params.print_header();
 * Copyright (C) 2003-2016 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University.
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <libdash.h>

#define MAX_ALIGNMENT 65536

#ifndef FIELD_WIDTH
#   define FIELD_WIDTH 20
#endif

#ifndef FLOAT_PRECISION
#   define FLOAT_PRECISION 2
#endif

#define CHECK(stmt)                                              \
do {                                                             \
   int ret = (stmt);                                           \
   if (0 != ret) {                                             \
       fprintf(stderr, "[%s:%d] function call failed with %d \n",\
        __FILE__, __LINE__, ret);                              \
       exit(EXIT_FAILURE);                                       \
   }                                                             \
   assert(ret == 0);                                           \
} while (0)

#define DART_CHECK(stmt)                                              \
do {                                                             \
   int ret = (stmt);                                           \
   if (DART_OK != ret) {                                             \
       fprintf(stderr, "[%s:%d] function call failed with %d \n",\
        __FILE__, __LINE__, ret);                              \
       exit(EXIT_FAILURE);                                       \
   }                                                             \
   assert(ret == 0);                                           \
} while (0)

/*structures, enumerators and such*/
/* Window creation */
typedef enum {
    DART_ALLOCATE,
} WINDOW;

/* Synchronization */
typedef enum {
    FLUSH,
    FLUSH_LOCAL,
    REQ_HANDLE,
    BLOCKING
} SYNC;

enum po_ret_type {
    po_bad_usage,
    po_help_message,
    po_okay,
};

struct options_t {
    char rank0;
    char rank1;
    int loop;
    int loop_large;
    int skip;
    int skip_large;
};

typedef dash::util::Timer<dash::util::TimeMeasure::Clock> Timer;

extern struct options_t options;

/*variables*/
extern char const *win_info[20];
extern char const *sync_info[20];

/*function declarations*/
void usage (char const *);
int  process_options (int, char **, WINDOW*, SYNC*);
void allocate_memory(int, char *, char *, char **, char **,
            dart_gptr_t *gptr, int, WINDOW);
void free_memory (void *, void *, dart_gptr_t, int);

#if 0
void allocate_atomic_memory(int, char *, char *, char *,
            char *, char **, char **, char **, char **,
            dart_gptr_t *, int, WINDOW);
void free_atomic_memory (void *, void *, void *, void *, dart_gptr_t, int);
#endif

