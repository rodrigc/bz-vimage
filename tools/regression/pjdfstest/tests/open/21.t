#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/open/21.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="open returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..2"

expect EFAULT open NULL O_RDONLY
expect EFAULT open DEADCODE O_RDONLY
