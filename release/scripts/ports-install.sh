#!/bin/sh
#
# $FreeBSD: src/release/scripts/ports-install.sh,v 1.4 2010/06/19 09:33:11 brian Exp $
#

if [ "`id -u`" != "0" ]; then
	echo "Sorry, this must be done as root."
	exit 1
fi
echo "Extracting ports tarball into ${DESTDIR}/usr"
tar --unlink -xpzf ports.tgz -C ${DESTDIR}/usr
exit 0
