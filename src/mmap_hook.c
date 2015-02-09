/* 
 * Copyright (C) 2014 FillZpp
 */


#include "mmap_hook.h"

#include <sys/mman.h>
#include <string.h>

#include "global_operation.h"
#include "heap_hook.h"


void *mmap_alloc_hook(size_t size) {
	ChunkHead *mm = NULL;

	/* Get mmap */
	mm = mmap(NULL, size + chunk_head_size, PROT_READ | PROT_WRITE,
		  MAP_PRIVATE | MAP_ANON, -1, 0);
	mm->seek = size;
        
	return mm + 1;
}

void *mmap_realloc_hook(void *ptr, size_t size) {
	ChunkHead *new_mm = NULL;
	// It could be in mmap or central
	ChunkHead *old_mm = ptr - chunk_head_size;

	if (old_mm->seek >= size)
		return ptr;
        
	new_mm = mmap(NULL, size + chunk_head_size, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANON, -1, 0);
	memcpy(new_mm + 1, ptr, old_mm->seek);
        
	// Release old
	if ((void*)old_mm > sbrk(0))    // Old pointer is in mmap
		do_mmap_free(old_mm);
	else                     // Old pointer is in heap
		do_chunk_free(find_central_of_pointer(old_mm), old_mm);

	return new_mm + 1;
}

void do_mmap_free(ChunkHead *old_mm) {
	munmap(old_mm, old_mm->seek + chunk_head_size);
}
