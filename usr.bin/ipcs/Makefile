# $FreeBSD: src/usr.bin/ipcs/Makefile,v 1.9 2010/01/02 10:27:05 ed Exp $

PROG=	ipcs
SRCS=	ipcs.c ipc.c
DPADD=	${LIBKVM}
LDADD=	-lkvm

WARNS?=	1

.include <bsd.prog.mk>
