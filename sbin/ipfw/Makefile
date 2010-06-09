# $FreeBSD: src/sbin/ipfw/Makefile,v 1.24 2010/03/08 14:43:55 luigi Exp $

PROG=	ipfw
SRCS=	ipfw2.c dummynet.c ipv6.c main.c nat.c altq.c
WARNS?=	2
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN=	ipfw.8

.include <bsd.prog.mk>
