# $FreeBSD: src/lib/libc_r/sys/Makefile.inc,v 1.11 2010/08/23 22:24:11 imp Exp $

.PATH:	 ${.CURDIR}/sys ${.CURDIR}/arch/${MACHINE_CPUARCH}

SRCS+=	uthread_error.c _atomic_lock.S

