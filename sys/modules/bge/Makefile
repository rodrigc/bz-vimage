# $FreeBSD: src/sys/modules/bge/Makefile,v 1.7 2010/08/23 06:13:29 imp Exp $

.PATH: ${.CURDIR}/../../dev/bge

KMOD=	if_bge
SRCS=	if_bge.c miibus_if.h miidevs.h device_if.h bus_if.h pci_if.h

.if ${MACHINE_CPUARCH} == sparc64
SRCS+=	ofw_bus_if.h
.endif

.include <bsd.kmod.mk>
