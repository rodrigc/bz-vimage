# $FreeBSD: src/usr.sbin/jexec/Makefile,v 1.6 2010/01/02 11:07:44 ed Exp $

PROG=	jexec
MAN=	jexec.8
DPADD=	${LIBJAIL} ${LIBKVM} ${LIBUTIL}
LDADD=	-ljail -lkvm -lutil

.include <bsd.prog.mk>
