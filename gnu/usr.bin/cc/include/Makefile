# $FreeBSD: src/gnu/usr.bin/cc/include/Makefile,v 1.11 2010/07/10 02:29:22 nwhitehorn Exp $

.include "../Makefile.inc"

INCSDIR=${INCLUDEDIR}/gcc/${GCCVER}

.PATH: ${GCCDIR}/config/${GCC_CPU}

.if ${TARGET_ARCH} == "i386" || ${TARGET_ARCH} == "amd64"
INCS=	emmintrin.h mmintrin.h pmmintrin.h xmmintrin.h mm_malloc.h
.elif ${TARGET_ARCH} == "ia64"
INCS=	ia64intrin.h
.elif ${TARGET_ARCH} == "arm"
INCS=	mmintrin.h
.elif ${TARGET_ARCH} == "powerpc" || ${TARGET_ARCH} == "powerpc64"
INCS=	ppc-asm.h altivec.h spe.h
.endif

mm_malloc.h: pmm_malloc.h
	@rm -rf ${.TARGET}
	@cp ${.ALLSRC} ${.TARGET}
CLEANFILES+=	mm_malloc.h

.include <bsd.init.mk>
.include <bsd.incs.mk>
.include <bsd.obj.mk>
