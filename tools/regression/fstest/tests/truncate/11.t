#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/truncate/11.t,v 1.2 2010/08/06 23:58:54 pjd Exp $

desc="truncate returns ETXTBSY the file is a pure procedure (shared text) file that is being executed"

dir=`dirname $0`
. ${dir}/../misc.sh

[ "${os}" = "FreeBSD" ] || quick_exit

echo "1..2"

n0=`namegen`

cp -pf `which sleep` ${n0}
./${n0} 3 &
expect ETXTBSY truncate ${n0} 123
expect 0 unlink ${n0}
