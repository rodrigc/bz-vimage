#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/chflags/04.t,v 1.2 2008/11/22 13:27:15 pjd Exp $

desc="chflags returns ENOENT if the named file does not exist"

dir=`dirname $0`
. ${dir}/../misc.sh

require chflags

echo "1..4"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
expect ENOENT chflags ${n0}/${n1}/test SF_IMMUTABLE
expect ENOENT chflags ${n0}/${n1} SF_IMMUTABLE
expect 0 rmdir ${n0}
