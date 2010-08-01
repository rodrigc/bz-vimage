# $FreeBSD: src/lib/libproc/Makefile,v 1.7 2010/07/31 17:14:54 rpaulo Exp $

LIB=	proc

SRCS=	proc_bkpt.c		\
	proc_create.c		\
	proc_regs.c		\
	proc_sym.c		\
	proc_rtld.c		\
	proc_util.c

INCS=	libproc.h

CFLAGS+=	-I${.CURDIR}

SHLIB_MAJOR=	2

WITHOUT_MAN=

.include <bsd.lib.mk>
