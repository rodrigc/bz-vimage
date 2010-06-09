# Make `routed` for FreeBSD
# $FreeBSD: src/sbin/routed/Makefile,v 1.20 2010/02/27 10:17:27 uqs Exp $

PROG=	routed
MAN=	routed.8
SRCS=	if.c input.c main.c output.c parms.c radix.c rdisc.c table.c trace.c
WARNS?=	3
DPADD=	${LIBMD}
LDADD=	-lmd

SUBDIR= rtquery

.include <bsd.prog.mk>
