# $FreeBSD: src/usr.bin/ncal/Makefile,v 1.12 2010/03/04 12:18:24 edwin Exp $

PROG=	ncal

DPADD=	${LIBCALENDAR} ${LIBTERMCAP}
LDADD=	-lcalendar -ltermcap

LINKS=	${BINDIR}/ncal ${BINDIR}/cal
MLINKS=	ncal.1 cal.1

.include <bsd.prog.mk>
