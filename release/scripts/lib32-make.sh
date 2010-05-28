#!/bin/sh
#
# $FreeBSD: src/release/scripts/lib32-make.sh,v 1.2 2009/08/24 21:55:43 jhb Exp $
#

# Clean the dust.
cd ${RD}/trees/lib32 && \
    find . '(' -path '*/usr/share/*' -or -path '*/usr/lib/*' ')' -delete
