# $FreeBSD: src/secure/Makefile.inc,v 1.26 2010/01/02 09:58:07 ed Exp $

.include <bsd.own.mk>

.if exists(${.CURDIR}/../../lib/libcrypt/obj)
CRYPTOBJDIR=	${.CURDIR}/../../lib/libcrypt/obj
.else
CRYPTOBJDIR=	${.CURDIR}/../../lib/libcrypt
.endif

.if ${MK_IDEA} == "no"
CFLAGS+= -DNO_IDEA
.endif

.if ${MK_OPENSSH} != "no"
SSHDIR=		${.CURDIR}/../../../crypto/openssh
.endif

WARNS?=		0
