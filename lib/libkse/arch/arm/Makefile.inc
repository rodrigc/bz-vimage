# $FreeBSD: src/lib/libkse/arch/arm/Makefile.inc,v 1.3 2007/10/09 13:42:22 obrien Exp $

.PATH:	${.CURDIR}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

CFLAGS+=-DSYSTEM_SCOPE_ONLY

SRCS+=	pthread_md.c context.S