#	@(#)Makefile	8.1 (Berkeley) 6/6/93
# $FreeBSD: src/usr.bin/nfsstat/Makefile,v 1.4 2010/01/02 10:27:05 ed Exp $

PROG=	nfsstat
CFLAGS+=-DNFS
DPADD=  ${LIBKVM}
LDADD= -lkvm

WARNS?=	3

.include <bsd.prog.mk>
