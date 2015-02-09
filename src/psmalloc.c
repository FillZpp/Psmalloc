/* 
 * Copyright (C) 2014 FillZpp
 */


#include "psmalloc.h"

#include <unistd.h>           // For getpagesize
#include <string.h>           // For memset

#include "crconfig.h"
#include "heap_hook.h"
#include "mmap_hook.h"
#include "libc_override.h"


static void *do_malloc(size_t size, size_t align) {
	void *ret = NULL;

	/* Check size */
	if (size <= 0)
		return NULL;

	if (size < critical_size)
		ret = chunk_alloc_hook(size, align);
	else
		ret = mmap_alloc_hook(size);

	return ret;
}

static void do_free(void *ptr) {
	CentralCache *cc = NULL;

	/* Check pointer */
	if (ptr == NULL)
		return;

	cc = find_central_of_pointer(ptr);
	if (cc != NULL)
		do_chunk_free(cc, ptr - chunk_head_size);
	else 
		do_mmap_free(ptr - chunk_head_size);
}

void *ps_malloc(size_t size) {
	void *ret = do_malloc(size, 0);
	return ret;
}

void *ps_calloc(size_t n, size_t size) {
	void *ret = do_malloc(n*size, 0);
	memset(ret, 0, size);
	return ret;
}

void ps_free(void *ptr) {
	do_free(ptr);
}

void ps_cfree(void *ptr) {
	do_free(ptr);
}

void *ps_memalign(size_t align, size_t size) {
	void *ret = do_malloc(size, align);
    return ret;
}

void *ps_valloc(size_t size) {
	void *ret = ps_memalign(getpagesize(), size);
	return ret;
}

void *ps_realloc(void *ptr, size_t size) {
	void *ret = NULL;

	/* Check pointer */
	if (ptr == NULL) {
		ret = do_malloc(size, 0);
		return ret;
	}
	/* Check size */
	if (size <= 0) {
		do_free(ptr);
		return NULL;
	}

	if (size < critical_size) {
		ret = chunk_realloc_hook(ptr, size);
	} else {
		ret = mmap_realloc_hook(ptr, size);
	}
	return ret;
}
