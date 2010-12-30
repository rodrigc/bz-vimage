# $FreeBSD: src/gnu/usr.bin/gdb/arch/mips/Makefile,v 1.3 2010/11/07 20:53:19 gonzo Exp $

.if !defined(GDB_CROSS_DEBUGGER)
LIBSRCS+= mipsfbsd-nat.c fbsd-threads.c
.endif
LIBSRCS+= solib.c solib-svr4.c
LIBSRCS+= mips-tdep.c mipsfbsd-tdep.c fbsd-proc.c

nm.h:
	echo '#include "mips/nm-fbsd.h"' > ${.TARGET}

tm.h:
	echo '#include "mips/tm-fbsd.h"' > ${.TARGET}
