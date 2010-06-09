# $FreeBSD: src/lib/librt/Makefile,v 1.8 2010/02/25 20:24:19 ru Exp $

LIB=rt
SHLIB_MAJOR= 1
CFLAGS+=-I${.CURDIR}/../libc/include -I${.CURDIR}
CFLAGS+=-Winline -Wall -g
DPADD=	${LIBPTHREAD}
LDADD=	-lpthread

WARNS?=	2

#MAN=	libthr.3

SRCS+= aio.c mq.c sigev_thread.c timer.c

PRECIOUSLIB=

VERSION_MAP=	${.CURDIR}/Version.map

.include <bsd.lib.mk>
