# $FreeBSD: src/usr.sbin/wpa/wpa_passphrase/Makefile,v 1.4 2010/11/03 10:44:25 rpaulo Exp $

.include "${.CURDIR}/../Makefile.inc"

.PATH.c:${WPA_SUPPLICANT_DISTDIR}

PROG=	wpa_passphrase
SRCS=	wpa_passphrase.c sha1.c sha1-internal.c sha1-pbkdf2.c \
	md5.c md5-internal.c

CFLAGS+= -DINTERNAL_SHA1
CFLAGS+= -DINTERNAL_MD5

MAN=	wpa_passphrase.8

.include <bsd.prog.mk>
