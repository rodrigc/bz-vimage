# $FreeBSD: src/cddl/usr.sbin/Makefile,v 1.12 2010/08/21 14:02:04 rpaulo Exp $

.include <bsd.own.mk>

SUBDIR=	${_dtrace} \
	${_lockstat} \
	${_zdb}

.if ${MK_ZFS} != "no"
.if ${MK_LIBTHR} != "no"
_zdb=	zdb
.endif
.endif

.if ${MACHINE_ARCH} == "amd64" || ${MACHINE_ARCH} == "i386"
_dtrace=	dtrace
_lockstat=	lockstat
.endif

.include <bsd.subdir.mk>
