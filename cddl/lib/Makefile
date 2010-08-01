# $FreeBSD: src/cddl/lib/Makefile,v 1.10 2010/07/31 17:13:14 rpaulo Exp $

.include <bsd.own.mk>

SUBDIR=	drti \
	libavl \
	libctf \
	${_libdtrace} \
	libnvpair \
	libumem \
	libuutil \
	${_libzfs} \
	${_libzpool}

.if ${MK_ZFS} != "no"
_libzfs=	libzfs
.if ${MK_LIBTHR} != "no"
_libzpool=	libzpool
.endif
.endif

.if ${MACHINE_ARCH} == amd64 || ${MACHINE_ARCH} == i386
_libdtrace=	libdtrace
.endif

.include <bsd.subdir.mk>
