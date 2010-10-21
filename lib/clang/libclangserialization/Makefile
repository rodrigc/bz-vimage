# $FreeBSD: src/lib/clang/libclangserialization/Makefile,v 1.1 2010/09/20 16:43:17 dim Exp $

LIB=	clangserialization

SRCDIR=	tools/clang/lib/Serialization
SRCS=	ASTCommon.cpp \
	ASTReader.cpp \
	ASTReaderDecl.cpp \
	ASTReaderStmt.cpp \
	ASTWriter.cpp \
	ASTWriterDecl.cpp \
	ASTWriterStmt.cpp \
	GeneratePCH.cpp

TGHDRS=	AttrList \
	AttrPCHRead \
	AttrPCHWrite \
	Attrs \
	DeclNodes \
	DiagnosticCommonKinds \
	DiagnosticFrontendKinds \
	DiagnosticSemaKinds \
	StmtNodes

.include "../clang.lib.mk"
