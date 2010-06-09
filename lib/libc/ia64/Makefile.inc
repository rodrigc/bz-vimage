# $FreeBSD: src/lib/libc/ia64/Makefile.inc,v 1.8 2007/12/03 07:17:32 das Exp $
#
# Machine dependent definitions for the ia64 architecture.
#

# Long double is 80 bits
GDTOASRCS+=strtorx.c
MDSRCS+=machdep_ldisx.c
SYM_MAPS+=${.CURDIR}/ia64/Symbol.map
