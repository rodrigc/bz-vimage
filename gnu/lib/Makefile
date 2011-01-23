# $FreeBSD: src/gnu/lib/Makefile,v 1.44 2011/01/12 14:55:02 nwhitehorn Exp $

.include <bsd.own.mk>

SUBDIR= csu libgcc libgcov libdialog libgomp libodialog libregex libreadline \
	libssp

# libsupc++ uses libstdc++ headers, although 'make includes' should
# have taken care of that already.
.if ${MK_CXX} != "no"
SUBDIR+= libstdc++ libsupc++
.endif

.if ${MK_OBJC} != "no"
SUBDIR+= libobjc
.endif

.include <bsd.subdir.mk>
