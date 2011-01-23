#
# $FreeBSD: src/tools/regression/acct/Makefile,v 1.3 2011/01/06 20:05:24 keramida Exp $
#
# "make" will compile the acct test programs
#

PROG=	pack
SRCS=	pack.c
NO_MAN=
CFLAGS+= -I${.OBJDIR}

pack.o:	convert.c

CLEANFILES+= convert.c
convert.c: ../../../sys/kern/kern_acct.c
	sed -n -e 's/log(/syslog(/g' \
	       -e '/FLOAT_CONVERSION_START/,/FLOAT_CONVERSION_END/p' $? >$@

regress: pack regress.t
	${.CURDIR}/regress.t

.include <bsd.prog.mk>
