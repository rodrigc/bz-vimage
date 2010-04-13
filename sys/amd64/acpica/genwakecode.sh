#!/bin/sh
# $FreeBSD: src/sys/amd64/acpica/genwakecode.sh,v 1.2 2009/03/23 22:35:30 jkim Exp $
#
file2c -sx 'static char wakecode[] = {' '};' <acpi_wakecode.bin

exit 0
