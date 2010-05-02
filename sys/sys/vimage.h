/*-
 * Copyright (c) 2009 Jeffrey Roberson <jeff@freebsd.org>
 * Copyright (c) 2009 Robert N. M. Watson
 * Copyright (c) 2010 The FreeBSD Foundation
 * All rights reserved.
 *
 * Portions of this software were developed by CK Software GmbH
 * under sponsorship from the FreeBSD Foundation.
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
 * $FreeBSD$
 */

#ifndef _SYS_VIMAGE_H_
#define	_SYS_VIMAGE_H_

#include <sys/queue.h>

MALLOC_DECLARE(M_VIMAGE_DATA_FREE);

struct vimage_data_free {
	TAILQ_ENTRY(vimage_data_free) vnd_link;
	uintptr_t	vnd_start;
	size_t		vnd_len;
};

/*
 * Caller of vimage_subsys_register() has to intialize setname and,
 * if dynamic data region support for modules is provided,
 * v_data_init() and v_data_copy(). v_data_free_list should be statically
 * initialized in this case.
 * v_data_alloc() and v_data_free() may be overloaded, or the default
 * implementation will be used.
 * Other fields are "private" to the implementation.
 */
struct vimage_subsys {
	LIST_ENTRY(vimage_subsys)	vimage_subsys_le; /* all subsys */

	int				refcnt;

	const char			*setname;	/* set_subsys */
	const char			*name;		/* subsys */

	TAILQ_HEAD(, vimage_data_free)	v_data_free_list;

	int				(*v_data_init )(struct vimage_subsys *);
	void *				(*v_data_alloc)(struct vimage_subsys *,
					    size_t);
	void				(*v_data_free )(struct vimage_subsys *,
					    void *, size_t);
	void				(*v_data_copy )(void *, size_t size);
};

extern struct sx		vimage_subsys_sxlock;
extern struct rwlock		vimage_subsys_rwlock;
#define	VIMAGE_SUBSYS_LIST_RLOCK()	sx_slock(&vimage_subsys_sxlock)
#define	VIMAGE_SUBSYS_LIST_RUNLOCK()	sx_sunlock(&vimage_subsys_sxlock)
#define	VIMAGE_SUBSYS_LIST_RLOCK_NOSLEEP()   rw_rlock(&vimage_subsys_rwlock)
#define	VIMAGE_SUBSYS_LIST_RUNLOCK_NOSLEEP() rw_runlock(&vimage_subsys_rwlock)

int vimage_subsys_register(struct vimage_subsys *);
int vimage_subsys_deregister(struct vimage_subsys *);

struct vimage_subsys *vimage_subsys_get(const char *);

#endif /* _SYS_VIMAGE_H_ */

/* end */
