# $FreeBSD: src/lib/libc/powerpc64/gen/Makefile.inc,v 1.1 2010/07/10 14:45:03 nwhitehorn Exp $

SRCS += _ctx_start.S fabs.S flt_rounds.c fpgetmask.c fpgetround.c \
	fpgetsticky.c fpsetmask.c fpsetround.c \
	infinity.c ldexp.c makecontext.c modf.c _setjmp.S \
	setjmp.S sigsetjmp.S signalcontext.c syncicache.c \
	_set_tp.c


