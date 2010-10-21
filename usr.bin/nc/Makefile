# $FreeBSD: src/usr.bin/nc/Makefile,v 1.5 2010/10/19 00:01:53 delphij Exp $

.PATH: ${.CURDIR}/../../contrib/netcat

PROG=	nc
SRCS=	netcat.c atomicio.c socks.c

CFLAGS+=-DIPSEC
LDADD=	-lipsec
DPADD=	${LIBIPSEC}

WARNS?=	2

.include <bsd.prog.mk>
