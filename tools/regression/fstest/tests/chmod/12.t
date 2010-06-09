#!/bin/sh
# $FreeBSD: src/tools/regression/fstest/tests/chmod/12.t,v 1.1 2009/09/07 19:40:22 trasz Exp $

desc="verify SUID/SGID bit behaviour"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..10"

n0=`namegen`
n1=`namegen`
n2=`namegen`

expect 0 mkdir ${n2} 0755
cdir=`pwd`
cd ${n2}

# Check whether writing to the file by non-owner clears the SUID.
expect 0 create ${n0} 04777
expect 0 -u 65534 -g 65534 write ${n0}
expect 0777 stat ${n0} mode
expect 0 unlink ${n0}

# Check whether writing to the file by non-owner clears the SGID.
expect 0 create ${n0} 02777
expect 0 -u 65534 -g 65534 write ${n0}
expect 0777 stat ${n0} mode
expect 0 unlink ${n0}

cd ${cdir}
expect 0 rmdir ${n2}
