#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/truncate/02.t,v 1.2 2010/08/11 16:33:17 pjd Exp $

desc="truncate returns ENAMETOOLONG if a component of a pathname exceeded {NAME_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..5"

nx=`namegen_max`
nxx="${nx}x"

expect 0 create ${nx} 0644
expect 0 truncate ${nx} 123
expect 123 stat ${nx} size
expect 0 unlink ${nx}
expect ENAMETOOLONG truncate ${nxx} 123
