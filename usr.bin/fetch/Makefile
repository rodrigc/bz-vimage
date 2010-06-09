# $FreeBSD: src/usr.bin/fetch/Makefile,v 1.20 2010/02/26 09:41:16 ru Exp $

.include <bsd.own.mk>

PROG=		fetch
CSTD?=		c99
DPADD=		${LIBFETCH} ${LIBMD}
LDADD=		-lfetch -lmd
.if ${MK_OPENSSL} != "no"
DPADD+=		${LIBSSL} ${LIBCRYPTO}
LDADD+=		-lssl -lcrypto
.endif

.include <bsd.prog.mk>
