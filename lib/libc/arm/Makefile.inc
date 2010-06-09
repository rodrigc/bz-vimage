# $FreeBSD: src/lib/libc/arm/Makefile.inc,v 1.5 2009/05/31 02:03:40 marcel Exp $
#
# Machine dependent definitions for the arm architecture.
#

SOFTFLOAT_BITS=32

# Long double is just double precision.
MDSRCS+=machdep_ldisd.c
SYM_MAPS+=${.CURDIR}/arm/Symbol.map
