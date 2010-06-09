# $FreeBSD: src/sbin/natd/Makefile,v 1.10 2010/01/17 21:56:27 ed Exp $

PROG		= natd
SRCS		= natd.c icmp.c
WARNS?=	3
LDADD		= -lalias
DPADD		= ${LIBALIAS}
MAN		= natd.8

.include <bsd.prog.mk>
