# $FreeBSD: src/sbin/geom/class/Makefile.inc,v 1.8 2010/12/15 23:24:34 obrien Exp $

SHLIBDIR?=${GEOM_CLASS_DIR}
SHLIB_NAME?=geom_${GEOM_CLASS}.so
LINKS=	${BINDIR}/geom ${BINDIR}/g${GEOM_CLASS}
MAN=	g${GEOM_CLASS}.8
SRCS+=	geom_${GEOM_CLASS}.c subr.c

CFLAGS+= -I${.CURDIR}/../..

.include "../Makefile.inc"
