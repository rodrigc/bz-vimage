# $FreeBSD: src/lib/clang/libllvmmipscodegen/Makefile,v 1.2 2010/07/20 17:16:57 ed Exp $

LIB=	llvmmipscodegen

SRCDIR=	lib/Target/Mips
SRCS=	MipsDelaySlotFiller.cpp \
	MipsISelDAGToDAG.cpp \
	MipsISelLowering.cpp \
	MipsInstrInfo.cpp \
	MipsMCAsmInfo.cpp \
	MipsRegisterInfo.cpp \
	MipsSelectionDAGInfo.cpp \
	MipsSubtarget.cpp \
	MipsTargetMachine.cpp \
	MipsTargetObjectFile.cpp

TGHDRS=	Intrinsics \
	MipsGenAsmWriter \
	MipsGenCallingConv \
	MipsGenDAGISel \
	MipsGenInstrInfo \
	MipsGenInstrNames \
	MipsGenRegisterInfo.h \
	MipsGenRegisterInfo \
	MipsGenRegisterNames \
	MipsGenSubtarget

.include "../clang.lib.mk"
