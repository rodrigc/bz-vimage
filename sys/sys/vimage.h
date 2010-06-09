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

#if defined(_KERNEL) || defined(_WANT_VIMAGE)
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/rwlock.h>
#include <sys/malloc.h>
#include <sys/sx.h>

/*
 * Export the VIMAGE module data free list malloc accounting type as all
 * subsystems will have to handle v_data_init themselves.
 */
MALLOC_DECLARE(M_VIMAGE_DATA);

/*
 * List of free chunks of memory that can be used by the linker to put the
 * virtualized global objects from modules in.
 */
struct vimage_data_free {
	TAILQ_ENTRY(vimage_data_free) vnd_link;
	uintptr_t	vnd_start;
	size_t		vnd_len;
};

/*
 * Virtual sysinit mechanism, allowing virtual subsystem components to declare
 * startup and shutdown methods to be run when virtual subsystem instances are
 * created and destroyed.
 */
#include <sys/kernel.h>

struct vimage_subsys;

struct vimage_sysinit {
	enum sysinit_sub_id		subsystem;
	enum sysinit_elem_order		order;
	sysinit_cfunc_t			func;
	const void			*arg;
	struct vimage_subsys		*v_subsys;
	TAILQ_ENTRY(vimage_sysinit)	link;
};
TAILQ_HEAD(vimage_sysinit_head, vimage_sysinit);
TAILQ_HEAD(vimage_sysuninit_head, vimage_sysinit);

/*
 * Tracking recursive set operations of subsystem instances.
 */
struct vimage_recursion {
	SLIST_ENTRY(vimage_recursion)	vr_le;
	const char			*prev_fn;
	const char			*where_fn;
	int				where_line;
	void				*old_instance;
	void				*new_instance;
};
SLIST_HEAD(vimage_recursion_head, vimage_recursion);

/*
 * VIMAGE allocator.
 */

/*
 * Basic description of each virtualized subsystem instance.  Each virtualized
 * subsytem may overload this and add more per-subsystem data to the end.
 * The VIMAGE allocator framework will allocate v_instance_size bytes.
 */
struct vimage {
	uintptr_t		 v_data_base;
	void			*v_data_mem;
	LIST_ENTRY(vimage)	 v_le;		/* all instance list */
	/*
	 * A subsystem might want to allocate more memory to
	 * hold other per subsystem per instance private data.
	 */
	uintptr_t		v_data[];
};
LIST_HEAD(vimage_instance_head, vimage);

/*
 * Read locks to access the list of all per-subsystem instances.  If a caller
 * may sleep while accessing the list, it must use the sleepable lock macros.
 */
#ifdef VIMAGE
extern struct rwlock vimage_list_rwlock;
extern struct sx vimage_list_sxlock;

#define	VIMAGE_LIST_RLOCK()		sx_slock(&vimage_list_sxlock)
#define	VIMAGE_LIST_RUNLOCK()		sx_sunlock(&vimage_list_sxlock)
#define	VIMAGE_LIST_RLOCK_NOSLEEP()	rw_rlock(&vimage_list_rwlock)
#define	VIMAGE_LIST_RUNLOCK_NOSLEEP()	rw_runlock(&vimage_list_rwlock)
#else /* !VIMAGE */
#define	VIMAGE_LIST_RLOCK()
#define	VIMAGE_LIST_RUNLOCK()
#define	VIMAGE_LIST_RLOCK_NOSLEEP()
#define	VIMAGE_LIST_RUNLOCK_NOSLEEP()
#endif /* VIMAGE */

/*
 * Caller of vimage_subsys_register() has to intialize name, NAME,
 * setname, v_symprefix, v_start, v_stop, v_curvar, v_curvar_lpush,
 * v_instance_size and v_sysinit_earliest.
 *
 * If dynamic per module data regions are supported, v_data_init() needs to
 * be implemented and initialized by the subystem implementation as well.
 * v_data_free_list should be statically initialized in this case.
 * v_data_alloc(), v_data_free() and v_data_copy() may be overloaded,
 * or the default implementation will be used.
 *
 * Other fields are "private" to the implementation and will be set from
 * the VIMAGE framework to save per-subsystem state.
 */
struct vimage_subsys {
	LIST_ENTRY(vimage_subsys)	vimage_subsys_le; /* all subsys */

	int				refcnt;

	const char			*name;		/* printfs */
	const char			*NAME;		/* printfs */

	/* VIMAGE allocator framework. */
	const char			*setname;	/* set_subsys */
	const char			*setname_s;	/* subsys */
	const char			*v_symprefix;	/* symbol name prefix */

	uintptr_t			v_start;
	uintptr_t			v_stop;
	size_t				v_size;

	size_t				v_curvar;
	size_t				v_curvar_lpush;

