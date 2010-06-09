# $FreeBSD: src/usr.bin/nc/Makefile,v 1.4 2010/01/02 10:27:05 ed Exp $

.PATH: ${.CURDIR}/../../contrib/netcat

PROG=	nc
SRCS=	netcat.c atomicio.c socks.c

CFLAGS+=-DIPSEC
LDADD=	-lipsec
DPADD=	${LIBIPSEC}

WARNS?=	1

.include <bsd.prog.mk>
