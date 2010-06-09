#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.3 2010/01/02 11:07:44 ed Exp $
#

OPENBSMDIR=${.CURDIR}/../../contrib/openbsm
.PATH: ${OPENBSMDIR}/bin/audit

CFLAGS+=	-I${OPENBSMDIR}

PROG=	audit
MAN=	audit.8

DPADD=	${LIBBSM}
LDADD=	-lbsm

WARNS?=	5

.include <bsd.prog.mk>
