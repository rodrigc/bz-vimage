# $FreeBSD: src/sys/modules/io/Makefile,v 1.3 2010/08/23 06:13:29 imp Exp $

.PATH: ${.CURDIR}/../../dev/io
.PATH: ${.CURDIR}/../../${MACHINE_CPUARCH}/${MACHINE_CPUARCH}

KMOD=	io
SRCS=	iodev.c io.c
SRCS+=	bus_if.h device_if.h

.include <bsd.kmod.mk>
