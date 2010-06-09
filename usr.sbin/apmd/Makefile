# $FreeBSD: src/usr.sbin/apmd/Makefile,v 1.11 2010/05/19 08:52:51 uqs Exp $

PROG=	apmd
MAN=	apmd.8
MANSUBDIR= /i386
SRCS=	apmd.c apmdlex.l apmdparse.y y.tab.h

WARNS?=	3

DPADD=	${LIBL}
LDADD=	-ll

CFLAGS+= -I${.CURDIR}

test:
	./apmd -d -f etc/apmd.conf -n

.include <bsd.prog.mk>
