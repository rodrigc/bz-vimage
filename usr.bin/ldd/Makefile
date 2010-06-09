# $FreeBSD: src/usr.bin/ldd/Makefile,v 1.15 2010/01/02 10:27:05 ed Exp $

PROG?=		ldd
SRCS=		ldd.c
.if ${MACHINE_ARCH} == "i386"
SRCS+=		sods.c
.endif

.include <bsd.prog.mk>
