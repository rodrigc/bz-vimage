# $FreeBSD: src/usr.sbin/fifolog/Makefile.inc,v 1.3 2010/01/02 11:07:44 ed Exp $

LIBFIFOLOG= ${.OBJDIR}/../lib/libfifolog.a

#LINT=	flint
#LINTFLAGS= ${.CURDIR}/../flint.lnt -I/usr/include

.include "../Makefile.inc"
