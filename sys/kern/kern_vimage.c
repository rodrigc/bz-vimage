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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/eventhandler.h>
#include <sys/jail.h>
#include <sys/kernel.h>
#include <sys/libkern.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/refcount.h>
#include <sys/rwlock.h>
#include <sys/sx.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/vimage.h>

#ifdef DDB
#include <ddb/ddb.h>
#include <ddb/db_sym.h>
#endif

MALLOC_DEFINE(M_VIMAGE_DATA_FREE, "vimage_data_free",
    "VIMAGE resource accounting");

static struct sx vimage_subsys_data_sxlock;

/* We do not really export it but link_elf.c knows about our guts. */
LIST_HEAD(vimage_subsys_list_head, vimage_subsys) vimage_subsys_head;

/*
 * The vimage subsystem list has two read-write locks, one sleepable and
 * the other not, so that the list can be stablized and walked in a variety
 * of kernel contexts.  Both must be acquired exclusively to modify the list,
 * but a read lock of either lock is sufficient to walk the list.
 */
struct sx		vimage_subsys_sxlock;
struct rwlock		vimage_subsys_rwlock;

#define	VIMAGE_SUBSYS_LIST_WLOCK() do {					\
	sx_xlock(&vimage_subsys_sxlock);				\
	rw_wlock(&vimage_subsys_rwlock);				\
} while (0)

#define	VIMAGE_SUBSYS_LIST_WUNLOCK() do {				\
	rw_wunlock(&vimage_subsys_rwlock);				\
	sx_xunlock(&vimage_subsys_sxlock);				\
} while (0)

/*
 * The virtual subsystem instance list has two read-write locks, one sleepable
 * and the other not, so that the list can be stablized and walked in a variety
 * of contexts.  Both must be acquired exclusively to modify the list, but a
 * read lock of either lock is sufficient to walk the list.
 */
struct rwlock		vimage_list_rwlock;
struct sx		vimage_list_sxlock;

#define	VIMAGE_LIST_WLOCK() do {						\
	sx_xlock(&vimage_list_sxlock);						\
	rw_wlock(&vimage_list_rwlock);						\
} while (0)

#define	VIMAGE_LIST_WUNLOCK() do {					\
	rw_wunlock(&vimage_list_rwlock);					\
	sx_xunlock(&vimage_list_sxlock);					\
} while (0)

/*
 * Global lists of subsystem constructor and destructors for subsystems.
 * They are registered via SUBSYS_SYSINIT() and SUBSYS_SYSUNINIT().  Both
 * lists are protected by the vimage_sysinit_sxlock global lock.
 */
static struct sx	vimage_sysinit_sxlock;

#define	VIMAGE_SYSINIT_WLOCK()		sx_xlock(&vimage_sysinit_sxlock);
#define	VIMAGE_SYSINIT_WUNLOCK()	sx_xunlock(&vimage_sysinit_sxlock);
#define	VIMAGE_SYSINIT_RLOCK()		sx_slock(&vimage_sysinit_sxlock);
#define	VIMAGE_SYSINIT_RUNLOCK()	sx_sunlock(&vimage_sysinit_sxlock);

static void *vimage_data_alloc(struct vimage_subsys *, size_t);
static void vimage_data_free(struct vimage_subsys *, void *, size_t);
static void vimage_data_copy(struct vimage_subsys *, void *, size_t);

static int vimage_data_init_notsupp(struct vimage_subsys * __unused);
static void *vimage_data_alloc_notsupp(struct vimage_subsys * __unused,
    size_t __unused);
static void vimage_data_free_notsupp(struct vimage_subsys * __unused,
    void * __unused, size_t __unused);
static void vimage_data_copy_notsupp(void * __unused, size_t __unused);


/*
 * Boot time intialization of the VIMAGe framework.
 */
static void
vimage_init_prelink(void *arg)
{

	rw_init(&vimage_subsys_rwlock, "vimage_subsys_rwlock");
	sx_init(&vimage_subsys_sxlock, "vimage_subsys_sxlock");
	LIST_INIT(&vimage_subsys_head);

	rw_init(&vimage_list_rwlock, "vimage_list_rwlock");
	sx_init(&vimage_list_sxlock, "vimage_list_sxlock");

	sx_init(&vimage_subsys_data_sxlock, "vimage subsys data alloc lock");

	sx_init(&vimage_sysinit_sxlock, "vimage_sysinit_sxlock");
}
SYSINIT(vimage_init_prelink, SI_SUB_VIMAGE_PRELINK, SI_ORDER_FIRST,
    vimage_init_prelink, NULL);

