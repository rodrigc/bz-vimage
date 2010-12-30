# $FreeBSD: src/lib/libblocksruntime/Makefile,v 1.1 2010/11/11 15:13:11 ed Exp $

LIB=	BlocksRuntime
SHLIB_MAJOR=0
CFLAGS+=-I${.CURDIR}
WARNS?=	2

.PATH: ${.CURDIR}/../../contrib/compiler-rt/BlocksRuntime

INCS=	Block.h Block_private.h
SRCS=	data.c runtime.c

.include <bsd.lib.mk>
