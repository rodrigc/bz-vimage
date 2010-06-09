# $FreeBSD: src/tools/regression/kgssapi/Makefile,v 1.2 2010/02/25 14:42:26 ru Exp $

PROG=	gsstest
NO_MAN=
WARNS?=	2
DPADD=	${LIBGSSAPI} ${LIBGSSAPI_KRB5}
LDADD=	-lgssapi -lgssapi_krb5
DEBUG_FLAGS= -g -O0

.include <bsd.prog.mk>
