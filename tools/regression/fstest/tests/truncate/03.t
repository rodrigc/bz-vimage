#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/truncate/03.t,v 1.2 2010/08/11 16:33:17 pjd Exp $

desc="truncate returns ENAMETOOLONG if an entire path name exceeded {PATH_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..5"

nx=`dirgen_max`
nxx="${nx}x"

mkdir -p "${nx%/*}"

expect 0 create ${nx} 0644
expect 0 truncate ${nx} 123
expect regular,123 stat ${nx} type,size
expect 0 unlink ${nx}
expect ENAMETOOLONG truncate ${nxx} 123

rm -rf "${nx%%/*}"
