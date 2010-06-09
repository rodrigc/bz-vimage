# $FreeBSD: src/kerberos5/lib/libheimntlm/Makefile,v 1.1 2008/05/07 13:53:04 dfr Exp $

LIB=	heimntlm
SRCS=	ntlm.c
INCS=	heimntlm.h heimntlm-protos.h
CFLAGS+=-I${KRB5DIR}/lib/ntlm
VERSION_MAP= ${KRB5DIR}/lib/ntlm/version-script.map

.include <bsd.lib.mk>

.PATH: ${KRB5DIR}/lib/ntlm
