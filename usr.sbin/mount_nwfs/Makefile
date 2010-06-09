# $FreeBSD: src/usr.sbin/mount_nwfs/Makefile,v 1.8 2010/03/04 16:07:14 uqs Exp $

PROG=	mount_nwfs
SRCS=	mount_nwfs.c getmntopts.c
MAN=	mount_nwfs.8

MOUNT=	${.CURDIR}/../../sbin/mount
CFLAGS+= -DNWFS -I${MOUNT}

.PATH:	${MOUNT}

DPADD=	${LIBNCP} ${LIBIPX}
LDADD=	-lncp -lipx

.include <bsd.prog.mk>
