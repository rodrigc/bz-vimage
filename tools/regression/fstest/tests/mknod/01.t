#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/mknod/01.t,v 1.1 2010/08/06 20:51:39 pjd Exp $

desc="mknod returns ENOTDIR if a component of the path prefix is not a directory"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..5"

n0=`namegen`
n1=`namegen`

expect 0 mkdir ${n0} 0755
expect 0 create ${n0}/${n1} 0644
expect ENOTDIR mknod ${n0}/${n1}/test f 0644 0 0
expect 0 unlink ${n0}/${n1}
expect 0 rmdir ${n0}
