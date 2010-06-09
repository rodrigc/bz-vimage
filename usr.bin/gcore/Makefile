#	@(#)Makefile	8.1 (Berkeley) 6/6/93
# $FreeBSD: src/usr.bin/gcore/Makefile,v 1.12 2010/02/25 20:24:19 ru Exp $

PROG=	gcore
SRCS=	elfcore.c gcore.c
DPADD=	${LIBUTIL}
LDADD=	-lutil

WARNS?=	1

.include <bsd.prog.mk>
