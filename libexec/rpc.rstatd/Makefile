# $FreeBSD: src/libexec/rpc.rstatd/Makefile,v 1.11 2010/02/26 09:41:16 ru Exp $

PROG =	rpc.rstatd
SRCS =	rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8

DPADD=	${LIBRPCSVC} ${LIBUTIL} ${LIBDEVSTAT} ${LIBKVM}
LDADD=	-lrpcsvc -lutil -ldevstat -lkvm

WARNS?=	1

.include <bsd.prog.mk>
