#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/unlink/03.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="unlink returns ENAMETOOLONG if an entire path name exceeded {PATH_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..4"

nx=`dirgen_max`
nxx="${nx}x"

mkdir -p "${nx%/*}"

expect 0 create ${nx} 0644
expect 0 unlink ${nx}
expect ENOENT unlink ${nx}
expect ENAMETOOLONG unlink ${nxx}

rm -rf "${nx%%/*}"
