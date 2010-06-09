# $FreeBSD: src/libexec/rpc.rquotad/Makefile,v 1.9 2010/05/07 00:41:12 mckusick Exp $

PROG =	rpc.rquotad
SRCS =	rquotad.c
MAN  =	rpc.rquotad.8

DPADD=	${LIBRPCSVC} ${LIBUTIL}
LDADD=	-lrpcsvc -lutil

.include <bsd.prog.mk>
