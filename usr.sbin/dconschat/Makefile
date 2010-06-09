# $FreeBSD: src/usr.sbin/dconschat/Makefile,v 1.3 2010/01/02 11:07:44 ed Exp $

PROG=	dconschat
MAN=	dconschat.8

CFLAGS+= -I${.CURDIR}/../../sys

DPADD=  ${LIBKVM}
LDADD=  -lkvm

WARNS?=	1

.include <bsd.prog.mk>
