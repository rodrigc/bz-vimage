# $FreeBSD: src/gnu/usr.bin/gdb/Makefile,v 1.18 2010/08/23 22:24:11 imp Exp $

SUBDIR=	doc libgdb gdb gdbtui kgdb

.if exists(${.CURDIR}/gdbserver/reg-${MACHINE_CPUARCH}.c)
SUBDIR+=gdbserver
.endif

.include <bsd.subdir.mk>
