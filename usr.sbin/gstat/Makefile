# $FreeBSD: src/usr.sbin/gstat/Makefile,v 1.9 2010/01/02 11:07:44 ed Exp $

PROG=	gstat
MAN=	gstat.8
DPADD=	${LIBDEVSTAT} ${LIBKVM} ${LIBGEOM} ${LIBBSDXML} ${LIBSBUF} ${LIBEDIT} ${LIBCURSES}
LDADD=	-ldevstat -lkvm -lgeom -lbsdxml -lsbuf -ledit -lcurses

.include <bsd.prog.mk>
