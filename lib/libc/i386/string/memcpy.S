#include <machine/asm.h>
__FBSDID("$FreeBSD: src/lib/libc/i386/string/memcpy.S,v 1.3 2011/01/07 16:08:40 kib Exp $");

#define MEMCOPY
#include "bcopy.S"

	.section .note.GNU-stack,"",%progbits
