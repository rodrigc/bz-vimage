#	$FreeBSD: src/usr.sbin/makefs/compat/Makefile.inc,v 1.1 2010/11/07 16:05:04 cognet Exp $
#

.PATH:	${.CURDIR}/compat

CFLAGS+=	-I${.CURDIR}/compat

SRCS+=	pwcache.c strsuftoll.c
