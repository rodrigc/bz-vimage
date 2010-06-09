# $FreeBSD: src/cddl/lib/libavl/Makefile,v 1.4 2010/03/02 19:04:07 uqs Exp $

.PATH: ${.CURDIR}/../../../sys/cddl/contrib/opensolaris/common/avl

LIB=	avl
SRCS=	avl.c
WARNS?=	0
CFLAGS+= -I${.CURDIR}/../../../sys/cddl/compat/opensolaris
CFLAGS+= -I${.CURDIR}/../../../sys/cddl/contrib/opensolaris/uts/common

.include <bsd.lib.mk>
