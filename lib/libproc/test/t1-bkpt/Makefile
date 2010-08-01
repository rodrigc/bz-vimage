# $FreeBSD: src/lib/libproc/test/t1-bkpt/Makefile,v 1.1 2010/07/31 16:10:20 rpaulo Exp $

PROG=	t1-bkpt

SRCS=	t1-bkpt.c

LDADD=	-lproc -lelf -lrtld_db -lutil
DPADD=	${LIBPROC} ${LIBELF}

WITHOUT_MAN=

.include <bsd.prog.mk>
