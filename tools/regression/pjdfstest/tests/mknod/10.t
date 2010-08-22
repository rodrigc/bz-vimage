#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/mknod/10.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="mknod returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..2"

expect EFAULT mknod NULL f 0644 0 0
expect EFAULT mknod DEADCODE f 0644 0 0
