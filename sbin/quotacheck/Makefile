# $FreeBSD: src/sbin/quotacheck/Makefile,v 1.10 2010/05/07 00:41:12 mckusick Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	quotacheck
SRCS=	quotacheck.c preen.c fsutil.c utilities.c
WARNS?=	2
MAN=	quotacheck.8
DPADD=  ${LIBUTIL}
LDADD=  -lutil

.PATH:	${.CURDIR}/../fsck ${.CURDIR}/../fsck_ffs

.include <bsd.prog.mk>
