#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/open/20.t,v 1.2 2010/08/06 23:58:54 pjd Exp $

desc="open returns ETXTBSY when the file is a pure procedure (shared text) file that is being executed and the open() system call requests write access"

dir=`dirname $0`
. ${dir}/../misc.sh

[ "${os}:${fs}" = "FreeBSD:UFS" ] || quick_exit

echo "1..4"

n0=`namegen`

cp -pf `which sleep` ${n0}
./${n0} 3 &
expect ETXTBSY open ${n0} O_WRONLY
expect ETXTBSY open ${n0} O_RDWR
expect ETXTBSY open ${n0} O_RDONLY,O_TRUNC
expect 0 unlink ${n0}
