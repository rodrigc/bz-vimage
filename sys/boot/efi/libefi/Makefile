# $FreeBSD: src/sys/boot/efi/libefi/Makefile,v 1.19 2010/08/23 01:48:07 imp Exp $

LIB=	efi
INTERNALLIB=

SRCS=	delay.c efi_console.c efinet.c efipart.c errno.c handles.c \
	libefi.c time.c

CFLAGS+= -I${.CURDIR}/../include
CFLAGS+= -I${.CURDIR}/../include/${MACHINE_CPUARCH:S/amd64/i386/}
CFLAGS+= -I${.CURDIR}/../../../../lib/libstand

# Pick up the bootstrap header for some interface items
CFLAGS+= -I${.CURDIR}/../../common

.include <bsd.lib.mk>
