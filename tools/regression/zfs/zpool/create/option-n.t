#!/bin/sh
# $FreeBSD: src/tools/regression/zfs/zpool/create/option-n.t,v 1.1 2008/11/17 20:49:29 pjd Exp $

dir=`dirname $0`
. ${dir}/../../misc.sh

echo "1..5"

disks_create 1
names_create 1

expect_fl is_mountpoint /${name0}
exp=`(
  echo "would create '${name0}' with the following layout:"
  echo "	${name0}"
  echo "	  ${disk0}"
)`
expect "${exp}" ${ZPOOL} create -n ${name0} ${disk0}
expect_fl is_mountpoint /${name0}
expect_fl ${ZPOOL} status -x ${name0}
expect_fl ${ZPOOL} destroy ${name0}

disks_destroy
