# $FreeBSD: src/gnu/lib/libreadline/readline/doc/Makefile,v 1.6 2010/08/12 20:46:49 will Exp $

SRCDIR= ${.CURDIR}/../../../../../contrib/libreadline/doc

INFO = readline rluserman

INFOSECTION= "Programming & development tools."
INFOENTRY_readline= "* Readline: (readline).		The GNU Readline library"
INFOENTRY_rluserman= "* Readline: (readline).       The GNU readline library API"

CLEANFILES += readline.texi

readline.info: rlman.texi rluser.texi rltech.texi version.texi fdl.texi

readline.texi: rlman.texi
	cp -f ${SRCDIR}/rlman.texi ${.TARGET}

rluserman.info: rluserman.texi version.texi rluser.texi fdl.texi

.include <bsd.info.mk>
