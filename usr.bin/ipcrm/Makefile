# $FreeBSD: src/usr.bin/ipcrm/Makefile,v 1.8 2010/01/02 10:27:05 ed Exp $

PROG=	ipcrm
SRCS=	ipcrm.c ipc.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
CFLAGS+=-I${.CURDIR}/../ipcs
.PATH:  ${.CURDIR}/../ipcs

WARNS?=	0

.include <bsd.prog.mk>
