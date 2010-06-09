# $FreeBSD: src/libexec/rpc.sprayd/Makefile,v 1.7 2010/01/02 09:50:19 ed Exp $

PROG =	rpc.sprayd
SRCS =	sprayd.c
MAN =	rpc.sprayd.8

DPADD=	${LIBRPCSVC}
LDADD=	-lrpcsvc

WARNS?=	2

.include <bsd.prog.mk>
