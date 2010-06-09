# $FreeBSD: src/usr.sbin/kernbb/Makefile,v 1.11 2010/01/02 11:07:44 ed Exp $

PROG=	kernbb
MAN=	kernbb.8

DPADD=	${LIBKVM}
LDADD=	-lkvm

CFLAGS+= -I${.CURDIR}/../../contrib/gcc

.include <bsd.prog.mk>

