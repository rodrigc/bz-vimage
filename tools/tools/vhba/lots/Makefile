# $FreeBSD: src/tools/tools/vhba/lots/Makefile,v 1.1 2010/06/08 22:26:47 mjacob Exp $
KMOD=	vlots
SRCS=	vhba_lots.c vhba.c
CFLAGS += -I${.CURDIR}/.. -DVHBA_MOD=\"vlots\"
VPATH=	${.CURDIR}/..

.include <bsd.kmod.mk>