/*
 * Boot or load time registration of virtualized subsystems.
 */
int
vimage_subsys_register(struct vimage_subsys *vse)
{
	int error;

	KASSERT(vse != NULL, ("%s: vse is NULL", __func__));
#if 0
	/* We cannot do this as we are running too early during boot. */
	KASSERT(curthread->td_ucred->cr_prison == &prison0,
	    ("%s: not called from prison0 td=%p prison=%p, vse=%p", __func__,
	    curthread, curthread->td_ucred->cr_prison, vse));
#endif

	/*
	 * Assert a few things but as we might be called from modules
	 * loaded, do not panic but just log and return an error.
	 * Where possible, initialize to defaults.
	 */
	if (vse->name == NULL || vse->NAME == NULL) {
		printf("%s: name or NAME of vse=%p is NULL\n", __func__, vse);
		return (EINVAL);
	}
	if (vse->setname == NULL) {
		printf("%s: setname of vse=%p is NULL\n", __func__, vse);
		return (EINVAL);
	}
	if (strncmp(vse->setname, "set_", 4)) {
		printf("%s: setname of vse=%p not starting with "
		    "\"set_\": %s\n", __func__, vse, vse->setname);
		return (EINVAL);
	}
	/* The short name is needed for link_elf.c. */
	if (vse->setname_s == NULL)
		vse->setname_s = vse->setname + 4;	/* Skip "set_". */
	if (vse->v_data_init == NULL) {
		/* Disabling dynamic data region/module support. */
		vse->v_data_init = vimage_data_init_notsupp;
		vse->v_data_alloc = vimage_data_alloc_notsupp;
		vse->v_data_free = vimage_data_free_notsupp;
		vse->v_data_copy = vimage_data_copy_notsupp;
		printf("%s: dynamic data region/module support disabled "
		    "for vse=%p %s\n", __func__, vse, vse->setname);
	}
	if (vse->v_data_alloc == NULL)
		vse->v_data_alloc = vimage_data_alloc;
	if (vse->v_data_free == NULL)
		vse->v_data_free = vimage_data_free;
	if (vse->v_data_copy == NULL)
		vse->v_data_copy = vimage_data_copy;

	/* Initialize debugging. */
	SLIST_INIT(&vse->v_recursions);

	/*
	 * Initalize dynamic data region for module support. Callee has to
	 * handle list initialization itself as we do not know varaible names
	 * of the region or extra space that might be available.
	 */
	error = (*vse->v_data_init)(vse);
	if (error)
		return (error);
	
	VIMAGE_SUBSYS_LIST_WLOCK();
	/*
	 * Ensure a subsytem with the setname has not yet been registered.
	 * We do not check for the vse pointer as that would still allow
	 * duplicate subsystem registration.
	 */
	if (vimage_subsys_get(vse->setname) != NULL) {
		VIMAGE_SUBSYS_LIST_WUNLOCK();
		printf("%s: vse=%p duplicate registration for %s\n",
		    __func__, vse, vse->setname);
		return (EEXIST);
	}
	refcount_init(&vse->refcnt, 1);
	LIST_INSERT_HEAD(&vimage_subsys_head, vse, vimage_subsys_le);
	VIMAGE_SUBSYS_LIST_WUNLOCK();

	return (0);
}

int
vimage_subsys_deregister(struct vimage_subsys *vse)
{
	struct vimage_subsys *vse2;
	int error;

	error = ENOENT;
	VIMAGE_SUBSYS_LIST_WLOCK();
	LIST_FOREACH(vse2, &vimage_subsys_head, vimage_subsys_le) {
		if (vse == vse2) {
			if (refcount_release(&vse->refcnt) == 1) {
				LIST_REMOVE(vse, vimage_subsys_le);
				error = 0;
			} else {
				/*
				 * We cannot allow vse to possibly go away
				 * if it is coming from a module.
				 * XXX-BZ catch22?
				 */
				error = EBUSY;
			}
			break;
		}
	}
	VIMAGE_SUBSYS_LIST_WUNLOCK();

	return (error);
}