	size_t				v_instance_size;/* instance struct */
	struct vimage_instance_head	v_instance_head;/* instance list */

	/* ddb */
	void				*v_db_instance;	/* ddb instance */

	/* Dynamic/module data allocator. */
	TAILQ_HEAD(, vimage_data_free)	v_data_free_list;

	int				(*v_data_init )(struct vimage_subsys *);
	void				(*v_data_destroy)(
					    struct vimage_subsys *);
	void *				(*v_data_alloc)(struct vimage_subsys *,
					    size_t);
	void				(*v_data_free )(struct vimage_subsys *,
					    void *, size_t);
	void				(*v_data_copy )(struct vimage_subsys *,
					    void *, size_t);

	/* System initialization framework. */
	struct vimage_sysinit_head	v_sysint_constructors;
	struct vimage_sysuninit_head	v_sysint_destructors;

	int				v_sysinit_earliest;

	/* Debugging. */
	struct vimage_recursion_head	v_recursions;
};

#ifdef VIMAGE
/* Only exposed for linker and ddb. */
extern LIST_HEAD(vimage_subsys_list_head, vimage_subsys) vimage_subsys_head;

struct vimage_subsys *vimage_subsys_hold(struct vimage_subsys *);
void vimage_subsys_free(struct vimage_subsys *);

int vimage_subsys_register(struct vimage_subsys *);
int vimage_subsys_deregister(struct vimage_subsys *);

struct vimage_subsys *vimage_subsys_get(const char *);

struct vimage *vimage_alloc(struct vimage_subsys *);
void vimage_destroy(struct vimage_subsys *, struct vimage *);

/*
 * Support for special SYSINIT handlers registered via VIMAGE_SYSINIT()
 * and VIMAGE_SYSUNINIT() or per-subsystem macros overloading these.
 */
extern struct sx		vimage_subsys_sxlock;
extern struct rwlock		vimage_subsys_rwlock;
#define	VIMAGE_SUBSYS_LIST_RLOCK()	sx_slock(&vimage_subsys_sxlock)
#define	VIMAGE_SUBSYS_LIST_RUNLOCK()	sx_sunlock(&vimage_subsys_sxlock)
#define	VIMAGE_SUBSYS_LIST_RLOCK_NOSLEEP()   rw_rlock(&vimage_subsys_rwlock)
#define	VIMAGE_SUBSYS_LIST_RUNLOCK_NOSLEEP() rw_runlock(&vimage_subsys_rwlock)

