# $FreeBSD: src/tools/regression/posixsem/Makefile,v 1.1 2008/06/27 05:39:04 jhb Exp $

PROG=	posixsem
SRCS=	posixsem.c test.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
NO_MAN=

WARNS?=	3

.include <bsd.prog.mk>
