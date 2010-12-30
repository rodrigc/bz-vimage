#include <machine/asm.h>
__FBSDID("$FreeBSD: src/lib/csu/arm/crtn.S,v 1.3 2010/12/09 21:31:21 dim Exp $");

	.section .init,"ax",%progbits
	ldmea	fp, {fp, sp, pc}
	mov	pc, lr

	.section .fini,"ax",%progbits
	ldmea	fp, {fp, sp, pc}
	mov	pc, lr
