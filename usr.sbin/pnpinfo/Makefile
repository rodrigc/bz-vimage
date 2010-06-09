# $FreeBSD: src/usr.sbin/pnpinfo/Makefile,v 1.13 2010/01/02 11:07:44 ed Exp $

.PATH: ${.CURDIR}/../../contrib/pnpinfo

PROG=	pnpinfo
MAN=	pnpinfo.8

CFLAGS+= -I${.CURDIR}/../../sys

.if ${MACHINE} == "pc98"
CFLAGS+= -DPC98
.endif

WARNS?=	2

.include <bsd.prog.mk>
