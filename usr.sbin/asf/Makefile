# $FreeBSD: src/usr.sbin/asf/Makefile,v 1.4 2010/01/02 11:07:44 ed Exp $

PROG=	asf
SRCS=	asf.c asf_kld.c asf_kvm.c asf_prog.c
MAN=	asf.8

DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
