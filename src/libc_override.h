/*
 * Copyright (C) 2014 FillZpp
 */


#ifndef PSMALLOC_LIBC_OVERRIDE_H_
#define PSMALLOC_LIBC_OVERRIDE_H_


#include <sys/cdefs.h>        // for __THROW

#include "psmalloc.h"


#ifndef __THROW
#define __THROW
#endif


#define ALIAS(tc_fn)   __attribute__ ((alias (#tc_fn)))

/* Override */
void *malloc (size_t size) __THROW                      ALIAS(ps_malloc);
void *calloc (size_t n, size_t size) __THROW            ALIAS(ps_calloc);
void free (void *ptr) __THROW                           ALIAS(ps_free);
void cfree (void *ptr) __THROW                          ALIAS(ps_cfree);
void *realloc (void *ptr, size_t size) __THROW          ALIAS(ps_realloc);
void *memalign (size_t align, size_t size) __THROW      ALIAS(ps_memalign);
void *valloc (size_t size) __THROW                      ALIAS(ps_valloc);

#undef ALIAS

#endif

