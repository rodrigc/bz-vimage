#! /bin/sh
# $FreeBSD: src/tools/regression/usr.bin/make/syntax/directive-t0/test.t,v 1.1 2010/01/04 09:49:23 obrien Exp $

cd `dirname $0`
. ../../common.sh

# Description
DESC="A typo'ed directive."

# Run
TEST_N=1
TEST_1=

eval_cmd $*
