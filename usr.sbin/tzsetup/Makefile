# $FreeBSD: src/usr.sbin/tzsetup/Makefile,v 1.18 2010/01/02 11:07:44 ed Exp $

PROG=	tzsetup
MAN=	tzsetup.8

CFLAGS+= -I${.CURDIR}

WARNS?=	3

DPADD=	${LIBDIALOG} ${LIBNCURSES}
LDADD=	-ldialog -lncurses

.include <bsd.prog.mk>
