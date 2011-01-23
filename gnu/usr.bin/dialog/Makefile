# $FreeBSD: src/gnu/usr.bin/dialog/Makefile,v 1.15 2011/01/18 14:58:12 nwhitehorn Exp $

DIALOG=	${.CURDIR}/../../../contrib/dialog
PROG=   dialog

DPADD=	$(LIBDIALOG) $(LIBNCURSES) $(LIBM)
LDADD=	-ldialog -lncursesw -lm
CFLAGS+= -I${.CURDIR} -I${DIALOG}
.PATH:	${DIALOG}

WARNS?= 3

.include <bsd.prog.mk>
