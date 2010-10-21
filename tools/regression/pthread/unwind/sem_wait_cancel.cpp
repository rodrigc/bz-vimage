/* $FreeBSD: src/tools/regression/pthread/unwind/sem_wait_cancel.cpp,v 1.1 2010/09/25 04:26:40 davidxu Exp $ */
/* Test stack unwinding for libc's sem */

#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>

#include "Test.cpp"

sem_t sem;

void *
thr(void *arg)
{
	Test t;

	sem_wait(&sem);
	printf("Bug, thread shouldn't be here.\n");
	return (0);
}

int
main()
{
	pthread_t td;

	sem_init(&sem, 0, 0);
	pthread_create(&td, NULL, thr, NULL);
	sleep(1);
	pthread_cancel(td);
	pthread_join(td, NULL);
	check_destruct();
	return (0);
}
