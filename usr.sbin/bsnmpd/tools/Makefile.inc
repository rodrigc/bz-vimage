# $FreeBSD: src/usr.sbin/bsnmpd/tools/Makefile.inc,v 1.1 2010/12/08 14:30:25 syrinx Exp $
# Author: Shteryana Shopova <syrinx@FreeBSD.org>

BINDIR?= /usr/bin

CFLAGS+= -I. -I${.CURDIR}

.if exists(${.OBJDIR}/../libbsnmptools)
LIBBSNMPTOOLSDIR=      ${.OBJDIR}/../libbsnmptools
.else
LIBBSNMPTOOLSDIR=      ${.CURDIR}/../libbsnmptools
.endif
LIBBSNMPTOOLS=         ${LIBBSNMPTOOLSDIR}/libbsnmptools.a
