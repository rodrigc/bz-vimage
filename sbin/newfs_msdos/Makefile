# $FreeBSD: src/sbin/newfs_msdos/Makefile,v 1.13 2010/08/23 22:24:11 imp Exp $

PROG=	newfs_msdos
MAN=	newfs_msdos.8

# XXX - this is verboten
.if ${MACHINE_CPUARCH} == "arm"
WARNS?= 3
.endif

.include <bsd.prog.mk>
