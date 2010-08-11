#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/rmdir/03.t,v 1.2 2010/08/11 16:33:17 pjd Exp $

desc="rmdir returns ENAMETOOLONG if an entire path name exceeded ${PATH_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..5"

nx=`dirgen_max`
nxx="${nx}x"

mkdir -p "${nx%/*}"

expect 0 mkdir ${nx} 0755
expect dir,0755 stat ${nx} type,mode
expect 0 rmdir ${nx}
expect ENOENT rmdir ${nx}
expect ENAMETOOLONG rmdir ${nxx}

rm -rf "${nx%%/*}"
