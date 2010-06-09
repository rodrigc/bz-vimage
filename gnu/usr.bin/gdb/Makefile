# $FreeBSD: src/gnu/usr.bin/gdb/Makefile,v 1.17 2010/04/12 23:35:58 imp Exp $

SUBDIR=	doc libgdb gdb gdbtui kgdb

TARGET_ARCH?= ${MACHINE_ARCH}
.if exists(${.CURDIR}/gdbserver/reg-${TARGET_ARCH}.c)
SUBDIR+=gdbserver
.endif

.include <bsd.subdir.mk>
