/*-
 * Copyright (c) 2003 Mike Barcroft <mike@FreeBSD.org>
 * Copyright (c) 2002, 2003 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/libc/ia64/_fpmath.h,v 1.7 2008/01/17 16:39:07 bde Exp $
 */

#include <sys/endian.h>

union IEEEl2bits {
	long double	e;
	struct {
#if _BYTE_ORDER == _LITTLE_ENDIAN
		unsigned int	manl	:32;
		unsigned int	manh	:32;
		unsigned int	exp	:15;
		unsigned int	sign	:1;
		unsigned long	junk	:48;
#else /* _BIG_ENDIAN */
		unsigned long	junk	:48;
		unsigned int	sign	:1;
		unsigned int	exp	:15;
		unsigned int	manh	:32;
		unsigned int	manl	:32;
#endif
	} bits;
	struct {
#if _BYTE_ORDER == _LITTLE_ENDIAN
		unsigned long	man	:64;
		unsigned int	expsign	:16;
		unsigned long	junk	:48;
#else /* _BIG_ENDIAN */
		unsigned long	junk	:48;
		unsigned int	expsign	:16;
		unsigned long	man	:64;
#endif
	} xbits;
};

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	LDBL_NBIT	0x80000000
#define	mask_nbit_l(u)	((u).bits.manh &= ~LDBL_NBIT)
#else /* _BIG_ENDIAN */
/*
 * XXX This doesn't look right.  Very few machines have a different
 *     endianness for integers and floating-point, and in nextafterl()
 *     we assume that none do.  If you have an environment for testing
 *     this, please let me know. --das
 */
#define	LDBL_NBIT	0x80
#define	mask_nbit_l(u)	((u).bits.manh &= ~LDBL_NBIT)
#endif

#define	LDBL_MANH_SIZE	32
#define	LDBL_MANL_SIZE	32

#define	LDBL_TO_ARRAY32(u, a) do {			\
	(a)[0] = (uint32_t)(u).bits.manl;		\
	(a)[1] = (uint32_t)(u).bits.manh;		\
} while (0)