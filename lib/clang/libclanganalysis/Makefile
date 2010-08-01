# $FreeBSD: src/lib/clang/libclanganalysis/Makefile,v 1.2 2010/07/20 17:16:57 ed Exp $

LIB=	clanganalysis

SRCDIR=	tools/clang/lib/Analysis
SRCS=	AnalysisContext.cpp \
	CFG.cpp \
	LiveVariables.cpp \
	PrintfFormatString.cpp \
	ReachableCode.cpp \
	UninitializedValues.cpp

TGHDRS=	AttrList \
	Attrs \
	DeclNodes \
	DiagnosticAnalysisKinds \
	DiagnosticCommonKinds \
	StmtNodes

.include "../clang.lib.mk"