/*
 * Get a reference to the subsystem entry referenced by linker set name.
 * This interface should be used only by the kernel linker to get access to
 * the v_data_{alloc,free,copy} functions.
 * Caller has to read lock and hold the lock for a save reference to the
 * vimage_subsyst entry.
 */
struct vimage_subsys *
vimage_subsys_get(const char *setname)
{
	struct vimage_subsys *vse;

	LIST_FOREACH(vse, &vimage_subsys_head, vimage_subsys_le) {
		if (!strcmp(setname, vse->setname))
			return (vse);
	}

	return (NULL);
}

/*
 * When a module is loaded and requires storage for a virtualized global
 * variable, allocate space from the modspace free list.  This interface
 * should be used only by the kernel linker via vimage_subsys_get() or
 * link_elf.c::parse_vimage().
 */
static void *
vimage_data_alloc(struct vimage_subsys *vse, size_t size)
{
	struct vimage_data_free *df;
	void *s;

	s = NULL;
	size = roundup2(size, sizeof(void *));
	sx_xlock(&vimage_subsys_data_sxlock);
	TAILQ_FOREACH(df, &vse->v_data_free_list, vnd_link) {
		if (df->vnd_len < size)
			continue;
		if (df->vnd_len == size) {
			s = (void *)df->vnd_start;
			TAILQ_REMOVE(&vse->v_data_free_list, df, vnd_link);
			free(df, M_VIMAGE_DATA_FREE);
			break;
		}
		s = (void *)df->vnd_start;
		df->vnd_len -= size;
		df->vnd_start = df->vnd_start + size;
		break;
	}
	sx_xunlock(&vimage_subsys_data_sxlock);

	return (s);
}

/*
 * Free space for a virtualized global variable on module unload.
 */
static void
vimage_data_free(struct vimage_subsys *vse, void *start_arg, size_t size)
{
	struct vimage_data_free *df;
	struct vimage_data_free *dn;
	uintptr_t start;
	uintptr_t end;

	size = roundup2(size, sizeof(void *));
	start = (uintptr_t)start_arg;
	end = start + size;
	/*
	 * Free a region of space and merge it with as many neighbors as
	 * possible.  Keeping the list sorted simplifies this operation.
	 */
	sx_xlock(&vimage_subsys_data_sxlock);
	TAILQ_FOREACH(df, &vse->v_data_free_list, vnd_link) {
		if (df->vnd_start > end)
			break;
		/*
		 * If we expand at the end of an entry we may have to merge
		 * it with the one following it as well.
		 */
		if (df->vnd_start + df->vnd_len == start) {
			df->vnd_len += size;
			dn = TAILQ_NEXT(df, vnd_link);
			if (df->vnd_start + df->vnd_len == dn->vnd_start) {
				df->vnd_len += dn->vnd_len;
				TAILQ_REMOVE(&vse->v_data_free_list, dn,
				    vnd_link);
				free(dn, M_VIMAGE_DATA_FREE);
			}
			sx_xunlock(&vimage_subsys_data_sxlock);
			return;
		}
		if (df->vnd_start == end) {
			df->vnd_start = start;
			df->vnd_len += size;
			sx_xunlock(&vimage_subsys_data_sxlock);
			return;
		}
	}
	dn = malloc(sizeof(*df), M_VIMAGE_DATA_FREE, M_WAITOK | M_ZERO);
	dn->vnd_start = start;
	dn->vnd_len = size;
	if (df)
		TAILQ_INSERT_BEFORE(df, dn, vnd_link);
	else
		TAILQ_INSERT_TAIL(&vse->v_data_free_list, dn, vnd_link);
	sx_xunlock(&vimage_subsys_data_sxlock);
}

/*
 * When a new virtualized global variable has been allocated, propagate its
 * initial value to each already-allocated virtual network stack instance.
 */
static void
vimage_data_copy(struct vimage_subsys *vse, void *start, size_t size)
{
	struct vimage *v;

	VIMAGE_LIST_RLOCK();
	LIST_FOREACH(v, &vse->v_instance_head, v_le)
		memcpy((void *)((uintptr_t)v->v_data_base +
		    (uintptr_t)start), start, size);
	VIMAGE_LIST_RUNLOCK();
}

static int
vimage_data_init_notsupp(struct vimage_subsys *vse __unused)
{

	return (0);
}

