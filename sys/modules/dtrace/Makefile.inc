# $FreeBSD: src/sys/modules/dtrace/Makefile.inc,v 1.2 2010/08/23 06:13:29 imp Exp $

IGNORE_PRAGMA=	1

load	:
	-kldload cyclic
	-kldload dtrace
.if ${MACHINE_CPUARCH} == "i386"
	-kldload sdt
	-kldload lockstat
	-kldload fbt
	-kldload prototype
.endif
	-kldload profile
	-kldload systrace
	kldstat

unload	:
	-kldunload systrace
	-kldunload profile
.if ${MACHINE_CPUARCH} == "i386"
	-kldunload prototype
	-kldunload fbt
	-kldunload lockstat
	-kldunload sdt
.endif
	-kldunload dtrace
	-kldunload cyclic
	kldstat
