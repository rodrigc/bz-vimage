#$FreeBSD: src/lib/libthr/arch/amd64/Makefile.inc,v 1.4 2008/04/02 07:41:23 davidxu Exp $

.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/${MACHINE_ARCH}

SRCS+=	pthread_md.c _umtx_op_err.S
