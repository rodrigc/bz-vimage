# $FreeBSD: src/tools/tools/vhba/faulty/Makefile,v 1.1 2010/06/08 22:26:47 mjacob Exp $
KMOD=	vfaulty
SRCS=	vhba_faulty.c vhba.c
CFLAGS += -I${.CURDIR}/.. -DVHBA_MOD=\"vfaulty\"
VPATH=	${.CURDIR}/..

.include <bsd.kmod.mk>
