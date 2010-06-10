# $FreeBSD: src/lib/clang/libllvmselectiondag/Makefile,v 1.1 2010/06/09 19:32:20 rdivacky Exp $

LIB=	llvmselectiondag

SRCDIR=	lib/CodeGen/SelectionDAG
SRCS=	CallingConvLower.cpp DAGCombiner.cpp FastISel.cpp \
	FunctionLoweringInfo.cpp InstrEmitter.cpp LegalizeDAG.cpp \
	LegalizeFloatTypes.cpp LegalizeIntegerTypes.cpp LegalizeTypes.cpp \
	LegalizeTypesGeneric.cpp LegalizeVectorOps.cpp LegalizeVectorTypes.cpp \
	ScheduleDAGFast.cpp ScheduleDAGList.cpp ScheduleDAGRRList.cpp \
	ScheduleDAGSDNodes.cpp SelectionDAG.cpp SelectionDAGBuilder.cpp \
	SelectionDAGISel.cpp SelectionDAGPrinter.cpp TargetLowering.cpp \
	TargetSelectionDAGInfo.cpp

TGHDRS=	Intrinsics

.include "../clang.lib.mk"
