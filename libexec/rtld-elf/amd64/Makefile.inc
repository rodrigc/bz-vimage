# $FreeBSD: src/libexec/rtld-elf/amd64/Makefile.inc,v 1.2 2010/08/23 22:24:11 imp Exp $

CFLAGS+=	-elf
LDFLAGS+=	-elf
# Uncomment this to build the dynamic linker as an executable instead
# of a shared library:
#LDSCRIPT=	${.CURDIR}/${MACHINE_CPUARCH}/elf_rtld.x
