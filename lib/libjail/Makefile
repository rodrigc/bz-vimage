# $FreeBSD: src/lib/libjail/Makefile,v 1.2 2010/01/02 09:58:07 ed Exp $

.include <bsd.own.mk>

LIB=	jail
SHLIBDIR?= /lib
SHLIB_MAJOR= 1
SRCS=	jail.c jail_getid.c
INCS=	jail.h

MAN=	jail.3

MLINKS+=jail.3 jail_getid.3
MLINKS+=jail.3 jail_getname.3
MLINKS+=jail.3 jail_getv.3
MLINKS+=jail.3 jail_setv.3
MLINKS+=jail.3 jailparam.3
MLINKS+=jail.3 jailparam_all.3
MLINKS+=jail.3 jailparam_init.3
MLINKS+=jail.3 jailparam_import.3
MLINKS+=jail.3 jailparam_import_raw.3
MLINKS+=jail.3 jailparam_get.3
MLINKS+=jail.3 jailparam_set.3
MLINKS+=jail.3 jailparam_export.3
MLINKS+=jail.3 jailparam_free.3

CFLAGS+=-I${.CURDIR} -DINET
.if ${MK_INET6} != "no"
CFLAGS+= -DINET6
.endif

.include <bsd.lib.mk>
