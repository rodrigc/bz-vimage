/* $FreeBSD: src/lib/bind/dns/dns/enumclass.h,v 1.9 2011/02/06 22:46:07 dougb Exp $ */

/*
 * Copyright (C) 2004-2011 Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2003 Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/***************
 ***************
 ***************   THIS FILE IS AUTOMATICALLY GENERATED BY gen.c.
 ***************   DO NOT EDIT!
 ***************
 ***************/

/*! \file */

#ifndef DNS_ENUMCLASS_H
#define DNS_ENUMCLASS_H 1

enum {
	dns_rdataclass_reserved0 = 0,
#define dns_rdataclass_reserved0 \
				((dns_rdataclass_t)dns_rdataclass_reserved0)
	dns_rdataclass_in = 1,
#define dns_rdataclass_in	((dns_rdataclass_t)dns_rdataclass_in)
	dns_rdataclass_chaos = 3,
#define dns_rdataclass_chaos	((dns_rdataclass_t)dns_rdataclass_chaos)
	dns_rdataclass_ch = 3,
#define dns_rdataclass_ch	((dns_rdataclass_t)dns_rdataclass_ch)
	dns_rdataclass_hs = 4,
#define dns_rdataclass_hs	((dns_rdataclass_t)dns_rdataclass_hs)
	dns_rdataclass_none = 254,
#define dns_rdataclass_none	((dns_rdataclass_t)dns_rdataclass_none)
	dns_rdataclass_any = 255
#define dns_rdataclass_any	((dns_rdataclass_t)dns_rdataclass_any)
};

#endif /* DNS_ENUMCLASS_H */
