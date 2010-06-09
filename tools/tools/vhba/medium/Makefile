# $FreeBSD: src/tools/tools/vhba/medium/Makefile,v 1.1 2010/06/08 22:26:47 mjacob Exp $
KMOD=	vmedium
SRCS=	vhba_medium.c vhba.c
CFLAGS += -I${.CURDIR}/.. -DVHBA_MOD=\"vmedium\"
VPATH=	${.CURDIR}/..

.include <bsd.kmod.mk>
