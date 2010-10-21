# $FreeBSD: src/sys/modules/cryptodev/Makefile,v 1.3 2010/10/13 14:44:38 rpaulo Exp $

.PATH:	${.CURDIR}/../../opencrypto
KMOD	= cryptodev
SRCS	= cryptodev.c
SRCS	+= bus_if.h device_if.h opt_compat.h

.include <bsd.kmod.mk>
