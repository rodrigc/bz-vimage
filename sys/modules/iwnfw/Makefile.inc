# $FreeBSD: src/sys/modules/iwnfw/Makefile.inc,v 1.1 2009/10/24 09:55:11 rpaulo Exp $
#
# Common rules for building firmware.  Note this gets auto-included
# by the subdir Makefile's as a consequence of included bsd.kmod.mk.

_FIRM=	${IMG}.fw

CLEANFILES+=	${_FIRM}

FIRMWS=	${_FIRM}:${KMOD}

${_FIRM}: ${.CURDIR}/../../../contrib/dev/iwn/${_FIRM}.uu
	uudecode -p $? > ${.TARGET}
