# $FreeBSD: src/usr.sbin/jail/Makefile,v 1.13 2010/01/02 11:07:44 ed Exp $

.include <bsd.own.mk>

PROG=	jail
MAN=	jail.8
DPADD=	${LIBJAIL} ${LIBUTIL}
LDADD=	-ljail -lutil

.if ${MK_INET6_SUPPORT} != "no"
CFLAGS+= -DINET6
.endif

.include <bsd.prog.mk>
