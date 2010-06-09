#!/bin/sh
# $FreeBSD: src/tools/regression/usr.bin/comm/regress.t,v 1.1 2009/12/12 18:18:46 jh Exp $

cd `dirname $0`

m4 ../regress.m4 regress.sh | sh
