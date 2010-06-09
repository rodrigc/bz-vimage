# $FreeBSD: src/sbin/ddb/Makefile,v 1.3 2010/03/02 16:58:04 uqs Exp $

PROG=	ddb
SRCS=	ddb.c ddb_capture.c ddb_script.c
MAN=	ddb.8
WARNS?=	3

DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
