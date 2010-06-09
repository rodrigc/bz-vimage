# $FreeBSD: src/cddl/usr.sbin/Makefile,v 1.10 2009/05/27 01:30:23 sson Exp $

.include <bsd.own.mk>

SUBDIR=	dtrace \
	lockstat \
	${_zdb}

.if ${MK_ZFS} != "no"
.if ${MK_LIBTHR} != "no"
_zdb=	zdb
.endif
.endif

.include <bsd.subdir.mk>
