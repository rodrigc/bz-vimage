/*	$NetBSD: memcpy.S,v 1.1 2005/12/20 19:28:50 christos Exp $	*/

#include <machine/asm.h>
__FBSDID("$FreeBSD: src/lib/libc/mips/string/memcpy.S,v 1.1 2008/04/26 12:08:01 imp Exp $");

#define MEMCOPY
#include "bcopy.S"
