#
# $FreeBSD: src/usr.sbin/auditreduce/Makefile,v 1.3 2010/01/02 11:07:44 ed Exp $
#

OPENBSMDIR=${.CURDIR}/../../contrib/openbsm
.PATH: ${OPENBSMDIR}/bin/auditreduce

CFLAGS+=	-I${OPENBSMDIR}

PROG=	auditreduce
MAN=	auditreduce.1

DPADD=	${LIBBSM}
LDADD=	-lbsm

WARNS?=	3

.include <bsd.prog.mk>
