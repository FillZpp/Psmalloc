/*
 * Copyright (C) 2014 FillZpp
 */


#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#ifdef __linux__
#include <malloc.h>
#endif


void func(void)
{
	const int num = 20;
	int i = 0;
	void *p1[num];
	void *p2[num];

	for (i=0; i<num; ++i)
		p1[i] = malloc(i*i*100 + 100);
	for (i=0; i<num; ++i)
		p1[i] = realloc(p1[i], i*i*100 + 200);
	for (i=0; i<num; ++i)
		p2[i] = malloc(i*i*100 + 100);
	for (i=0; i<num; ++i)
		free(p1[i]);
	for (i=0; i<num; ++i)
		p2[i] = realloc(p2[i], i*i*100 + 60);
	for (i=0; i<num; ++i)
		free(p2[i]);
        
	pthread_exit(0);
}

int main(int argc, char *argv[])
{
	int i, j;
	clock_t cl;
	double call_time = 0;
	int num = atoi(argv[1]);
	pthread_t tid[num];

	for (j=0; j<5; ++j) {
		cl = clock();

		for (i=0; i<num; ++i)
			pthread_create(&tid[i], NULL, (void*)func, NULL);

		for (i=0; i<num; ++i)
			pthread_join(tid[i], NULL);

		call_time += (clock() - cl) * 1000
			/CLOCKS_PER_SEC;
	}
	printf("time: %.2lf ms\n", call_time/5);
	printf("heap top: %p\n", sbrk(0));

	return 0;
}
