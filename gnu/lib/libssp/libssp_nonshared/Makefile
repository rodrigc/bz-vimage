# $FreeBSD: src/gnu/lib/libssp/libssp_nonshared/Makefile,v 1.4 2009/07/14 21:19:13 kan Exp $

GCCDIR=	${.CURDIR}/../../../../contrib/gcc
GCCLIB=	${.CURDIR}/../../../../contrib/gcclibs

.PATH: ${GCCLIB}/libssp ${GCCLIB}/libssp/ssp

LIB=	ssp_nonshared
NO_PIC=
NO_PROFILE=

SRCS=	ssp-local.c

CFLAGS+= -DHAVE_CONFIG_H
CFLAGS+= -I${.CURDIR}/..  -I${GCCLIB}/libssp  -I${GCCLIB}/include
CFLAGS+= -fPIC -DPIC -fvisibility=hidden

.include <bsd.lib.mk>
