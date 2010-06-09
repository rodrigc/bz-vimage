#!/bin/sh
# $FreeBSD: src/share/examples/hast/vip-up.sh,v 1.1 2010/02/18 23:16:19 pjd Exp $

set -m
/root/hast/sbin/hastd/ucarp_up.sh &
set +m
exit 0
