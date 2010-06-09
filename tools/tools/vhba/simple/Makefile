# $FreeBSD: src/tools/tools/vhba/simple/Makefile,v 1.1 2010/06/08 22:26:47 mjacob Exp $
KMOD=	vsimple
SRCS=	vhba_simple.c vhba.c
CFLAGS += -I${.CURDIR}/.. -DVHBA_MOD=\"vsimple\"
VPATH=	${.CURDIR}/..

.include <bsd.kmod.mk>
