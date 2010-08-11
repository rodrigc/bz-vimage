#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/mknod/07.t,v 1.1 2010/08/06 20:51:39 pjd Exp $

desc="mknod returns ELOOP if too many symbolic links were encountered in translating the pathname"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..6"

n0=`namegen`
n1=`namegen`

expect 0 symlink ${n0} ${n1}
expect 0 symlink ${n1} ${n0}
expect ELOOP mknod ${n0}/test f 0644 0 0
expect ELOOP mknod ${n1}/test f 0644 0 0
expect 0 unlink ${n0}
expect 0 unlink ${n1}
