# $FreeBSD: src/lib/clang/libllvmpowerpccodegen/Makefile,v 1.3 2010/09/20 16:43:17 dim Exp $

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
	PPCGenRegisterInfo \
	PPCGenRegisterInfo.h \
	PPCGenRegisterNames \
	PPCGenSubtarget

.include "../clang.lib.mk"
