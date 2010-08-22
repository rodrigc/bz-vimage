#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/rename/19.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="rename returns EINVAL/EBUSY when an attempt is made to rename '.' or '..'"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..6"

n0=`namegen`
n1=`namegen`
n2=`namegen`

expect 0 mkdir ${n0} 0755
expect 0 mkdir ${n0}/${n1} 0755

expect "EINVAL|EBUSY" rename ${n0}/${n1}/. ${n2}
expect "EINVAL|EBUSY" rename ${n0}/${n1}/.. ${n2}

expect 0 rmdir ${n0}/${n1}
expect 0 rmdir ${n0}
