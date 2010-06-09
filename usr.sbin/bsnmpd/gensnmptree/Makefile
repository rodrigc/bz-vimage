# $FreeBSD: src/usr.sbin/bsnmpd/gensnmptree/Makefile,v 1.5 2010/01/02 11:07:44 ed Exp $
#
# Author: Harti Brandt <harti@freebsd.org>

CONTRIB=${.CURDIR}/../../../contrib/bsnmp
.PATH: ${CONTRIB}/gensnmptree

PROG=	gensnmptree
CFLAGS+= -I${CONTRIB}/lib
CFLAGS+= -DQUADFMT='"llu"' -DQUADXFMT='"llx"' -DHAVE_STDINT_H
CFLAGS+= -DHAVE_INTTYPES_H

WARNS?=	5

.include <bsd.prog.mk>
