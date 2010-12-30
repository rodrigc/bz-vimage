# $FreeBSD: src/sbin/pflogd/Makefile,v 1.7 2010/11/05 11:31:39 rpaulo Exp $

.PATH:	${.CURDIR}/../../contrib/pf/pflogd

PROG=	pflogd
SRCS= 	pflogd.c pidfile.c privsep.c privsep_fdpass.c
MAN=	pflogd.8

CFLAGS+=-include ${.CURDIR}/../../lib/libpcap/config.h

LDADD=	-lpcap -lutil
DPADD=	${LIBPCAP} ${LIBUTIL}

WARNS?=	2

.include <bsd.prog.mk>
