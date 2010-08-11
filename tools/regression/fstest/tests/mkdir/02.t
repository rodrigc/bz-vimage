#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/mkdir/02.t,v 1.2 2010/08/11 16:33:17 pjd Exp $

desc="mkdir returns ENAMETOOLONG if a component of a pathname exceeded {NAME_MAX} characters"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..3"

nx=`namegen_max`
nxx="${nx}x"

expect 0 mkdir ${nx} 0755
expect 0 rmdir ${nx}
expect ENAMETOOLONG mkdir ${nxx} 0755
