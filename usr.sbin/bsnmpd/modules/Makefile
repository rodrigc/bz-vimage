# $FreeBSD: src/usr.sbin/bsnmpd/modules/Makefile,v 1.19 2010/12/20 17:13:14 syrinx Exp $

.include <bsd.own.mk>

.PATH: ${.CURDIR}/../../../contrib/bsnmp/snmpd

.if ${MK_ATM} != "no"
_snmp_atm= snmp_atm
.endif

SUBDIR=	${_snmp_atm} \
	snmp_bridge \
	snmp_hostres \
	snmp_mibII \
	snmp_pf \
	snmp_target \
	snmp_usm \
	snmp_vacm \
	snmp_wlan

.if ${MK_NETGRAPH_SUPPORT} != "no"
SUBDIR+=snmp_netgraph
.endif

INCS=	snmpmod.h
INCSDIR= ${INCLUDEDIR}/bsnmp

.include <bsd.prog.mk>
