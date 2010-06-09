# $FreeBSD: src/usr.sbin/fdread/Makefile,v 1.6 2010/01/02 11:07:44 ed Exp $

PROG=	fdread
SRCS=	fdread.c fdutil.c

.if ${MACHINE} == "pc98"
CFLAGS+= -DPC98
.endif

.include <bsd.prog.mk>
