# $FreeBSD: src/usr.bin/ldd/Makefile,v 1.16 2010/08/23 22:24:11 imp Exp $

PROG?=		ldd
SRCS=		ldd.c
.if ${MACHINE_CPUARCH} == "i386"
SRCS+=		sods.c
.endif

.include <bsd.prog.mk>
