/* $FreeBSD: src/tools/regression/pthread/unwind/thread_normal_exit.cpp,v 1.1 2010/09/25 04:26:40 davidxu Exp $ */
/* test stack unwinding for a new thread */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "Test.cpp"

void *
thr_routine(void *arg)
{
	Test test;

	pthread_exit(NULL);
	printf("Bug, thread shouldn't be here\n");
}

int
main()
{
	pthread_t td;

	pthread_create(&td, NULL, thr_routine, NULL);
	pthread_join(td, NULL);
	check_destruct();
	return (0);
}
