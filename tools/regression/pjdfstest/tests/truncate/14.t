#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/truncate/14.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="truncate returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..2"

expect EFAULT truncate NULL 123
expect EFAULT truncate DEADCODE 123
