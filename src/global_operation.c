/* 
 * Copyright (C) 2014 FillZpp
 */

#include "global_operation.h"

#include <unistd.h>                // For sbrk, getpagesize
#include <pthread.h>


/* =================================================================== */
/*                Definitions for static variables                     */
/* =================================================================== */
static struct thread_cache *thread_slab = NULL;
static struct thread_cache *free_thread = NULL;
static struct central_cache *used_central = NULL;
static struct central_cache *free_central = NULL;
static pthread_key_t tkey;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;


/* =================================================================== */
/*                Declaration for static functions                     */
/* =================================================================== */
static void central_init(struct central_cache *cc);
static void *get_align_brk(void);
static void global_add_central (void);
static void thread_destructor (void *ptr);
static struct thread_cache *thread_init (void);
static void init_once(void);
static void func_before_main(void)  __attribute__((constructor));


/* =================================================================== */
/*                Definitions for global operations                    */
/* =================================================================== */
struct thread_cache *get_current_thread(void) {
	pthread_once(&once, init_once);
	struct thread_cache *tc = pthread_getspecific(tkey);
        
	if (tc == NULL)
		tc = thread_init();
        
	return tc;
}

void thread_add_central(struct thread_cache *tc) {
	struct central_cache *new_cc = NULL;

	pthread_mutex_lock(&mutex);     // Lock
        
	/* Check if there is free central cache */
	if (free_central == NULL)
		global_add_central();
        
	/* Get first free central cache */
	free_central = (new_cc = free_central)->next;
	/* Add this central cache to used_central */
	new_cc->used_next = used_central;
	used_central = new_cc;
        
	pthread_mutex_unlock(&mutex);   // Unlock
	
	new_cc->next = NULL;
	pthread_mutex_lock(&new_cc->central_mutex);
	new_cc->tc = tc;
	pthread_mutex_unlock(&new_cc->central_mutex);
	new_cc->next = tc->cc;
	tc->cc = new_cc;
}

struct central_cache *find_central_of_pointer(void *ptr) {
	struct central_cache *cc = used_central;

	// Check if pointer is in heap
	if ((int)ptr > sbrk(0))
		return NULL;
        
	while (cc != NULL) {
		if (ptr>(void*)cc && ptr<(void*)cc+central_cache_size)
			break;
		cc = cc->used_next;
	}

	if (cc == NULL) {
		cc = free_central;
		while (cc != NULL) {
			if (ptr>(void*)cc && ptr<(void*)cc+central_cache_size)
				break;
			cc = cc->next;
		}
	}
	
	return cc;
}


/* =================================================================== */
/*                 Definitions for static functions                    */
/* =================================================================== */

/* Initialize a central cache when get it from system */
static void central_init(struct central_cache *cc) {
	pthread_mutex_init(&cc->central_mutex, NULL);
	cc->used_next = NULL;
	cc->wait_free_chunk = NULL;
	cc->free_chunk = (void*)cc + chunk_size[1] - chunk_head_size;
	cc->free_chunk->seek = central_cache_size - chunk_size[1];
	cc->free_chunk->next = NULL;
}

static void *get_align_brk(void) {
	int page_size = getpagesize();
	int current_brk = sbrk(0);
	int remainder = current_brk % page_size;

	/* Check if current brk aligns with page size */
	if (remainder != 0)
		sbrk(page_size - remainder);
        
	return sbrk(central_cache_size);
}

static void global_add_central(void) {
	int index;
	struct central_cache *cc = NULL;

	/* Initialize first central cahce */
	free_central = get_align_brk();
	cc = free_central;
	central_init(cc);

	/* Initialize other central cache */
	for (index = num_of_add_central-1; index > 0; --index) {
		cc->next = sbrk(central_cache_size);
		cc = cc->next;
		central_init(cc);
	}
	cc->next = NULL;
}

static void thread_destructor(void *ptr) {
	struct thread_cache *tc = ptr;
	struct central_cache *cc = tc->cc;
	struct central_cache *used_cc = NULL;
	struct central_cache *next_cc = NULL;

	pthread_mutex_lock(&mutex);     // Lock
	while (cc != NULL) {
		next_cc = cc->next;
		pthread_mutex_lock(&cc->central_mutex);     // Central lock
		cc->tc = NULL;
                
		/* Take cc off from used_central */
		used_cc = used_central;
		if (used_cc == cc) {
			used_central = cc->used_next;
		} else {
			while (used_cc->used_next != cc)
				used_cc = used_cc->used_next;
			used_cc->used_next = cc->used_next;
		}
		/* Add cc to free_central */
		cc->next = free_central;
		free_central = cc;
                
		pthread_mutex_unlock(&cc->central_mutex);   // Central unclok
		cc = next_cc;
	}
        
	/* Give back tc */
	tc->next = free_thread;
	free_thread = tc;

	pthread_mutex_unlock(&mutex);   // Unlock
}

static struct thread_cache *thread_init(void) {
	struct central_cache *cc = NULL;
	struct thread_cache *tc  = NULL;

	pthread_mutex_lock(&mutex);     // Lock
        
	/* Get struct thread_cache */
	if (free_thread == NULL)
		tc = thread_slab++;
	else
		free_thread = (tc = free_thread)->next;
        
	/* Check if there is free central cache */
	if (free_central == NULL)
		global_add_central();
	/* Get a free central cache */
	free_central = (cc = free_central)->next;
	/* add this central cache to used_central */
	if (used_central == NULL) {
		used_central = cc;
	} else {
		cc->used_next = used_central;
		used_central = cc;
	}
        
	/* Set tc as thread private data */
	pthread_setspecific(tkey, tc);
	pthread_mutex_unlock(&mutex);   // Unlock
        
	cc->next = NULL;
	pthread_mutex_lock(&cc->central_mutex);
	cc->tc = tc;
	pthread_mutex_unlock(&cc->central_mutex);
	tc->cc = cc;
        
	return tc;
}

static void init_once(void) {
	pthread_key_create(&tkey, &thread_destructor);
	/* Allocate for struct thread_cache */
	thread_slab  = sbrk(thread_slab_size);

	/* Initialize global central cache
	   and allocate for main thread */
	global_add_central();
}

static void func_before_main(void) {
	pthread_once(&once, init_once);
}

