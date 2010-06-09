# $FreeBSD: src/usr.sbin/yp_mkdb/Makefile,v 1.10 2010/01/02 11:07:44 ed Exp $

.PATH: ${.CURDIR}/../../libexec/ypxfr ${.CURDIR}/../ypserv

PROG=	yp_mkdb
MAN=	yp_mkdb.8
SRCS=	yp_mkdb.c yp_dblookup.c yp_dbwrite.c

CFLAGS+= -Dyp_error=warnx
CFLAGS+= -I${.CURDIR}/../../libexec/ypxfr -I${.CURDIR}/../../usr.sbin/ypserv

WARNS?=	1

.include <bsd.prog.mk>
