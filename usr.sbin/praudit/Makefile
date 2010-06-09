#
# $FreeBSD: src/usr.sbin/praudit/Makefile,v 1.2 2010/01/02 11:07:44 ed Exp $
#

OPENBSMDIR=${.CURDIR}/../../contrib/openbsm
.PATH: ${OPENBSMDIR}/bin/praudit

PROG=	praudit
MAN=	praudit.1

WARNS?=	3

DPADD=	${LIBBSM}
LDADD=	-lbsm

.include <bsd.prog.mk>
