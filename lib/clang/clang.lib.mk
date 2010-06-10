# $FreeBSD: src/lib/clang/clang.lib.mk,v 1.1 2010/06/09 19:32:20 rdivacky Exp $

LLVM_SRCS=${.CURDIR}/../../../contrib/llvm

.include "clang.build.mk"

INTERNALLIB=

.include <bsd.lib.mk>
