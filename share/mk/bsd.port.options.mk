# $FreeBSD: src/share/mk/bsd.port.options.mk,v 1.1 2007/06/01 15:17:51 pav Exp $

USEOPTIONSMK=	yes
INOPTIONSMK=	yes

.include <bsd.port.mk>

.undef INOPTIONSMK
