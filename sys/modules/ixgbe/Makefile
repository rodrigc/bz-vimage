#$FreeBSD: src/sys/modules/ixgbe/Makefile,v 1.8 2010/08/16 17:48:55 yongari Exp $
.PATH:  ${.CURDIR}/../../dev/ixgbe
KMOD    = ixgbe
SRCS    = device_if.h bus_if.h pci_if.h
SRCS    += ixgbe.c
# Shared source
SRCS    += ixgbe_common.c ixgbe_api.c ixgbe_phy.c
SRCS    += ixgbe_82599.c ixgbe_82598.c
CFLAGS+= -I${.CURDIR}/../../dev/ixgbe -DSMP -DIXGBE_FDIR

.include <bsd.kmod.mk>
