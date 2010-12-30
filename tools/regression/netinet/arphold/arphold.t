#!/bin/sh
#
# $FreeBSD: src/tools/regression/netinet/arphold/arphold.t,v 1.1 2010/11/12 22:03:02 gnn Exp $

make arphold 2>&1 > /dev/null

./arphold 192.168.1.222
