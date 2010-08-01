# $FreeBSD: src/lib/libc/powerpc64/Makefile.inc,v 1.1 2010/07/10 14:45:03 nwhitehorn Exp $

# Long double is 64-bits
MDSRCS+=machdep_ldisd.c
SYM_MAPS+=${.CURDIR}/powerpc64/Symbol.map
