# $FreeBSD: src/etc/pam.d/Makefile,v 1.17 2009/07/18 16:29:40 marcus Exp $

NO_OBJ=

FILES=	README \
	atrun \
	cron \
	ftpd \
	imap \
	kde \
	login \
	other \
	passwd pop3 \
	rsh \
	sshd su system \
	telnetd \
	xdm

FILESDIR=	/etc/pam.d
FILESMODE=	644
FILESMODE_README=	444
LINKS=	${FILESDIR}/ftpd ${FILESDIR}/ftp

.include <bsd.prog.mk>
