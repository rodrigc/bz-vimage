# $FreeBSD: src/sbin/mount_reiserfs/Makefile,v 1.3 2009/10/19 16:00:24 ru Exp $

PROG = mount_reiserfs
SRCS = mount_reiserfs.c getmntopts.c
MAN  = mount_reiserfs.8

# mount_reiserfs needs mntopts.h and getmntopts.c from src/sbin/mount/
MOUNT  ?= ${.CURDIR}/../mount
CFLAGS += -I${MOUNT}

.PATH: ${MOUNT}

.include <bsd.prog.mk>
