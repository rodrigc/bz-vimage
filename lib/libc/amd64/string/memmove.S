/*	$NetBSD: memmove.S,v 1.1 2001/06/19 00:25:05 fvdl Exp $	*/
/*	$FreeBSD: src/lib/libc/amd64/string/memmove.S,v 1.2 2011/01/07 16:08:40 kib Exp $ */

#define MEMMOVE
#include "bcopy.S"

	.section .note.GNU-stack,"",%progbist
