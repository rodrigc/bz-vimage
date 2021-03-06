/*	$NetBSD: div.S,v 1.1 2001/06/19 00:25:04 fvdl Exp $	*/

/*-
 * Written by Frank van der Linden (fvdl@wasabisystems.com)
 * Public domain.
 */

#include <machine/asm.h>
__FBSDID("$FreeBSD: src/lib/libc/amd64/stdlib/div.S,v 1.3 2011/01/07 16:08:40 kib Exp $");

ENTRY(div)
	movl	%edi,%eax
	cltd
	idivl	%esi
	salq	$32,%rdx
	orq	%rdx,%rax
	ret
END(div)

	.section .note.GNU-stack,"",%progbits
