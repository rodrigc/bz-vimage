# $FreeBSD: src/usr.sbin/pkg_install/create/Makefile,v 1.23 2010/04/23 11:07:43 flz Exp $

PROG=	pkg_create
SRCS=	main.c perform.c pl.c

WARNS?=	3
WFORMAT?=	1

DPADD=	${LIBMD}
LDADD=	-lmd

.include <bsd.prog.mk>
