# $FreeBSD: src/secure/libexec/Makefile,v 1.19 2010/03/10 11:33:15 des Exp $

.include <bsd.own.mk>

SUBDIR=
.if ${MK_OPENSSH} != "no"
SUBDIR+=sftp-server ssh-keysign ssh-pkcs11-helper
.endif

.include <bsd.subdir.mk>
