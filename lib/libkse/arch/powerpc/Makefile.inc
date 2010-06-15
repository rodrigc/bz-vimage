# $FreeBSD: src/lib/libkse/arch/powerpc/Makefile.inc,v 1.3 2010/06/13 01:27:29 imp Exp $

# XXX temporary
CFLAGS+=-DSYSTEM_SCOPE_ONLY

SRCS+=	enter_uts.S context.S pthread_md.c
