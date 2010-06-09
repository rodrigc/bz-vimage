# $FreeBSD: src/usr.sbin/bsnmpd/modules/Makefile.inc,v 1.15 2010/03/02 18:44:08 uqs Exp $

SHLIB_MAJOR=	6

MANFILTER=	sed -e 's%@MODPATH@%${LIBDIR}/%g'		\
		    -e 's%@DEFPATH@%${DEFSDIR}/%g'		\
		    -e 's%@MIBSPATH@%${BMIBSDIR}/%g'

.include "../Makefile.inc"