static void *
vimage_data_alloc_notsupp(struct vimage_subsys *vse __unused,
    size_t size __unused)
{

	return (NULL);
}

static void
vimage_data_free_notsupp(struct vimage_subsys *vse __unused,
    void *start_arg __unused, size_t size __unused)
{

}

static void
vimage_data_copy_notsupp(void *start __unused, size_t size __unused)
{

}


/*
 * Invoke all registered subsystem constructors on the current subsystem.  Used
 * during subsystem construction.  The caller is responsible for ensuring the
 * new subsystem is the current subsystem.
 */
void
vimage_sysinit(struct vimage_subsys *vse)
{
	struct vimage_sysinit *vs;

	VIMAGE_SYSINIT_RLOCK();
	TAILQ_FOREACH(vs, &vse->v_sysint_constructors, link) {
		vs->func(vs->arg);
	}
	VIMAGE_SYSINIT_RUNLOCK();
}

/*
 * Invoke all registered subsystem destructors on the current subsystem.  Used
 * during subsystem destruction.  The caller is responsible for ensuring the
 * dying subsystem the current subsystem.
 */
void
vimage_sysuninit(struct vimage_subsys *vse)
{
	struct vimage_sysinit *vs;

	VIMAGE_SYSINIT_RLOCK();
	TAILQ_FOREACH_REVERSE(vs, &vse->v_sysint_destructors,
	    vimage_sysuninit_head, link) {
		vs->func(vs->arg);
	}
	VIMAGE_SYSINIT_RUNLOCK();
}

void
vimage_register_sysinit(void *arg)
{
	struct vimage_sysinit *vs, *vs2;	
	struct vimage_subsys *vse;

	vs = arg;
	vse = vs->v_subsys;
	KASSERT(vs->subsystem > vse->v_sysinit_earliest, ("%s: %s sysinit too "
	    "early: struct vimage_sysinit *=%p 0x%08x <= 0x%08x",
	    __func__, vse->name, vs, vs->subsystem, vse->v_sysinit_earliest));

	/* Add the constructor to the global list of subsystem constructors. */
	VIMAGE_SYSINIT_WLOCK();
	TAILQ_FOREACH(vs2, &vse->v_sysint_constructors, link) {
		if (vs2->subsystem > vs->subsystem)
			break;
		if (vs2->subsystem == vs->subsystem && vs2->order > vs->order)
			break;
	}
	if (vs2 != NULL)
		TAILQ_INSERT_BEFORE(vs2, vs, link);
	else
		TAILQ_INSERT_TAIL(&vse->v_sysint_constructors, vs, link);

	/*
	 * Invoke the constructor on all the existing subsystem instances when
	 * it is registered.
	 */
	(*vse->v_sysinit_iter)(vs);
	VIMAGE_SYSINIT_WUNLOCK();
}

void
vimage_deregister_sysinit(void *arg)
{
	struct vimage_sysinit *vs;
	struct vimage_subsys *vse;

	vs = arg;
	vse = vs->v_subsys;
	/*
	 * Remove the constructor from the global list of subsystem
	 * constructors.
	 */
	VIMAGE_SYSINIT_WLOCK();
	TAILQ_REMOVE(&vse->v_sysint_constructors, vs, link);
	VIMAGE_SYSINIT_WUNLOCK();
}

void
vimage_register_sysuninit(void *arg)
{
	struct vimage_sysinit *vs, *vs2;
	struct vimage_subsys *vse;

	vs = arg;
	vse = vs->v_subsys;

	/* Add the destructor to the global list of subsystem destructors. */
	VIMAGE_SYSINIT_WLOCK();
	TAILQ_FOREACH(vs2, &vse->v_sysint_destructors, link) {
		if (vs2->subsystem > vs->subsystem)
			break;
		if (vs2->subsystem == vs->subsystem && vs2->order > vs->order)
			break;
	}
	if (vs2 != NULL)
		TAILQ_INSERT_BEFORE(vs2, vs, link);
	else
		TAILQ_INSERT_TAIL(&vse->v_sysint_destructors, vs, link);
	VIMAGE_SYSINIT_WUNLOCK();
}

