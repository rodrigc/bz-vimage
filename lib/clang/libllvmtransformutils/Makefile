# $FreeBSD: src/lib/clang/libllvmtransformutils/Makefile,v 1.1 2010/06/09 19:32:20 rdivacky Exp $

LIB=	llvmtransformutils

SRCDIR=	lib/Transforms/Utils
SRCS=	AddrModeMatcher.cpp BasicBlockUtils.cpp BasicInliner.cpp \
	BreakCriticalEdges.cpp BuildLibCalls.cpp CloneFunction.cpp \
	CloneLoop.cpp CloneModule.cpp CodeExtractor.cpp \
	DemoteRegToStack.cpp InlineFunction.cpp InstructionNamer.cpp \
	LCSSA.cpp Local.cpp LoopSimplify.cpp LoopUnroll.cpp \
	LowerInvoke.cpp LowerSwitch.cpp Mem2Reg.cpp \
	PromoteMemoryToRegister.cpp SSAUpdater.cpp SSI.cpp \
	SimplifyCFG.cpp UnifyFunctionExitNodes.cpp ValueMapper.cpp

TGHDRS=	Intrinsics

.include "../clang.lib.mk"
