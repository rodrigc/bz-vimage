# $FreeBSD: src/lib/clang/libllvmarmasmprinter/Makefile,v 1.2 2010/07/20 17:16:57 ed Exp $

LIB=	llvmarmasmprinter

SRCDIR=	lib/Target/ARM/AsmPrinter
INCDIR=	lib/Target/ARM
SRCS=	ARMAsmPrinter.cpp \
	ARMInstPrinter.cpp \
	ARMMCInstLower.cpp

TGHDRS=	ARMGenAsmWriter \
	ARMGenInstrNames \
	ARMGenRegisterInfo.h \
	ARMGenRegisterNames

.include "../clang.lib.mk"
