# $FreeBSD: src/lib/clang/libllvmpowerpccodegen/Makefile,v 1.2 2010/07/20 17:16:57 ed Exp $

LIB=	llvmpowerpccodegen

SRCDIR=	lib/Target/PowerPC
SRCS=	PPCBranchSelector.cpp \
	PPCCodeEmitter.cpp \
	PPCHazardRecognizers.cpp \
	PPCISelDAGToDAG.cpp \
	PPCISelLowering.cpp \
	PPCInstrInfo.cpp \
	PPCJITInfo.cpp \
	PPCMCAsmInfo.cpp \
	PPCPredicates.cpp \
	PPCRegisterInfo.cpp \
	PPCSelectionDAGInfo.cpp \
	PPCSubtarget.cpp \
	PPCTargetMachine.cpp

TGHDRS=	Intrinsics \
	PPCGenCallingConv \
	PPCGenCodeEmitter \
	PPCGenDAGISel \
	PPCGenInstrInfo \
	PPCGenInstrNames \
	PPCGenRegisterInfo.h \
	PPCGenRegisterInfo \
	PPCGenRegisterNames \
	PPCGenSubtarget

.include "../clang.lib.mk"
