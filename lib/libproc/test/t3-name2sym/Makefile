# $FreeBSD: src/lib/libproc/test/t3-name2sym/Makefile,v 1.1 2010/07/31 16:10:20 rpaulo Exp $

PROG=	t3-name2sym

SRCS=	t3-name2sym.c

LDADD=	-lproc -lelf -lrtld_db -lutil
DPADD=	${LIBPROC} ${LIBELF}

WITHOUT_MAN=

.include <bsd.prog.mk>
