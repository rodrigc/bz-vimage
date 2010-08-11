#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/mknod/03.t,v 1.2 2010/08/11 16:33:17 pjd Exp $

desc="mknod returns ENAMETOOLONG if an entire path name exceeded {PATH_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..12"

nx=`dirgen_max`
nxx="${nx}x"

mkdir -p "${nx%/*}"

expect 0 mknod ${nx} f 0644 0 0
expect fifo stat ${nx} type
expect 0 unlink ${nx}
expect ENAMETOOLONG mknod ${nxx} f 0644 0 0

expect 0 mknod ${nx} b 0644 1 2
expect block stat ${nx} type
expect 0 unlink ${nx}
expect ENAMETOOLONG mknod ${nxx} b 0644 1 2

expect 0 mknod ${nx} c 0644 1 2
expect char stat ${nx} type
expect 0 unlink ${nx}
expect ENAMETOOLONG mknod ${nxx} c 0644 1 2

rm -rf "${nx%%/*}"
