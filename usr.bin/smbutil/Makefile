# $FreeBSD: src/usr.bin/smbutil/Makefile,v 1.3 2010/01/02 10:27:05 ed Exp $

PROG=	smbutil
SRCS=	smbutil.c dumptree.c login.c lookup.c view.c print.c

DPADD=	${LIBSMB} ${LIBKICONV}
LDADD=	-lsmb -lkiconv

CONTRIBDIR=	${.CURDIR}/../../contrib/smbfs
CFLAGS+=	-I${CONTRIBDIR}/include

WARNS?=	0

.PATH:	${CONTRIBDIR}/smbutil

.include <bsd.prog.mk>
