/*
  Copyright (C) 2014 Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
  * Neither the name of Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
  WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#if USE_CILKPLUS
#include "cilkplus/parallel_stable_sort.h"
#endif
#if USE_TBB_LOWLEVEL
#include "tbb-lowlevel/parallel_stable_sort.h"
#endif
#if USE_TBB_HIGHLEVEL
#include "tbb-highlevel/parallel_stable_sort.h"
#endif
#if USE_OPENMP
#include "openmp/parallel_stable_sort.h"
#endif

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iterator>

#include "IndexedValue.h"

const size_t N_MAX = 1000000;

static Key Array[N_MAX];
static char Flag[N_MAX];

// Initialize Array with n elements.
void CreateDataSet( size_t n ) {
    HighValidIndex = n-1;
    // Keys will be in [0..m-1].  The limit almost ensures that some duplicate keys will occur.
    int m = 2*n;
    for( size_t i=0; i<n; ++i ) {
        Array[i].value = rand() % m;
        Array[i].index = i;
    }
}

// Check that Array is sorted, and sort is stable.
void CheckIsSorted( size_t n ) {
    std::memset(Flag,0,sizeof(Flag));
    for( size_t i=0; i<n; ++i ) {
        int k = Array[i].index;
        if( Flag[k] ) {
            printf("ERROR: duplicate!\n");
            abort();
        }
        Flag[k] = 1;
    }
    if( memchr(Flag,0,n) ) {
        printf("ERROR: missing value!\n");
        abort();
    }
    if( n<2 )
        return;
    for( size_t i=0; i<n-2; ++i ) {
        int ai = Array[i].value;
        int aj = Array[i+1].value;
        int bi = Array[i].index;
        int bj = Array[i+1].index;
        if( ai>aj ) {
            printf("ERROR: not sorted! array[%ld].value = %d > %d = array[%ld].value\n", long(i), ai, aj, long(i+1) );
            abort();
        } else if( ai==aj && bi>bj ) {
            printf("ERROR: not stable! array[%ld].index = %d > %d = array[%ld].index\n", long(i), bi, bj, long(i+1) );
            abort();
        }
    }
}

//! Test sort for n items
void Test( size_t n ) {
    CreateDataSet(n);
    Iterator i(Array,Array+n,0);
    int count0 = KeyCount;
#if USE_OPENMP
    if( rand()&0x100 )
#pragma omp parallel
#pragma omp master
        // Check that sort works when already in a parallel region.
        pss::parallel_stable_sort( i, i+n, KeyCompare(KeyCompare::Live) );
    else
#endif
    pss::parallel_stable_sort( i, i+n, KeyCompare(KeyCompare::Live) );
    int count1 = KeyCount;
    // Check that number of keys constructed are equal to number destroyed
    if( count0!=count1 ) {
        printf("ERROR: count difference = %d\n",count1-count0);
        abort();
    }
    // Check that keys were sorted
    CheckIsSorted(n);
}

int main() {
    printf("Testing for n =");
    for( int n=0; n<=N_MAX; n = n<10 ? n+1 : n*1.618f ) {
        printf(" %d", n);
        fflush(stdout);
        Test(n);
    }
    printf("\n");
    return 0;
}
