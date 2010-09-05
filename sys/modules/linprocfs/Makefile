# $FreeBSD: src/sys/modules/linprocfs/Makefile,v 1.18 2010/08/23 06:13:29 imp Exp $

.PATH: ${.CURDIR}/../../compat/linprocfs

KMOD=	linprocfs
SRCS=	vnode_if.h \
	device_if.h bus_if.h \
	linprocfs.c \
	opt_compat.h

.if ${MACHINE_CPUARCH} == "amd64"
CFLAGS+=-DCOMPAT_LINUX32
.endif

.include <bsd.kmod.mk>
