#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/chflags/13.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="chflags returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

require chflags

echo "1..2"

expect EFAULT chflags NULL UF_NODUMP
expect EFAULT chflags DEADCODE UF_NODUMP
