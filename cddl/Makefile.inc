# $FreeBSD: src/cddl/Makefile.inc,v 1.8 2010/11/18 16:32:52 markm Exp $

OPENSOLARIS_USR_DISTDIR= ${.CURDIR}/../../../cddl/contrib/opensolaris
OPENSOLARIS_SYS_DISTDIR= ${.CURDIR}/../../../sys/cddl/contrib/opensolaris

IGNORE_PRAGMA=	YES

CFLAGS+=	-DNEED_SOLARIS_BOOLEAN

WARNS?=		6
CSTD?=		gnu89

# Do not lint the CDDL stuff. It is all externally maintained and
# lint output is wasteful noise here.

NO_LINT=
