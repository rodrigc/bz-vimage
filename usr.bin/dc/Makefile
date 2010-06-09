#	$FreeBSD: src/usr.bin/dc/Makefile,v 1.4 2010/02/03 19:12:27 gabor Exp $
#	$OpenBSD: Makefile,v 1.2 2006/11/26 11:31:09 deraadt Exp $

PROG=	dc
SRCS=	dc.c bcode.c inout.c mem.c stack.c
CFLAGS+=--param max-inline-insns-single=64
DPADD=	${LIBCRYPTO}
LDADD=	-lcrypto

.include <bsd.prog.mk>
