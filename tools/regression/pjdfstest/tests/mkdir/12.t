#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/mkdir/12.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="mkdir returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..2"

expect EFAULT mkdir NULL 0755
expect EFAULT mkdir DEADCODE 0755
