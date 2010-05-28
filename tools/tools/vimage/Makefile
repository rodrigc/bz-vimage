# $FreeBSD: src/tools/tools/vimage/Makefile,v 1.3 2009/08/20 22:39:20 zec Exp $

PROG=	vimage
LDADD=	-ljail -lkvm
DPADD=	${LIBJAIL} ${LIBKVM}

WARNS?=	2
CFLAGS+= -I../../../sys

MAN=	vimage.8

BINDIR?=	/usr/sbin

.include <bsd.prog.mk>
