# $FreeBSD: src/usr.sbin/pppctl/Makefile,v 1.17 2010/01/02 11:07:44 ed Exp $

PROG=	pppctl
MAN=	pppctl.8

WARNS?=	2

DPADD=	${LIBPTHREAD} ${LIBEDIT} ${LIBTERMCAP}
LDADD=	-lpthread -ledit -ltermcap

.include <bsd.prog.mk>
