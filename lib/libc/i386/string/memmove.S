#include <machine/asm.h>
__FBSDID("$FreeBSD: src/lib/libc/i386/string/memmove.S,v 1.8 2011/01/07 16:08:40 kib Exp $");

#define MEMMOVE
#include "bcopy.S"

	.section .note.GNU-stack,"",%progbits
