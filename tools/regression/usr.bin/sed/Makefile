# $FreeBSD: src/tools/regression/usr.bin/sed/Makefile,v 1.5 2011/01/08 00:06:22 jilles Exp $

all:
	@m4 ${.CURDIR}/../regress.m4 ${.CURDIR}/regress.sh | sh /dev/stdin ${.CURDIR}
	@sh multitest.t
	@sh inplace_race.t
