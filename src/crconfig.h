/*
 * Copyright (C) 2014 FillZpp
 */


#ifndef PSMALLOC_CRCONFIG_H_
#define PSMALLOC_CRCONFIG_H_


#include <pthread.h>
#include <stdint.h>           /* for uint8_t, uint16_t, uint32_t */
#include <stddef.h>           /* for size_t */


/* Version of PSMalloc */
#define __PSMALLOC__ 0
#define __PSMALLOC_MINOR__ 1


/* 
  **************************************************
  Define structs
  **************************************************
*/

/* Each thread has a thread_cache */
typedef struct ThreadCache {
	struct CentralCache *cc;
        struct ThreadCache  *next;
} ThreadCache;

/* Each central has a central_cache at the beginning */
typedef struct CentralCache {
        pthread_mutex_t      central_mutex;
	struct ChunkHead    *wait_free_chunk;
        struct ChunkHead    *free_chunk;
        struct CentralCache *used_next;
        struct CentralCache *next;
        ThreadCache  *tc;
} CentralCache;

/* Each chunk has a chunk_head at the beginning */
typedef struct ChunkHead {
        uint16_t           kind;
        uint16_t           num;
        size_t             seek;
        struct ChunkHead *next;
} ChunkHead;


/*
  **************************************************
  Configuration
  **************************************************
*/

/* Size of chunk_head */
static const size_t chunk_head_size = sizeof(ChunkHead);
/* Size of slab for central and thread struct */
static const size_t thread_slab_size = 500 * sizeof(ThreadCache);
/* Size of each central cache */
static const size_t central_cache_size = 1024*512;  // 512 KB
/* The number of central caches when add */
static const size_t num_of_add_central = 4;
/* Num of kinds */
static const size_t num_of_kinds = 5;

/* Size of four kinds of chunk in each thread */
static const size_t chunk_size[] = {
        64,          // 64 Bytes
        256,         // 256 Bytes
        1024,        // 1 KB
        1024*4,      // 4 KB
        1024*16      // 16 KB
};

/* Critical size
   Allocation more than this size should use mmap */
static const size_t critical_size = 1024*512 - 1024*16;

#endif
