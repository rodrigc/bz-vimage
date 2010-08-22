# $FreeBSD: src/sys/modules/dtrace/Makefile,v 1.7 2010/08/22 11:12:07 rpaulo Exp $

.include <bsd.own.mk>
.include "Makefile.inc"

SUBDIR=		dtmalloc	\
		dtnfsclient	\
		dtrace		\
		dtraceall	\
		dtrace_test	\
		lockstat	\
		profile		\
		prototype	\
		sdt		\
		systrace

.if ${MACHINE_ARCH} == "amd64" || ${MACHINE_ARCH} == "i386"
SUBDIR+=	fbt
.endif

.include <bsd.subdir.mk>
