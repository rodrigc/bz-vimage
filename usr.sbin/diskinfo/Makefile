# $FreeBSD: src/usr.sbin/diskinfo/Makefile,v 1.4 2010/01/02 11:07:44 ed Exp $

PROG=	diskinfo
MAN=	diskinfo.8

DPADD=	${LIBUTIL}
LDADD=	-lutil

.include <bsd.prog.mk>

test:	${PROG}
	./${PROG} /dev/ad4 md50
	./${PROG} -v /dev/ad4 md50
	./${PROG} -t /dev/ad4
