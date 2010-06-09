#!/bin/sh
# $FreeBSD: src/tools/regression/usr.bin/ncal/regress.t,v 1.1 2010/03/14 10:24:03 edwin Exp $

cd `dirname $0`

m4 ../regress.m4 regress.sh | sh
