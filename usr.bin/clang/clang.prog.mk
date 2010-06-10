# $FreeBSD: src/usr.bin/clang/clang.prog.mk,v 1.1 2010/06/09 19:32:20 rdivacky Exp $

LLVM_SRCS=${.CURDIR}/../../../contrib/llvm

.include "../../lib/clang/clang.build.mk"

.for lib in ${LIBDEPS}
DPADD+= ${.OBJDIR}/../../../lib/clang/lib${lib}/lib${lib}.a
LDADD+= ${.OBJDIR}/../../../lib/clang/lib${lib}/lib${lib}.a
.endfor

BINDIR?=/usr/bin

.include <bsd.prog.mk>
