# $FreeBSD: src/sbin/geom/class/journal/Makefile,v 1.3 2010/12/15 23:24:34 obrien Exp $

.PATH: ${.CURDIR}/../../misc

GEOM_CLASS=	journal
SRCS+=	geom_journal_ufs.c

DPADD=	${LIBMD} ${LIBUFS}
LDADD=	-lmd -lufs

CFLAGS+=-I${.CURDIR}/../../../../sys

.include <bsd.lib.mk>
