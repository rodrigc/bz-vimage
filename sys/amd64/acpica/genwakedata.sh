#!/bin/sh
# $FreeBSD: src/sys/amd64/acpica/genwakedata.sh,v 1.1 2009/03/17 00:48:11 jkim Exp $
#
nm -n --defined-only acpi_wakecode.o | while read offset dummy what
do
    echo "#define ${what}	0x${offset}"
done

exit 0
