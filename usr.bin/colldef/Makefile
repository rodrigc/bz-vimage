# $FreeBSD: src/usr.bin/colldef/Makefile,v 1.22 2010/01/02 10:27:05 ed Exp $

PROG=	colldef
SRCS=	parse.y scan.l y.tab.h
LFLAGS=	-8 -i
CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/../../lib/libc/locale
CFLAGS+=-DCOLLATE_DEBUG -DYY_NO_UNPUT
LDADD=	-ll
DPADD=	${LIBL}

WARNS?=	2

.include <bsd.prog.mk>
