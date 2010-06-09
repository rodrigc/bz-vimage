# $FreeBSD: src/etc/periodic/monthly/Makefile,v 1.4 2008/09/21 22:02:26 sam Exp $

.include <bsd.own.mk>

FILES=	999.local

# NB: keep these sorted by MK_* knobs

.if ${MK_ACCT} != "no"
FILES+=	200.accounting
.endif

.include <bsd.prog.mk>
