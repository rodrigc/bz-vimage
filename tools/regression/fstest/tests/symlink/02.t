#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/symlink/02.t,v 1.2 2010/08/11 16:33:17 pjd Exp $

desc="symlink returns ENAMETOOLONG if a component of the name2 pathname exceeded {NAME_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..7"

n0=`namegen`
nx=`namegen_max`
nxx="${nx}x"

expect 0 symlink ${nx} ${n0}
expect 0 unlink ${n0}
expect 0 symlink ${n0} ${nx}
expect 0 unlink ${nx}

expect ENAMETOOLONG symlink ${n0} ${nxx}
expect 0 symlink ${nxx} ${n0}
expect 0 unlink ${n0}
