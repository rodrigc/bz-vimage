# $FreeBSD: src/tools/regression/tls/ttls1/Makefile,v 1.5 2010/02/25 14:42:26 ru Exp $

PROG=	ttls1
LDFLAGS+= -L../libxx -lxx -Wl,--rpath=${.OBJDIR}/../libxx
LDFLAGS+= -L../libyy -lyy -Wl,--rpath=${.OBJDIR}/../libyy
NO_MAN=
DEBUG_FLAGS= -g

.include <bsd.prog.mk>