#define	VIMAGE_SYSINIT(ident, subsystem, order, func, arg, name, v_subsys) \
	static struct vimage_sysinit ident ## _ ## name ## _init = {	\
		subsystem,						\
		order,							\
		(sysinit_cfunc_t)(sysinit_nfunc_t)func,			\
		(arg),							\
		(v_subsys)						\
	};								\
	SYSINIT(name ## _init_ ## ident, subsystem, order,		\
	    vimage_register_sysinit, &ident ## _ ## name ## _init);	\
	SYSUNINIT(name ## _init_ ## ident, subsystem, order,		\
	    vimage_deregister_sysinit, &ident ## _ ## name ## _init)

#define	VIMAGE_SYSUNINIT(ident, subsystem, order, func, arg, name, v_subsys) \
	static struct vimage_sysinit ident ## _ ## name ## _uninit = {	\
		subsystem,						\
		order,							\
		(sysinit_cfunc_t)(sysinit_nfunc_t)func,			\
		(arg),							\
		(v_subsys)						\
	};								\
	SYSINIT(name ## _uninit_ ## ident, subsystem, order,		\
	    vimage_register_sysuninit, &ident ## _ ## name ## _uninit);	\
	SYSUNINIT(name ## _uninit_ ## ident, subsystem, order,		\
	    vimage_deregister_sysuninit, &ident ## _ ## name ## _uninit)
#else /* !VIMAGE */
/*
 * When VIMAGE isn't compiled into the kernel, <SUBSYS>_SYSINIT/
 * <SUBSYS>_SYSUNINIT map into normal sysinits, which have the same
 * ordering properties.
 */
#define	VIMAGE_SYSINIT(ident, subsystem, order, func, arg, name, v_subsys) \
	SYSINIT(ident, subsystem, order, func, arg)
#define	VIMAGE_SYSUNINIT(ident, subsystem, order, func, arg, name, v_subsys) \
	SYSUNINIT(ident, subsystem, order, func, arg)
#endif /* VIMAGE */

#ifdef VIMAGE
void vimage_sysinit(struct vimage_subsys *);
void vimage_sysuninit(struct vimage_subsys *);
void vimage_register_sysinit(void *);
void vimage_deregister_sysinit(void *);
void vimage_register_sysuninit(void *);
void vimage_deregister_sysuninit(void *);

/*
 * Set and unset operations to tell the kernel on which instance the
 * current thread is working on, so that the VIMAGE allocator framework
 * can access the correct copy of global objects.
 */
void vimage_log_recursion(struct vimage_subsys *,
    void *, const char *, int, void *, const char *);
#endif /* VIMAGE */

#if defined(INVARIANTS) || defined(VIMAGE_DEBUG)
#define	VIMAGE_ASSERT(exp, msg)	KASSERT(exp, msg)
#else
#define	VIMAGE_ASSERT(exp, msg)	do { } while (0)
#endif

#ifdef VIMAGE
#include "opt_kdtrace.h"
#include <sys/sdt.h>

SDT_PROVIDER_DECLARE(vimage);
SDT_PROBE_DECLARE(vimage, macros, curvimage, set);
SDT_PROBE_DECLARE(vimage, macros, curvimage, restore);
#define	curinstance(vse)						\
	*((void **)((uintptr_t)curthread + (vse)->v_curvar))
#define	_CURVIMAGE_SET_QUIET(vse, ident, arg)				\
	VIMAGE_ASSERT((arg) != NULL, ("CUR%s_SET at %s:%d %s() "	\
	    "cur%s=%p %s=%p", (vse)->NAME, __FILE__, __LINE__, __func__,\
	    (vse)->name, curinstance(vse), (vse)->name, (arg)));	\
	struct vimage *saved_ ## ident = curinstance(vse);		\
	curinstance(vse) = (void *)arg;					\
	SDT_PROBE5(vimage, macros, curvimage, set,			\
	    vse, __func__, __LINE__, saved_ ## ident, arg)
#define	_CURVIMAGE_RESTORE(vse, ident)					\
	VIMAGE_ASSERT(curinstance(vse) != NULL,				\
	    ("CUR%s_RESTORE at %s:%d %s() cur%s=%p saved_%s=%p",	\
	    (vse)->NAME, __FILE__, __LINE__, __func__, (vse)->name,	\
	    curinstance(vse), (vse)->name, saved_ ## ident));		\
	SDT_PROBE5(vimage, macros, curvimage, restore,			\
	    vse, __func__, __LINE__, curinstance(vse), saved_ ## ident);\
	curinstance(vse) = (void *)saved_ ## ident
#ifdef VIMAGE_DEBUG
#define	curinstance_lpush(vse)						\
	*((const char **)((uintptr_t)curthread + (vse)->v_curvar_lpush))
#define	CURVIMAGE_SET_QUIET(vse, ident, arg)				\
	_CURVIMAGE_SET_QUIET(vse, ident, arg);				\
	const char *saved_ ## ident ##_lpush = curinstance_lpush(vse);	\
	curinstance_lpush(vse) = __func__
#define	CURVIMAGE_SET(vse, ident, arg)					\
	CURVIMAGE_SET_QUIET(vse, ident, arg);				\
	if (saved_ ## ident != NULL)					\
		vimage_log_recursion(vse,				\
		    curinstance(vse), __func__, __LINE__,		\
		    saved_ ## ident, saved_ ## ident ## _lpush)
#define	CURVIMAGE_RESTORE(vse, ident)					\
	_CURVIMAGE_RESTORE(vse, ident);					\
	curinstance_lpush(vse) = saved_ ## ident ## _lpush
#else /* !VIMAGE_DEBUG */
#define	CURVIMAGE_SET_QUIET(vse, ident, arg)				\
	_CURVIMAGE_SET_QUIET(vse, ident, arg)
#define	CURVIMAGE_SET(vse, ident, arg)					\
	CURVIMAGE_SET_QUIET(vse, ident, arg)
#define	CURVIMAGE_RESTORE(vse, ident)					\
	_CURVIMAGE_RESTORE(vse, ident)
#endif /* VIMAGE_DEBUG */
#else /*!VIMAGE */
#define	CURVIMAGE_SET_QUIET(vse, ident, arg)	do { } while(0)
#define	CURVIMAGE_SET(vse, ident, arg)		do { } while(0)
#define	CURVIMAGE_RESTORE(vse, ident)		do { } while(0)
#endif /* VIMAGE */


#ifdef VIMAGE
#define	VIMAGE_FOREACH(vse, arg)					\
	LIST_FOREACH((arg), &(vse)->v_instance_head, v_le)
#else /*!VIMAGE */
#define	VIMAGE_FOREACH(vse, arg)
#endif /* VIMAGE */


#ifdef SYSCTL_OID
#ifdef VIMAGE
/*
 * Sysctl variants for subsystem-virtualized global variables.  Include
 * <sys/sysctl.h> to expose these definitions.
 */
#define	req_to_curinstance(req, vse)					\
	*((void **)((uintptr_t)(req)->td + (vse)->v_curvar))
#define	VIMAGE_SYSCTL_INT(parent, nbr, name, access, ptr, val, descr,	\
	    v_access)							\
	SYSCTL_INT(parent, nbr, name, (v_access)|(access), ptr, val,	\
	    descr)
#define	VIMAGE_SYSCTL_OPAQUE(parent, nbr, name, access, ptr, len, fmt,	\
	    descr, v_access)						\
	SYSCTL_OPAQUE(parent, nbr, name, (v_access)|(access), ptr, len,	\
	    fmt, descr)
#define	VIMAGE_SYSCTL_PROC(parent, nbr, name, access, ptr, arg, handler,\
	    fmt, descr, v_access)					\
	SYSCTL_PROC(parent, nbr, name, (v_access)|(access), ptr, arg,	\
	    handler, fmt, descr)
#define	VIMAGE_SYSCTL_STRING(parent, nbr, name, access, arg, len, descr,\
	    v_access)							\
	SYSCTL_STRING(parent, nbr, name, (v_access)|(access), arg, len,	\
	    descr)
#define	VIMAGE_SYSCTL_STRUCT(parent, nbr, name, access, ptr, type,	\
	    descr, v_access)						\
	SYSCTL_STRUCT(parent, nbr, name, (v_access)|(access), ptr, type,\
	    descr)
#define	VIMAGE_SYSCTL_UINT(parent, nbr, name, access, ptr, val, descr,	\
	    v_access)							\
	SYSCTL_UINT(parent, nbr, name, (v_access)|(access), ptr, val,	\
	    descr)
#define	VIMAGE_SYSCTL_ARG(req, arg1, vse) do {				\
	if (arg1 != NULL) {						\
		struct vimage *v;					\
									\
		v = (struct vimage *)req_to_curinstance(req, vse);	\
		arg1 = (void *)(v->v_data_base + (uintptr_t)(arg1));	\
	}								\
} while (0)
#else /* !VIMAGE */
/*
 * When VIMAGE isn't compiled into the kernel, virtaulized SYSCTLs simply
 * become normal SYSCTLs.
 */
#define	VIMAGE_SYSCTL_INT(parent, nbr, name, access, ptr, val, descr,	\
	    v_access)							\
	SYSCTL_INT(parent, nbr, name, access, ptr, val, descr)
#define	VIMAGE_SYSCTL_OPAQUE(parent, nbr, name, access, ptr, len, fmt,	\
	    descr, v_access)						\
	SYSCTL_OPAQUE(parent, nbr, name, access, ptr, len, fmt, descr)
