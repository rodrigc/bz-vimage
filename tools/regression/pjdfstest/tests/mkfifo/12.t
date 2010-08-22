#!/bin/sh
# $FreeBSD: src/tools/regression/pjdfstest/tests/mkfifo/12.t,v 1.1 2010/08/15 21:24:17 pjd Exp $

desc="mkfifo returns EFAULT if the path argument points outside the process's allocated address space"

dir=`dirname $0`
. ${dir}/../misc.sh

echo "1..2"

expect EFAULT mkfifo NULL 0644
expect EFAULT mkfifo DEADCODE 0644
