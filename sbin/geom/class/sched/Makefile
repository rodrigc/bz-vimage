# GEOM_LIBRARY_PATH
# $FreeBSD: src/sbin/geom/class/sched/Makefile,v 1.2 2010/04/13 09:52:42 luigi Exp $

.PATH: ${.CURDIR}/../../misc
#CFLAGS += -I/usr/src/sbin/geom

CLASS=sched

WARNS?= 6
CLASS_DIR?=/lib/geom

SHLIBDIR?=${CLASS_DIR}
SHLIB_NAME?=geom_${CLASS}.so
LINKS=  ${BINDIR}/geom ${BINDIR}/g${CLASS}
MAN=    g${CLASS}.8
SRCS+=  geom_${CLASS}.c subr.c

.include <bsd.lib.mk>
