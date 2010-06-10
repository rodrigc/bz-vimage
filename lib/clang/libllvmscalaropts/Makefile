# $FreeBSD: src/lib/clang/libllvmscalaropts/Makefile,v 1.1 2010/06/09 19:32:20 rdivacky Exp $

LIB=	llvmscalaropts

SRCDIR=	lib/Transforms/Scalar
SRCS=	ADCE.cpp BasicBlockPlacement.cpp CodeGenPrepare.cpp \
	ConstantProp.cpp DCE.cpp DeadStoreElimination.cpp \
	GEPSplitter.cpp GVN.cpp IndVarSimplify.cpp JumpThreading.cpp \
	LICM.cpp LoopDeletion.cpp LoopIndexSplit.cpp LoopRotation.cpp \
	LoopStrengthReduce.cpp LoopUnrollPass.cpp LoopUnswitch.cpp \
	MemCpyOptimizer.cpp Reassociate.cpp Reg2Mem.cpp SCCP.cpp \
	Scalar.cpp ScalarReplAggregates.cpp SimplifyCFGPass.cpp \
	SimplifyHalfPowrLibCalls.cpp SimplifyLibCalls.cpp \
	TailDuplication.cpp TailRecursionElimination.cpp

TGHDRS=	Intrinsics

.include "../clang.lib.mk"
