#	$NetBSD: Makefile.inc,v 1.7 2005/09/17 11:49:39 tsutsui Exp $
# $FreeBSD: src/lib/libc/mips/Makefile.inc,v 1.3 2008/04/26 12:07:59 imp Exp $

SOFTFLOAT_BITS=32

CFLAGS+=-DSOFTFLOAT

MDSRCS+= machdep_ldisd.c
SYM_MAPS+= ${.CURDIR}/mips/Symbol.map
