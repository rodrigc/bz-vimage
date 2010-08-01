#
# $FreeBSD: src/usr.sbin/bsnmpd/modules/snmp_wlan/Makefile,v 1.1 2010/07/26 16:16:39 syrinx Exp $
#

MOD=	wlan
SRCS=	wlan_snmp.c wlan_sys.c
CFLAGS+= -DSNMPTREE_TYPES

WARNS=            6

XSYM=	begemotWlan

BMIBS=	BEGEMOT-WIRELESS-MIB.txt
MAN=	snmp_${MOD}.3
DEFS=	${MOD}_tree.def

.include <bsd.snmpmod.mk>
