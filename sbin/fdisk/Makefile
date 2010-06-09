# $FreeBSD: src/sbin/fdisk/Makefile,v 1.13 2008/09/23 13:12:51 ru Exp $

PROG=	fdisk
SRCS=	fdisk.c geom_mbr_enc.c
WARNS?=	4
MAN=	fdisk.8

.PATH:	${.CURDIR}/../../sys/geom

DPADD+=	${LIBGEOM} ${LIBBSDXML} ${LIBSBUF}
LDADD+=	-lgeom -lbsdxml -lsbuf

.include <bsd.prog.mk>

test:	${PROG}
	sh ${.CURDIR}/runtest.sh