void
vimage_deregister_sysuninit(void *arg)
{
	struct vimage_sysinit *vs;
	struct vimage_subsys *vse;

	vs = arg;
	vse = vs->v_subsys;

	/*
	 * Invoke the destructor on all the existing subsystem instance when
	 * it is deregistered.
	 */
	VIMAGE_SYSINIT_WLOCK();
	(*vse->v_sysinit_iter)(vs);
	/*
	 * Remove the destructor from the global list of subsystem destructors.
	 */
	TAILQ_REMOVE(&vse->v_sysint_destructors, vs, link);
	VIMAGE_SYSINIT_WUNLOCK();
}

/*
 * EVENTHANDLER(9) extensions.
 */
/*
 * Invoke the eventhandler function originally registered with the possibly
 * registered argument for all virtual network stack instances.
 *
 * This iterator can only be used for eventhandlers that do not take any
 * additional arguments, as we do ignore the variadic arguments from the
 * EVENTHANDLER_INVOKE() call.
 */
void
vimage_global_eventhandler_iterator_func(void *arg, ...)
{
	struct vimage *v;
	struct eventhandler_entry_vimage *v_ee;

	/*
	 * There is a bug here in that we should actually cast things to
	 * (struct eventhandler_entry_ ## name *)  but that's not easily
	 * possible in here so just re-using the variadic version we
	 * defined for the generic vimage case.
	 */
	v_ee = arg;
	VIMAGE_LIST_RLOCK();
	VIMAGE_FOREACH(v_ee->vse, v) {
		CURVIMAGE_SET_QUIET(v_ee->vse, __func__, v);
		((vimage_iterator_func_t)v_ee->func)(v_ee->ee_arg);
		CURVIMAGE_RESTORE(v_ee->vse, __func__);
	}
	VIMAGE_LIST_RUNLOCK();
}

static void
vimage_print_recursion(struct vimage_subsys *vse,
    struct vimage_recursion *vr, int brief)
{

	if (!brief)
		printf("CUR%s_SET() recursion in ", vse->NAME);
	printf("%s() line %d, prev in %s()",
	    vr->where_fn, vr->where_line, vr->prev_fn);
	if (brief)
		printf(", ");
	else
		printf("\n    ");
	printf("%p -> %p\n", vr->old_instance, vr->new_instance);
}

void
vimage_log_recursion(struct vimage_subsys *vse,
    void *where_instance, const char *where_fn, int where_line,
    void *old_instance, const char *old_fn)
{
	struct vimage_recursion *vr;

	/* Skip already logged recursion events. */
	SLIST_FOREACH(vr, &vse->v_recursions, vr_le)
		if (vr->prev_fn == old_fn &&
		    vr->where_fn == where_fn &&
		    vr->where_line == where_line &&
		    (vr->old_instance == vr->new_instance) ==
			(where_instance == old_instance))
			    return;

	vr = malloc(sizeof(*vr), M_VIMAGE, M_NOWAIT | M_ZERO);
	if (vr == NULL) {
		printf("%s: malloc failed, not tracking recursion", __func__);
		return;
	}
	vr->prev_fn = old_fn;
	vr->where_fn = where_fn;
	vr->where_line = where_line;
	vr->old_instance = old_instance;
	vr->new_instance = where_instance;

	SLIST_INSERT_HEAD(&vse->v_recursions, vr, vr_le);

	vimage_print_recursion(vse, vr, 0);
#ifdef KDB
	kdb_backtrace();
#endif
}

#ifdef DDB
DB_SHOW_VIMAGE_COMMAND(recursions, db_show_vimage_recursions)
{
	struct vimage_subsys *vse;
	struct vimage_recursion *vr;

	if (!have_addr) {
		db_printf("usage: show vimage recursions "
		    "<struct vimage_subsys *>\n");
		return;
	}
	vse = (struct vimage_subsys *)addr;

	SLIST_FOREACH(vr, &vse->v_recursions, vr_le)
		vimage_print_recursion(vse, vr, 1);
}

static void
db_show_vimage_print_vs(struct vimage_sysinit *vs)
{
	const char *vsname, *funcname;
	c_db_sym_t sym;
	db_expr_t  offset;

	if (vs == NULL) {
		db_printf("%s: no vimage_sysinit * given\n", __func__);
		return;
	}

	sym = db_search_symbol((vm_offset_t)vs, DB_STGY_ANY, &offset);
	db_symbol_values(sym, &vsname, NULL);
	sym = db_search_symbol((vm_offset_t)vs->func, DB_STGY_PROC, &offset);
	db_symbol_values(sym, &funcname, NULL);
	db_printf("%s(%p)\n", (vsname != NULL) ? vsname : "", vs);
	db_printf("  0x%08x 0x%08x\n", vs->subsystem, vs->order);
	db_printf("  %p(%s)(%p)\n",
	    vs->func, (funcname != NULL) ? funcname : "", vs->arg);
}

