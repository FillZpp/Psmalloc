/* 
 * Copyright (C) 2014 FillZpp
 */


#include "heap_hook.h"

#include <stdint.h>
#include <string.h>           // For memcpy
#include <pthread.h>

#include "global_operation.h"


static uint16_t check_size(size_t size, uint16_t *kind) {
	int index;
	for (index=0; index<num_of_kinds-1; ++index) {
		if (size <= (3*chunk_size[index] - chunk_head_size))
			break;
	}
	*kind = index;
	return (size+chunk_head_size)/chunk_size[index] + 1;
}

static void *
get_appoint_chunk(CentralCache *cc, size_t tar_size, void *ptr) {
	ChunkHead *ch = cc->free_chunk;
	ChunkHead *prev_ch = ch;

	/* Search in free chunk */
	for(; ch!=NULL; prev_ch=ch, ch=ch->next) {
		if (ch==ptr && ch->seek>=tar_size)
			break;
	}

	/* Can not get appoint chunk */
	if (ch == NULL)
		return ch;

	/* If the size of target chunk is bigger than need */
	if (ch->seek != tar_size) {
		if (ch == cc->free_chunk) {
			cc->free_chunk = (void*)ch + tar_size;
			cc->free_chunk->seek = ch->seek - tar_size;
		} else {
			prev_ch->next = (void*)ch + tar_size;
			prev_ch->next->seek = ch->seek - tar_size;
		}
	} else {
		if (ch == cc->free_chunk)
			cc->free_chunk = ch->next;
		else
			prev_ch->next = ch->next;
	}
	return ch;
}

static void *
get_suitable_chunk(ThreadCache *tc, uint16_t kind, uint16_t num,
                   size_t align, ChunkHead *old_ch) {
	ChunkHead *ch = NULL;
	ChunkHead *prev_ch = NULL;
	CentralCache *cc = NULL;
	size_t tar_size = chunk_size[kind] * num;
	int index;
	size_t ali_num;
	if (align != 0 && (ali_num = align%chunk_size[0]))
		align = (align - ali_num) * 2;

	if (old_ch != NULL) {           // realloc
		cc = find_central_of_pointer(old_ch);
		if (old_ch->kind == kind) {                     // Same kind
			if (old_ch->num == num) {           // Same num
				return old_ch;
			} else if (old_ch->num > num) {     // Less num
				ch = (void*)old_ch + tar_size;
				ch->kind = kind;
				ch->num = old_ch->num - num;
				do_chunk_free(cc, ch);
                                
				old_ch->num = num;
				return old_ch;
			} else {                            // More num
				/* Check if it is free behind old chunk */
				ch = get_appoint_chunk(cc,
						       tar_size -
						       chunk_size[kind] *
						       old_ch->num,
						       (void*)old_ch +
						       old_ch->num *
						       chunk_size[kind]);
				if (ch != NULL) {
					old_ch->num = num;
					return old_ch;
				}
			}
		} else if (old_ch->kind > kind) {               // Smaller kind
			/* Turn to new kind */
			for (index=kind; index<old_ch->kind; ++index)
				old_ch->num *= 4;
			old_ch->kind = kind;

			ch = (void*)old_ch + tar_size;
			ch->kind = kind;
			ch->num = old_ch->num - num;
			do_chunk_free(cc, ch);

			old_ch->num = num;
			return old_ch;
		}
	}

	if (tc->cc == NULL)
		thread_add_central(tc);
	cc = tc->cc;
	/* Find in centrals */
	while(1) {
		prev_ch = cc->free_chunk;
		ch = prev_ch;
		/* Find in free chunks */
		for (; ch != NULL; prev_ch=ch, ch=ch->next) {
			if (ch->seek < tar_size)
				continue;
			if (!align || !(ali_num = (size_t)(ch+1)%align))
				break;
                        
			/* Check alignment and size */
			if (ch->seek < tar_size + align - ali_num)
				continue;
			ali_num = ch->seek + ali_num - align;
			ch->seek = ch->seek - ali_num;
			prev_ch = ch;
			ch = (void*)ch + ch->seek;
			ch->seek = ali_num;
			ch->next = prev_ch->next;
			prev_ch->next = ch;
			break;
		}
		if (ch != NULL)
			break;
		if (cc->next == NULL) {
			thread_add_central(tc);
			cc = tc->cc;
		} else {
			cc = cc->next;
		}
	}

	/* If the size of target chunk is bigger than need */
	if (ch->seek != tar_size) {
		if (ch == cc->free_chunk) {
			cc->free_chunk = (void*)ch + tar_size;
			cc->free_chunk->seek = ch->seek - tar_size;
			cc->free_chunk->next = ch->next;
		} else {
			prev_ch->next = (void*)ch + tar_size;
			prev_ch->next->seek = ch->seek - tar_size;
			prev_ch->next->next = ch->next;
		}
	} else {
		if (ch == cc->free_chunk)
			cc->free_chunk = ch->next;
		else
			prev_ch->next = ch->next;
	}

	/* Initialize final target chunk */
	ch->kind = kind;
	ch->num = num;
	return ch;
}

