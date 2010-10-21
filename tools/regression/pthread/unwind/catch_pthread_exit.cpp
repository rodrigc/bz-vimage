/* $FreeBSD: src/tools/regression/pthread/unwind/catch_pthread_exit.cpp,v 1.1 2010/09/25 04:26:40 davidxu Exp $ */
/* try to catch thread exiting, and rethrow the exception */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int caught;

void *
thr_routine(void *arg)
{
	try {
		pthread_exit(NULL);
	} catch (...) {
		caught = 1;
		printf("thread exiting exception caught\n");
		/* rethrow */
		throw;
	}
}

int
main()
{
	pthread_t td;

	pthread_create(&td, NULL, thr_routine, NULL);
	pthread_join(td, NULL);
	if (caught)
		printf("OK\n");
	else
		printf("failure\n");
	return (0);
}