DB_SHOW_VIMAGE_COMMAND(sysinit, db_show_vimage_sysinit)
{
	struct vimage_subsys *vse;
	struct vimage_sysinit *vs;

	if (!have_addr) {
		db_printf("usage: show vimage sysinit "
		    "<struct vimage_subsys *>\n");
		return;
	}
	vse = (struct vimage_subsys *)addr;

	db_printf("%s_SYSINIT vs Name(Ptr)\n", vse->name);
	db_printf("  Subsystem  Order\n");
	db_printf("  Function(Name)(Arg)\n");
	TAILQ_FOREACH(vs, &vse->v_sysint_constructors, link) {
		db_show_vimage_print_vs(vs);
		if (db_pager_quit)
			break;
	}
}

DB_SHOW_VIMAGE_COMMAND(sysuninit, db_show_vimage_sysuninit)
{
	struct vimage_subsys *vse;
	struct vimage_sysinit *vs;

	if (!have_addr) {
		db_printf("usage: show vimage sysuninit "
		    "<struct vimage_subsys *>\n");
		return;
	}
	vse = (struct vimage_subsys *)addr;

	db_printf("%s_SYSUNINIT vs Name(Ptr)\n", vse->name);
	db_printf("  Subsystem  Order\n");
	db_printf("  Function(Name)(Arg)\n");
	TAILQ_FOREACH_REVERSE(vs, &vse->v_sysint_destructors,
	    vimage_sysuninit_head, link) {
		db_show_vimage_print_vs(vs);
		if (db_pager_quit)
			break;
	}
}

static void
db_show_vimage_print_subsys(struct vimage_subsys *vse)
{
	const char *funcname;
	c_db_sym_t sym;
	db_expr_t  offset;

	if (vse == NULL) {
		db_printf("%s: no vimage_subsys * given\n", __func__);
		return;
	}

#define	V_FPTR(fptr)								\
	sym = db_search_symbol((vm_offset_t)vse->fptr, DB_STGY_PROC, &offset);	\
	db_symbol_values(sym, &funcname, NULL);					\
	db_printf("  %-30s = %s(%p)\n", # fptr ,				\
	    (funcname != NULL) ? funcname : "", vse->fptr);
#define	V_PRINT(name, format)							\
	db_printf("  %-30s = " format "\n", # name, vse->name);
#define	V_PRINT_PTR(name, format)						\
	db_printf("  %-30s = " format "\n", # name, &vse->name);

	db_printf("VIMAGE subsystem %s (%p)\n", vse->name, vse);
	V_PRINT(name, "%s");
	V_PRINT(NAME, "%s");
	V_PRINT(setname, "%s");
	V_PRINT(setname_s, "%s");
	V_PRINT(refcnt, "%d");
	V_PRINT_PTR(v_data_free_list, "%p");
	V_FPTR(v_data_init);
	V_FPTR(v_data_alloc);
	V_FPTR(v_data_free);
	V_FPTR(v_data_copy);
	V_PRINT_PTR(v_sysint_constructors, "%p");
	V_PRINT_PTR(v_sysint_destructors, "%p");
	V_PRINT(v_sysinit_earliest, "0x%07X");
	V_FPTR(v_sysinit_iter);
	V_PRINT_PTR(v_recursions, "%p");
#undef	V_PRINT_PTR
#undef	V_PRINT
#undef	V_FPTR
}

DB_SHOW_VIMAGE_COMMAND(subsys, db_show_vimage_subsys)
{

	if (!have_addr) {
		db_printf("usage: show vimage sysinit "
		    "<struct vimage_subsys *>\n");
		return;
	}
	db_show_vimage_print_subsys((struct vimage_subsys *)addr);
}


DB_SHOW_ALL_COMMAND(vimage_subsys, db_show_all_vimage_subsys)
{
	struct vimage_subsys *vse;

	LIST_FOREACH(vse, &vimage_subsys_head, vimage_subsys_le) {
		db_show_vimage_print_subsys(vse);
		if (db_pager_quit)
			break;
	}
}

#endif

/* end */
