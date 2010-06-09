# $FreeBSD: src/usr.sbin/cron/lib/Makefile,v 1.10 2008/11/18 00:59:26 matteo Exp $

LIB=	cron
INTERNALLIB=
SRCS= entry.c env.c misc.c

WARNS?=	3

CFLAGS+= -I${.CURDIR}/../cron
CFLAGS+= -DLOGIN_CAP -DPAM

.include <bsd.lib.mk>
