#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/chown/10.t,v 1.2 2010/08/17 06:08:09 pjd Exp $

desc="chown returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..4"

expect EFAULT chown NULL 65534 65534
expect EFAULT chown DEADCODE 65534 65534
expect EFAULT lchown NULL 65534 65534
expect EFAULT lchown DEADCODE 65534 65534
