#!/bin/sh
#
# $FreeBSD: src/tools/regression/lib/libutil/test-grp.t,v 1.1 2008/04/23 00:49:13 scf Exp $
#

base=$(realpath $(dirname $0))
name=$(basename $0 .t)

set -e
cd $base
make -s $name >/dev/null
exec $base/$name
