#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/chown/03.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="chown returns ENAMETOOLONG if an entire path name exceeded {PATH_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..10"

nx=`dirgen_max`
nxx="${nx}x"

mkdir -p "${nx%/*}"

expect 0 create ${nx} 0644
expect 0 chown ${nx} 65534 65534
expect 65534,65534 stat ${nx} uid,gid
expect 0 unlink ${nx}
expect ENAMETOOLONG chown ${nxx} 65534 65534

expect 0 create ${nx} 0644
expect 0 lchown ${nx} 65534 65534
expect 65534,65534 stat ${nx} uid,gid
expect 0 unlink ${nx}
expect ENAMETOOLONG lchown ${nxx} 65534 65534

rm -rf "${nx%%/*}"
