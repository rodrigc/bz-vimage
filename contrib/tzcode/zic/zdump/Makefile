# $FreeBSD: src/contrib/tzcode/zic/zdump/Makefile,v 1.1 2010/02/25 06:53:46 edwin Exp $

.PATH: ${.CURDIR}/..

PROG=	zdump
MAN=	zdump.8
SRCS=	zdump.c ialloc.c scheck.c

CFLAGS+= -DTM_GMTOFF=tm_gmtoff -DTM_ZONE=tm_zone -DSTD_INSPIRED -DPCTS
CFLAGS+= -DHAVE_LONG_DOUBLE -DTZDIR=\"/usr/share/zoneinfo\" -Demkdir=mkdir
CFLAGS+= -I${.CURDIR}/.. -I${.CURDIR}/../../../lib/libc/stdtime

WARNS?=	2

.include <bsd.prog.mk>
