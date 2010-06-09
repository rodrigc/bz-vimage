# $FreeBSD: src/usr.bin/minigzip/Makefile,v 1.6 2010/03/22 21:19:17 delphij Exp $

SRCDIR=	${.CURDIR}/../../lib/libz
.PATH:	${SRCDIR}

PROG=	minigzip

WARNS?=	5
CFLAGS+=-DUSE_MMAP
DPADD=	${LIBZ}
LDADD=	-lz

.include <bsd.prog.mk>
