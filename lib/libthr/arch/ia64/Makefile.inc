# $FreeBSD: src/lib/libthr/arch/ia64/Makefile.inc,v 1.4 2009/10/24 20:07:17 marcel Exp $

.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

SRCS+= _umtx_op_err.S pthread_md.c
