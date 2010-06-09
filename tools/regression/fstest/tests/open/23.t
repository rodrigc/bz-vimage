#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/open/23.t,v 1.2 2008/11/23 20:07:00 pjd Exp $

desc="open may return EINVAL when an attempt was made to open a descriptor with an illegal combination of O_RDONLY, O_WRONLY, and O_RDWR"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..4"

n0=`namegen`

expect 0 create ${n0} 0644
expect "0|EINVAL" open ${n0} O_WRONLY,O_RDWR
expect "0|EINVAL" open ${n0} O_RDONLY,O_WRONLY,O_RDWR
expect 0 unlink ${n0}
