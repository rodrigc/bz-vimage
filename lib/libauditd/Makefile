#
# $FreeBSD: src/lib/libauditd/Makefile,v 1.2 2010/01/02 09:58:07 ed Exp $
#

OPENBSMDIR=		${.CURDIR}/../../contrib/openbsm
LIBAUDITDDIR=		${OPENBSMDIR}/libauditd
LIBBSMDIR=		${OPENBSMDIR}/libbsm

LIB=		auditd

.PATH:		${LIBAUDITDDIR}

SRCS=	auditd_lib.c

#
# Must use BSM include files from within the contrib area, not the system.
#
CFLAGS+=	-I${OPENBSMDIR} -I${LIBBSMDIR}

WARNS?=		3

NO_MAN=

.include <bsd.lib.mk>
