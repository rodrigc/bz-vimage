#!/bin/sh
# $FreeBSD: src/tools/regression/usr.bin/apply/regress.01.sh,v 1.1 2010/03/05 15:23:01 jh Exp $

SHELL=/bin/sh; export SHELL

ARG_MAX=$(getconf ARG_MAX)
ARG_MAX_HALF=$((ARG_MAX / 2))

apply 'echo %1 %1 %1' $(jot $ARG_MAX_HALF 1 1 | tr -d '\n') 2>&1

if [ $? -eq 0 ]; then
	return 1
else
	return 0
fi
