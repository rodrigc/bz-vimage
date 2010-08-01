# $FreeBSD: src/lib/libproc/test/t2-name2map/Makefile,v 1.1 2010/07/31 16:10:20 rpaulo Exp $

PROG=	t2-name2map

SRCS=	t2-name2map.c

LDADD=	-lproc -lelf -lrtld_db -lutil
DPADD=	${LIBPROC} ${LIBELF}

WITHOUT_MAN=

.include <bsd.prog.mk>
