/* 
 * Copyright (C) 2014 FillZpp
 */


#ifndef PSMALLOC_HEAP_HOOK_H_
#define PSMALLOC_HEAP_HOOK_H_


#include "crconfig.h"


/* Allocate chunk in central cache */
void *chunk_alloc_hook(size_t size, size_t align);

/* Reallocate chunk in central cache */
void *chunk_realloc_hook(void *ptr, size_t size);

/* Release chunk in central cache */
void do_chunk_free(CentralCache *cc, ChunkHead *ch);


#endif
