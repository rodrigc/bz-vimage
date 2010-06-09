# $Id$
# $FreeBSD: src/usr.sbin/acpi/Makefile.inc,v 1.10 2009/06/05 18:50:45 jkim Exp $

ACPICA_DIR= ${.CURDIR}/../../../sys/contrib/dev/acpica
CFLAGS+= -I${.CURDIR}/../../../sys

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.PATH:	${ACPICA_DIR}			\
	${ACPICA_DIR}/common		\
	${ACPICA_DIR}/compiler		\
	${ACPICA_DIR}/debugger		\
	${ACPICA_DIR}/disassembler	\
	${ACPICA_DIR}/dispatcher	\
	${ACPICA_DIR}/events		\
	${ACPICA_DIR}/executer		\
	${ACPICA_DIR}/hardware		\
	${ACPICA_DIR}/namespace		\
	${ACPICA_DIR}/parser		\
	${ACPICA_DIR}/resources		\
	${ACPICA_DIR}/tables		\
	${ACPICA_DIR}/utilities
