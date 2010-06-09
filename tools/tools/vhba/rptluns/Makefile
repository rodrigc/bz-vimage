# $FreeBSD: src/tools/tools/vhba/rptluns/Makefile,v 1.1 2010/06/08 22:26:47 mjacob Exp $
KMOD=	vrptluns
SRCS=	vhba_rptluns.c vhba.c
CFLAGS += -I${.CURDIR}/.. -DVHBA_MOD=\"vrptluns\"
VPATH=	${.CURDIR}/..

.include <bsd.kmod.mk>
