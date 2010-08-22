# $FreeBSD: src/sys/powerpc/conf/Makefile,v 1.3 2010/08/19 03:19:26 nwhitehorn Exp $

TARGET=powerpc
.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpc64
TARGET_ARCH?=${MACHINE_ARCH}
.endif

.include "${.CURDIR}/../../conf/makeLINT.mk"
