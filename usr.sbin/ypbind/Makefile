#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
# $FreeBSD: src/usr.sbin/ypbind/Makefile,v 1.10 2010/01/02 11:07:44 ed Exp $

PROG=	ypbind
MAN=	ypbind.8
SRCS=	ypbind.c yp_ping.c

CFLAGS+= -DDAEMON

WARNS?=	2

.include <bsd.prog.mk>
