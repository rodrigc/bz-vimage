# $FreeBSD: src/lib/libkse/arch/arm/Makefile.inc,v 1.4 2010/06/13 01:27:29 imp Exp $

CFLAGS+=-DSYSTEM_SCOPE_ONLY

SRCS+=	pthread_md.c context.S
