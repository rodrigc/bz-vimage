# $FreeBSD: src/lib/clang/libllvmmipscodegen/Makefile,v 1.1 2010/06/09 19:32:20 rdivacky Exp $

LIB=	llvmmipscodegen

SRCDIR=	lib/Target/Mips
SRCS=	MipsDelaySlotFiller.cpp MipsISelDAGToDAG.cpp \
	MipsISelLowering.cpp MipsInstrInfo.cpp MipsMCAsmInfo.cpp \
	MipsRegisterInfo.cpp MipsSelectionDAGInfo.cpp MipsSubtarget.cpp \
	MipsTargetMachine.cpp MipsTargetObjectFile.cpp

TGHDRS=	Intrinsics MipsGenAsmWriter MipsGenCallingConv MipsGenDAGISel \
	MipsGenInstrInfo MipsGenInstrNames MipsGenRegisterInfo.h \
	MipsGenRegisterInfo MipsGenRegisterNames MipsGenSubtarget

.include "../clang.lib.mk"
