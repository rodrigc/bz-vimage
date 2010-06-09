# $FreeBSD: src/libexec/ulog-helper/Makefile,v 1.3 2010/02/26 09:41:16 ru Exp $

PROG=	ulog-helper
BINOWN=	root
BINMODE=4555
NO_MAN=

DPADD=	${LIBULOG} ${LIBMD}
LDADD=	-lulog -lmd

.include <bsd.prog.mk>
