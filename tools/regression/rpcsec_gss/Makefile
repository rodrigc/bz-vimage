# $FreeBSD: src/tools/regression/rpcsec_gss/Makefile,v 1.2 2010/02/25 14:42:26 ru Exp $

PROG=	rpctest
NO_MAN=
WARNS?=	6
DPADD=	${LIBRPCSEC_GSS}
LDADD=	-lrpcsec_gss
DEBUG_FLAGS= -g -O0

.include <bsd.prog.mk>
