# $FreeBSD: src/lib/libc/amd64/Makefile.inc,v 1.6 2007/12/03 07:17:32 das Exp $
#
# Machine dependent definitions for the amd64 architecture.
#

# Long double is 80 bits
GDTOASRCS+=strtorx.c
MDSRCS+=machdep_ldisx.c
SYM_MAPS+=${.CURDIR}/amd64/Symbol.map
