#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/symlink/03.t,v 1.2 2010/08/11 16:33:17 pjd Exp $

desc="symlink returns ENAMETOOLONG if an entire length of either path name exceeded {PATH_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..6"

n0=`namegen`
nx=`dirgen_max`
nxx="${nx}x"

mkdir -p "${nx%/*}"
expect 0 symlink ${nx} ${n0}
expect 0 unlink ${n0}
expect 0 symlink ${n0} ${nx}
expect 0 unlink ${nx}
expect ENAMETOOLONG symlink ${n0} ${nxx}
expect ENAMETOOLONG symlink ${nxx} ${n0}
rm -rf "${nx%%/*}"