#define	VIMAGE_SYSCTL_PROC(parent, nbr, name, access, ptr, arg, handler,\
	    fmt, descr, v_access)					\
	SYSCTL_PROC(parent, nbr, name, access, ptr, arg, handler, fmt,	\
	    descr)
#define	VIMAGE_SYSCTL_STRING(parent, nbr, name, access, arg, len, descr,\
	    v_access)							\
	SYSCTL_STRING(parent, nbr, name, access, arg, len, descr)
#define	VIMAGE_SYSCTL_STRUCT(parent, nbr, name, access, ptr, type,	\
	    descr, v_access)						\
	SYSCTL_STRUCT(parent, nbr, name, access, ptr, type, descr)
#define	VIMAGE_SYSCTL_UINT(parent, nbr, name, access, ptr, val, descr,	\
	    v_access)							\
	SYSCTL_UINT(parent, nbr, name, access, ptr, val, descr)
#define	VIMAGE_SYSCTL_ARG(req, arg1, vse)
#endif /* VIMAGE */
#endif /* SYSCTL_OID */

/*
 * EVENTHANDLER(9) extensions.
 */
#include <sys/eventhandler.h>

void	vimage_global_eventhandler_iterator_func(void *, ...);

/*
 * Per subsystem linker-set declarations that will put a set into a dedicated
 * ELF section.
 */

#ifdef VIMAGE
#if defined(__arm__)
#define	_PROGBITS	"%progbits"
#else
#define	_PROGBITS	"@progbits"
#endif
#define	DECLARE_LINKER_SET(SETNAME)					\
__asm__(								\
	".section " #SETNAME ", \"aw\", " _PROGBITS "\n"		\
	"\t.p2align " __XSTRING(CACHE_LINE_SHIFT) "\n"			\
	"\t.previous");							\
extern uintptr_t	*__start_ ## SETNAME;				\
extern uintptr_t	*__stop_ ## SETNAME
#endif /* VIMAGE */

#endif /* defined(_KERNEL) || defined(_WANT_VIMAGE) */
#endif /* _SYS_VIMAGE_H_ */

/* end */
