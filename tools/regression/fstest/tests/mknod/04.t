#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/mknod/04.t,v 1.1 2010/08/06 20:51:39 pjd Exp $

desc="mkfifo returns ENOENT if a component of the path prefix does not exist"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..3"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
expect ENOENT mknod ${n0}/${n1}/test f 0644 0 0
expect 0 rmdir ${n0}
