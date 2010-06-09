# $FreeBSD: src/usr.sbin/pkg_install/Makefile.inc,v 1.18 2010/04/23 11:07:43 flz Exp $

.include <bsd.own.mk>

CFLAGS+=	-DPKG_INSTALL_VERSION=20100423
CFLAGS+=	-DYES_I_KNOW_THE_API_IS_RUBBISH_AND_IS_DOOMED_TO_CHANGE

DPADD+=		${LIBPKG}
LDADD+=		-lpkg

# Inherit BINDIR from one level up.
.include "../Makefile.inc"
