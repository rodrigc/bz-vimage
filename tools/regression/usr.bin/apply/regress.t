#!/bin/sh
# $FreeBSD: src/tools/regression/usr.bin/apply/regress.t,v 1.1 2010/03/05 15:23:01 jh Exp $

cd `dirname $0`

m4 ../regress.m4 regress.sh | sh
