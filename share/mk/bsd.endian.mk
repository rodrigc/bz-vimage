# $FreeBSD: src/share/mk/bsd.endian.mk,v 1.6 2010/07/10 02:32:50 nwhitehorn Exp $

.if ${MACHINE_ARCH} == "amd64" || \
    ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "ia64" || \
    (${MACHINE_ARCH} == "arm" && !defined(TARGET_BIG_ENDIAN)) || \
    (${MACHINE_ARCH} == "mips" && !defined(TARGET_BIG_ENDIAN))
TARGET_ENDIANNESS= 1234
.elif ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "powerpc64" || \
    ${MACHINE_ARCH} == "sparc64" || \
    ${MACHINE_ARCH} == "arm" || \
    ${MACHINE_ARCH} == "mips"
TARGET_ENDIANNESS= 4321
.endif
