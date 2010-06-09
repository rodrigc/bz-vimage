# $FreeBSD: src/usr.sbin/eeprom/Makefile,v 1.4 2010/01/02 11:07:44 ed Exp $

.PATH: ${.CURDIR}/../ofwdump

PROG=	eeprom
MAN=	eeprom.8
MANSUBDIR= /sparc64
SRCS=	eeprom.c ofw_options.c ofw_util.c
CFLAGS+= -I${.CURDIR}/../ofwdump

.include <bsd.prog.mk>