void do_chunk_free(CentralCache *cc, ChunkHead *ch) {
	ChunkHead *prev_ch = NULL;
	ChunkHead *next_ch = NULL;
	size_t check;

	pthread_mutex_lock(&cc->central_mutex);                  // Lock
	/* If this thread is not the owner of this cc */
	if (cc->tc!=NULL && get_current_thread() != cc->tc) {
		ch->next = cc->wait_free_chunk;
		cc->wait_free_chunk = ch;
		pthread_mutex_unlock(&cc->central_mutex);        // Unlock
		return;
	}

	while (1) {
		prev_ch = cc->free_chunk;
		next_ch = prev_ch;
		ch->seek = chunk_size[ch->kind] * ch->num;
		ch->next = NULL;

		for (; next_ch!=NULL; prev_ch=next_ch, next_ch=next_ch->next) {
			if (next_ch > ch)
				break;
		}

		/* Check arround target chunk */
		if (next_ch == cc->free_chunk) {
			check = 1;
			cc->free_chunk = ch;
		} else {
			check = (size_t)ch - (size_t)prev_ch;
			if (check > prev_ch->seek) {
				check = 2;
				ch->next = prev_ch->next;
				prev_ch->next = ch;
			} else if (check == prev_ch->seek) {
				check = 3;
				prev_ch->seek +=  ch->seek;
				ch = prev_ch;
			} else {
				check = 0;
			}
		}
                
		if (next_ch != NULL && check > 0) {
			if ((size_t)next_ch-(size_t)ch == ch->seek) {
				ch->seek += next_ch->seek;
				ch->next = next_ch->next;
			} else {
				ch->next = next_ch;
			}
		}

		/* Check wait_free_chunk */
		if ((ch = cc->wait_free_chunk) != NULL)
			cc->wait_free_chunk = ch->next;
		else 
			break;
	}
	pthread_mutex_unlock(&cc->central_mutex);                // Unlock
}

void *chunk_alloc_hook(size_t size, size_t align) {
	ChunkHead *ch = NULL;
	ThreadCache *tc = get_current_thread();
	uint16_t kind;
	uint16_t num = check_size(size, &kind);

	ch = get_suitable_chunk(tc, kind, num, align, NULL);
	ch->seek = size;
        
	return ch + 1;
}

void *chunk_realloc_hook(void *ptr, size_t size) {
	void *ret = NULL;
	ChunkHead *old_ch = ptr - chunk_head_size;
	ChunkHead *new_ch = NULL;
	ThreadCache *tc = get_current_thread();
	uint16_t kind;
	uint16_t num = check_size(size, &kind);
        
	new_ch = get_suitable_chunk(tc, kind, num, 0, old_ch);
	if (new_ch == old_ch) {
		ret = ptr;
	} else {
		ret = new_ch + 1;
		memcpy(ret, ptr, old_ch->seek);
		do_chunk_free(find_central_of_pointer(old_ch), old_ch);
	}

	new_ch->seek = size;
	return ret;
}

