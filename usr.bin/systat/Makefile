#	@(#)Makefile	8.1 (Berkeley) 6/6/93
# $FreeBSD: src/usr.bin/systat/Makefile,v 1.25 2010/02/25 20:24:19 ru Exp $

.include <bsd.own.mk>

PROG=	systat
SRCS=	cmds.c cmdtab.c devs.c fetch.c iostat.c keyboard.c main.c \
	mbufs.c netcmds.c netstat.c pigs.c swap.c icmp.c \
	mode.c ip.c tcp.c \
	vmstat.c convtbl.c ifcmds.c ifstat.c

.if ${MK_INET6_SUPPORT} != "no"
SRCS+=	icmp6.c ip6.c
CFLAGS+= -DINET6
.endif

WARNS?=	0

DPADD=	${LIBNCURSESW} ${LIBM} ${LIBDEVSTAT} ${LIBKVM}
LDADD=	-lncursesw -lm -ldevstat -lkvm

.include <bsd.prog.mk>
