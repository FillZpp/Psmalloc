/* 
 * Copyright (C) FillZpp
 */


#ifndef PSMALLOC_MMAP_HOOK_H_
#define PSMALLOC_MMAP_HOOK_H_


#include <stddef.h>        // for size_t
#include "crconfig.h"


void *mmap_alloc_hook (size_t size);

void *mmap_realloc_hook (void *ptr, size_t size);

void do_mmap_free (struct chunk_head *old_mm);


#endif
