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
#include "opt_kdb.h"
#include "opt_kdtrace.h"

#include <sys/param.h>
#include <sys/eventhandler.h>
#include <sys/jail.h>
#include <sys/kdb.h>
#include <sys/kernel.h>
#include <sys/libkern.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/refcount.h>
#include <sys/rwlock.h>
#include <sys/sdt.h>
#include <sys/sx.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/vimage.h>

#ifdef DDB
#include <ddb/ddb.h>
#include <ddb/db_lex.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#endif

/*-
 * This file implements core functions for the VIMAGE kernel virtualization
 * framework:
 *
 * - Functions to register, deregister and query for a virtualized subsystem.
 *
 * - Virtualized subsystem memory allocator to start and teardown instances,
 *   as well as functions to handle (alloc/copy/free) modules space.
 *
 * - Virtualized SYSINITs/SYSUNINITs, which allow subsystems to register
 *   startup/shutdown events to be run for each instance.
 *
 * - Eventhandler helper function.
 *
 * - Functions to help with debugging recursive set operations of subsystem
 *   instances.
 *
 * - General interaction kernel debugger (ddb) functions.
 */

MALLOC_DEFINE(M_VIMAGE, "vimage", "VIMAGE resource accounting");
MALLOC_DEFINE(M_VIMAGE_DATA, "vimage_data", "VIMAGE data resource accounting");

/*
 * List of all registered virtualized subsystems.
 * We do not really export it but link_elf.c and ddb know about our guts.
 */
struct vimage_subsys_list_head vimage_subsys_head;

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
 *
 * For the moment the same locks are shared between all virtualized subsystems
 * to traverse their instances.  We really only need to aquire the write locks
 * on startup or teardown of an instance which is a very rare event. Should
 * it cause problems we can make them per-subsytem easily as long as subsystems
 * continue to define their own version of the rlock macros, just calling the
 * default ones.
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
 * Virtual subsystem instance data allocator (module space) lock to protect the
 * free space lists.
 */
static struct sx vimage_subsys_data_sxlock;

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

/*
 * DTrace support.
 */
SDT_PROVIDER_DEFINE(vimage);

/*
 * Defines for the instance allocation and free framework.  Those were needed
 * as the FBT provider missed function exists for some reason if compiled at
 * certain optimization levels.
 */
SDT_PROBE_DEFINE1(vimage, functions, vimage_alloc, entry, "int");
SDT_PROBE_DEFINE2(vimage, functions, vimage_alloc, alloc, "int",
    "struct vimage *");
SDT_PROBE_DEFINE2(vimage, functions, vimage_alloc, return, "int",
    "struct vimage *");
SDT_PROBE_DEFINE2(vimage, functions, vimage_destroy, entry, "int",
    "struct vimage *");
SDT_PROBE_DEFINE1(vimage, functions, vimage_destroy, return, "int");
SDT_PROBE_DEFINE5(vimage, macros, curvimage, set, "struct vimage_subsys *",
    "char *", "int", "struct vimage *", "struct vimage *");
SDT_PROBE_DEFINE5(vimage, macros, curvimage, restore, "struct vimage_subsys *",
    "char *", "int", "struct vimage *", "struct vimage *");

static struct vimage_subsys *vimage_subsys_get_locked(const char *);

static void vimage_data_destroy(struct vimage_subsys *);
static void *vimage_data_alloc(struct vimage_subsys *, size_t);
static void vimage_data_free(struct vimage_subsys *, void *, size_t);
static void vimage_data_copy(struct vimage_subsys *, void *, size_t);

static int vimage_data_init_notsupp(struct vimage_subsys * __unused);
static void vimage_data_destroy_notsupp(struct vimage_subsys * __unused);
static void *vimage_data_alloc_notsupp(struct vimage_subsys * __unused,
    size_t __unused);
static void vimage_data_free_notsupp(struct vimage_subsys * __unused,
    void * __unused, size_t __unused);
static void vimage_data_copy_notsupp(struct vimage_subsys * __unused,
    void * __unused, size_t __unused);

/*
 * Boot time intialization of the VIMAGE framework.
 */
static void
vimage_init_prelink(void *arg)
{

	LIST_INIT(&vimage_subsys_head);
	rw_init(&vimage_subsys_rwlock, "vimage_subsys_rwlock");
	sx_init(&vimage_subsys_sxlock, "vimage_subsys_sxlock");
	rw_init(&vimage_list_rwlock, "vimage_list_rwlock");
	sx_init(&vimage_list_sxlock, "vimage_list_sxlock");
	sx_init(&vimage_subsys_data_sxlock, "vimage subsys data alloc lock");
	sx_init(&vimage_sysinit_sxlock, "vimage_sysinit_sxlock");
}
SYSINIT(vimage_init_prelink, SI_SUB_VIMAGE_PRELINK, SI_ORDER_FIRST,
    vimage_init_prelink, NULL);

/*
 * Wrapper functions around the reference count.
 */
struct vimage_subsys *
vimage_subsys_hold(struct vimage_subsys *vse)
{

	refcount_acquire(&vse->refcnt);
	return (vse);
}

void
vimage_subsys_free(struct vimage_subsys *vse)
{

	if (refcount_release(&vse->refcnt)) {
#ifdef DDB
		db_vimage_variable_unregister(vse);
#endif
		(*vse->v_data_destroy)(vse);
	}
}

/*
 * Boot or load time registration of virtualized subsystems.
 */
int
vimage_subsys_register(struct vimage_subsys *vse)
{
	struct vimage_subsys *vse2;
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
	 * loaded, do not panic but just log and return an error and
	 * leave it to the caller to panic or not.  Where possible,
	 * initialize to defaults.
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
		vse->v_data_destroy = vimage_data_destroy_notsupp;
		vse->v_data_alloc = vimage_data_alloc_notsupp;
		vse->v_data_free = vimage_data_free_notsupp;
		vse->v_data_copy = vimage_data_copy_notsupp;
		printf("%s: dynamic data region/module support disabled "
		    "for vse=%p %s\n", __func__, vse, vse->setname);
	}
	if (vse->v_data_destroy == NULL)
		vse->v_data_destroy = vimage_data_destroy;
	if (vse->v_data_alloc == NULL)
		vse->v_data_alloc = vimage_data_alloc;
	if (vse->v_data_free == NULL)
		vse->v_data_free = vimage_data_free;
	if (vse->v_data_copy == NULL)
		vse->v_data_copy = vimage_data_copy;

	/* Initialize size and always force page boundries. */
	vse->v_size = roundup2(vse->v_stop - vse->v_start, PAGE_SIZE);

	/* Initialize debugging. */
	SLIST_INIT(&vse->v_recursions);

	/*
	 * Initalize dynamic data region for module support. Callee has to
	 * handle list initialization itself as we do not know variable names
	 * of the region or about extra space that might be available.
	 */
	error = (*vse->v_data_init)(vse);
	if (error)
		return (error);

#ifdef DDB
	/*
	 * Let ddb know about the subsystem and synamicly create $db_<name>
	 * and cur<name> variables to debug an instance of this subsystem.
	 */
	db_vimage_variable_register(vse);
#endif
	
	/*
	 * Ensure a subsytem with the setname has not yet been registered.
	 * We do not check for the vse pointer as that would still allow
	 * duplicate subsystem registration.
	 * The reason we pick the setname is that this is the unique name
	 * needed for and used by the linker.
	 */
	VIMAGE_SUBSYS_LIST_WLOCK();
	vse2 = vimage_subsys_get_locked(vse->setname);
	if (vse2 != NULL) {
		vimage_subsys_free(vse2);
		VIMAGE_SUBSYS_LIST_WUNLOCK();
#ifdef DDB
		db_vimage_variable_unregister(vse);
#endif
		(*vse->v_data_destroy)(vse);
		printf("%s: vse=%p duplicate registration for %s\n",
		    __func__, vse, vse->setname);
		return (EEXIST);
	}
	refcount_init(&vse->refcnt, 1);
	LIST_INSERT_HEAD(&vimage_subsys_head, vse, vimage_subsys_le);
	VIMAGE_SUBSYS_LIST_WUNLOCK();

	return (0);
}

/*
 * Disconnect the virtualized subsystem.  If the subsystem is not registered
 * ENOENT is returned.  If the subsystem cannot be disconnected EBUSY is
 * returned and any other unloads, e.g. for modules implementation the
 * virtualized subsystem, must fail as well.  Upon success 0 is returned.
 */
int
vimage_subsys_deregister(struct vimage_subsys *vse)
{
	struct vimage_subsys *vse2;
	int error;

	error = ENOENT;
	VIMAGE_SUBSYS_LIST_WLOCK();
	LIST_FOREACH(vse2, &vimage_subsys_head, vimage_subsys_le) {
		if (vse == vse2) {
			u_int old;

			old = atomic_fetchadd_int(&vse->refcnt, 0);
			if (old == 1) {
				LIST_REMOVE(vse, vimage_subsys_le);
				vimage_subsys_free(vse);
				error = 0;
			} else {
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
static struct vimage_subsys *
vimage_subsys_get_locked(const char *setname)
{
	struct vimage_subsys *vse;

	/* XXX Cannot assert either or possibly non-exclusive lock. */
	LIST_FOREACH(vse, &vimage_subsys_head, vimage_subsys_le) {
		if (!strcmp(setname, vse->setname)) {
			vimage_subsys_hold(vse);
			return (vse);
		}
	}

	return (NULL);
}

struct vimage_subsys *
vimage_subsys_get(const char *setname)
{
	struct vimage_subsys *vse;

	VIMAGE_SUBSYS_LIST_RLOCK();
	vse = vimage_subsys_get_locked(setname);
	VIMAGE_SUBSYS_LIST_RUNLOCK();

	return (vse);
}

/*
 * The VIMAGE allocator provides storage for virtualized global variables
 * of a virtualized subsystem.  These variables are defined/declared using the
 * <SUBSYS>_DEFINE()/<SUBSYS>_DECLARE() macros, which place them in the
 * 'set_<name>' linker set.  The details of the implementation are somewhat
 * subtle, but allow the majority of most subsystems to maintain
 * virtualization-agnostic.
 *
 * The VIMAGE allocator handles variables in the base kernel vs. modules in
 * similar but different ways.  In both cases, virtualized global variables
 * are marked as such by being declared to be part of the substem's linker set.
 * These "master" copies of global variables serve two functions:
 *
 * (1) They contain static initialization or "default" values for global
 *     variables which will be propagated to each subsytem instance when
 *     created.  As with normal global variables, they default to zero-filled.
 *
 * (2) They act as unique global names by which the variable can be referred
 *     to, regardless of subsystem instance.  The single global symbol will
 *     be used to calculate the location of a per-virtual instance variable
 *     at run-time.
 *
 * Each virtual subsystem instance has a complete copy of each virtualized
 * global variable, stored in a malloc'd block of memory referred to by
 * <struct vimage *>->v_data_mem.  Critical to the design is that each
 * per-instance memory block is laid out identically to the master block so
 * that the offset of each global variable is the same across all blocks.  To
 * optimize run-time access, a precalculated 'base' address,
 * <struct vimage *>->v_data_base, is stored in each instance, and is the
 * amount that can be added to the address of a 'master' instance of a
 * variable to get to the per-subsystem instance.
 *
 * Virtualized global variables from modules are handled in a similar manner,
 * but as each module has its own 'set_<name>' linker set, and we want to keep
 * all virtualized globals togther, we reserve space in the kernel's linker set
 * for potential module variables using a per-subsystem character array, usually
 * called 'modspace' but not necessarily as the v_data_init function is
 * subsystem private and handles this internally.  The virtual subsystem
 * allocator maintains a free list to track what space in the array is free
 * (all, initially) and as modules are linked, allocates portions of the space
 * to specific globals.  The kernel module linker queries the subsystem
 * allocator and will bind references of the global to the location during
 * linking.  It also calls into the subsystem allocator, once the memory is
 * initialized, in order to propagate the new static initializations to all
 * existing subsystem instances so that the soon-to-be executing module will
 * find every subsystem instance with proper default values.
 */

/*
 * Allocate a virtual instance.
 */
struct vimage *
vimage_alloc(struct vimage_subsys *vse)
{
	struct vimage *v;

	SDT_PROBE1(vimage, functions, vimage_alloc, entry, __LINE__);

	/*
	 * Check that the subsystem is registered.  If so, lend the referece
	 * to the newly allocated vimage.  This will avoid instance creation
	 * after the subsystem has been unregistered.
	 */
	v = NULL;
	if (vimage_subsys_get(vse->setname) == NULL)
		goto err;

	v = malloc(vse->v_instance_size, M_VIMAGE, M_WAITOK | M_ZERO);
	SDT_PROBE2(vimage, functions, vimage_alloc, alloc, __LINE__, v);

	/*
	 * Allocate storage for virtualized global variables and copy in
	 * initial values from our 'master' copy.
	 */
	v->v_data_mem = malloc(vse->v_size, M_VIMAGE_DATA, M_WAITOK);
	memcpy(v->v_data_mem, (void *)vse->v_start, vse->v_size);

	/*
	 * All use of subsystem-specific data will immediately subtract start
	 * from the base memory pointer, so pre-calculate that now to avoid
	 * it on each use.
	 */
	v->v_data_base = (uintptr_t)v->v_data_mem - vse->v_start;

	/* Initialize / attach subsystem module instances. */
	CURVIMAGE_SET_QUIET(vse, __func__, v);
	vimage_sysinit(vse);
	CURVIMAGE_RESTORE(vse, __func__);

	VIMAGE_LIST_WLOCK();
	LIST_INSERT_HEAD(&vse->v_instance_head, v, v_le);
	VIMAGE_LIST_WUNLOCK();

err:
	SDT_PROBE2(vimage, functions, vimage_alloc, return, __LINE__, v);
	return (v);
}

/*
 * Destroy a virtual instance.
 */
void
vimage_destroy(struct vimage_subsys *vse, struct vimage *v)
{

	SDT_PROBE2(vimage, functions, vimage_destroy, entry, __LINE__, v);

	VIMAGE_LIST_WLOCK();
	LIST_REMOVE(v, v_le);
	VIMAGE_LIST_WUNLOCK();

	CURVIMAGE_SET_QUIET(vse, __func__, v);
	vimage_sysuninit(vse);
	CURVIMAGE_RESTORE(vse, __func__);

	/*
	 * Release storage for the virtual instance (module) data allocator.
	 */
	free(v->v_data_mem, M_VIMAGE_DATA);
	free(v, M_VIMAGE);

	/*
	 * Release the reference borrowed from the vimage subsystem.
	 */
	vimage_subsys_free(vse);
	SDT_PROBE1(vimage, functions, vimage_destroy, return, __LINE__);
}

/*
 * Destroy the modspace free list.
 */
static void
vimage_data_destroy(struct vimage_subsys *vse)
{
	struct vimage_data_free *df, *tdf;

	sx_xlock(&vimage_subsys_data_sxlock);
	TAILQ_FOREACH_SAFE(df, &vse->v_data_free_list, vnd_link, tdf) {
		TAILQ_REMOVE(&vse->v_data_free_list, df, vnd_link);
		free(df, M_VIMAGE_DATA);
	}
	sx_xunlock(&vimage_subsys_data_sxlock);
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
			free(df, M_VIMAGE_DATA);
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
 * Free space for virtualized global variables on module unload.
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
				free(dn, M_VIMAGE_DATA);
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
	dn = malloc(sizeof(*df), M_VIMAGE_DATA, M_WAITOK | M_ZERO);
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
 * initial value to each already-allocated virtual subsystem instance.
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

/*
 * Dummy functions so we can initialize the functions pointer upon subsytem
 * registration and do not have to care about at a later time.
 */
static int
vimage_data_init_notsupp(struct vimage_subsys *vse __unused)
{

	return (0);
}

static void
vimage_data_destroy_notsupp(struct vimage_subsys *vse __unused)
{

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
vimage_data_copy_notsupp(struct vimage_subsys *vse __unused,
    void *start __unused, size_t size __unused)
{

}

/*
 * Support for special SYSINIT handlers registered via VIMAGE_SYSINIT()
 * and VIMAGE_SYSUNINIT() or per-subsystem macros overloading these.
 */
static void
vimage_sysinit_iterator(struct vimage_subsys *vse, struct vimage_sysinit *vs)
{
	struct vimage *v;

	/*
	 * Invoke the sysinit function on all the existing instances. This
	 * happens upon (de)registereation of the sysinit handler.
	 */
	VIMAGE_LIST_RLOCK();
	VIMAGE_FOREACH(vse, v) {
		CURVIMAGE_SET_QUIET(vse, __func__, v);
		vs->func(vs->arg);
		CURVIMAGE_RESTORE(vse, __func__);
	}
	VIMAGE_LIST_RUNLOCK();
}

/*
 * Invoke all registered subsystem constructors on the current subsystem.  Used
 * during subsystem construction.  The caller is responsible for ensuring the
 * new subsystem instance is the current subsystem instance.
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
 * dying subsystem instance is the current subsystem instance.
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
	vimage_sysinit_iterator(vse, vs);
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
	vimage_sysinit_iterator(vse, vs);

	/*
	 * Remove the destructor from the global list of subsystem destructors.
	 */
	TAILQ_REMOVE(&vse->v_sysint_destructors, vs, link);
	VIMAGE_SYSINIT_WUNLOCK();
}

/*
 * EVENTHANDLER(9) extensions.
 *
 * Invoke the eventhandler function originally registered with the possibly
 * registered argument for all virtualized subsystem instances.
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

#ifdef VIMAGE_DEBUG
/*
 * Functions to help with debugging recursive set operations of subsystem
 * instances.
 */
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
#if 0
	kdb_backtrace();
#endif
#endif
}
#endif

#ifdef DDB
/*
 * Interactive kernel debugger (ddb) commands.
 */
static struct vimage_subsys *
db_get_vse(db_expr_t addr, boolean_t have_addr, db_expr_t count, char *modif)
{
	struct vimage_subsys *vse;
	int t;

	/*
	 * Parse like the standard syntax (ignoring count).
	 * command [/modifier] [addr] [,count]
	 */
	t = db_read_token();
	if (t == tSLASH) {
		t = db_read_token();
		if (t != tIDENT) {
			db_flush_lex();
			return (NULL);
		}
	} else {
		db_unread_token(t);
		modif[0] = '\0';
	}
	t = db_read_token();
	if (t == tIDENT) {
		if (db_value_of_name(db_tok_string, &addr) ||
		    db_value_of_name_pcpu(db_tok_string, &addr) ||
		    db_value_of_name_vimage(db_tok_string, &addr)) {
			have_addr = 1;
		} else {
			addr = (db_expr_t)db_tok_string;
		}
	} else if (t == tNUMBER) {
		addr = (db_expr_t)db_tok_number;
		have_addr = 1;
	} else {
		db_flush_lex();
		return (NULL);
	}
	db_skip_to_eol();
	vse = NULL;
	if (!have_addr) {
		/* Try to lookup the subsystem by name internally. */
		LIST_FOREACH(vse, &vimage_subsys_head, vimage_subsys_le) {
			if (!strcmp(vse->name, (const char *)addr))
				break;
		}
	} else {
		vse = (struct vimage_subsys *)addr;
	}

	return (vse);
}

#ifdef VIMAGE_DEBUG
DB_SHOW_VIMAGE_COMMAND_FLAGS(recursions, db_show_vimage_recursions, CS_OWN)
{
	struct vimage_subsys *vse;
	struct vimage_recursion *vr;

	vse = db_get_vse(addr, have_addr, count, modif);
	if (vse == NULL) {
		db_printf("usage: show vimage recursions "
		    "<struct vimage_subsys *>|subsysname\n");
		return;
	}

	SLIST_FOREACH(vr, &vse->v_recursions, vr_le)
		vimage_print_recursion(vse, vr, 1);
}
#endif

DB_SHOW_VIMAGE_COMMAND_FLAGS(vars, db_show_vimage_vars, CS_OWN)
{
	struct vimage_subsys *vse;

	vse = db_get_vse(addr, have_addr, count, modif);
	if (vse == NULL) {
		db_printf("usage: show vimage vars "
		    "<struct vimage_subsys *>|subsysname\n");
		return;
	}
	db_lookup_vimage_set_variables(vse);
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

DB_SHOW_VIMAGE_COMMAND_FLAGS(sysinit, db_show_vimage_sysinit, CS_OWN)
{
	struct vimage_subsys *vse;
	struct vimage_sysinit *vs;

	vse = db_get_vse(addr, have_addr, count, modif);
	if (vse == NULL) {
		db_printf("usage: show vimage sysinit "
		    "<struct vimage_subsys *>|subsysname\n");
		return;
	}

	db_printf("%s_SYSINIT vs Name(Ptr)\n", vse->NAME);
	db_printf("  Subsystem  Order\n");
	db_printf("  Function(Name)(Arg)\n");
	TAILQ_FOREACH(vs, &vse->v_sysint_constructors, link) {
		db_show_vimage_print_vs(vs);
		if (db_pager_quit)
			break;
	}
}

DB_SHOW_VIMAGE_COMMAND_FLAGS(sysuninit, db_show_vimage_sysuninit, CS_OWN)
{
	struct vimage_subsys *vse;
	struct vimage_sysinit *vs;

	vse = db_get_vse(addr, have_addr, count, modif);
	if (vse == NULL) {
		db_printf("usage: show vimage sysuninit "
		    "<struct vimage_subsys *>|subsysname\n");
		return;
	}

	db_printf("%s_SYSUNINIT vs Name(Ptr)\n", vse->NAME);
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
#define	V_PRINT_CAST(name, format, cast)					\
	db_printf("  %-30s = " format "\n", # name, cast vse->name);
#define	V_PRINT(name, format)	V_PRINT_CAST(name, format, )
#define	V_PRINT_PTR(name, format)						\
	db_printf("  %-30s = " format "\n", # name, &vse->name);

	db_printf("VIMAGE subsystem '%s' (%p)\n", vse->name, vse);
	V_PRINT(refcnt, "%d");
	V_PRINT(name, "%s");
	V_PRINT(NAME, "%s");
	V_PRINT(setname, "%s");
	V_PRINT(setname_s, "%s");
	V_PRINT(v_symprefix, "%s");
	V_PRINT_CAST(v_start, "%p", (void *));
	V_PRINT_CAST(v_stop, "%p", (void *));
	V_PRINT(v_size, "%zu");
	V_PRINT(v_curvar, "%zu");
	V_PRINT(v_curvar_lpush, "%zu");
	V_PRINT(v_db_instance, "%p");
	V_PRINT(v_instance_size, "%zu");
	V_PRINT_PTR(v_instance_head, "%p");
	V_PRINT_PTR(v_data_free_list, "%p");
	V_FPTR(v_data_init);
	V_FPTR(v_data_alloc);
	V_FPTR(v_data_free);
	V_FPTR(v_data_copy);
	V_PRINT_PTR(v_sysint_constructors, "%p");
	V_PRINT_PTR(v_sysint_destructors, "%p");
	V_PRINT(v_sysinit_earliest, "0x%07X");
	V_PRINT_PTR(v_recursions, "%p");
#undef	V_PRINT_PTR
#undef	V_PRINT
#undef	V_PRINT_CAST
#undef	V_FPTR
}

DB_SHOW_VIMAGE_COMMAND_FLAGS(subsys, db_show_vimage_subsys, CS_OWN)
{
	struct vimage_subsys *vse;

	vse = db_get_vse(addr, have_addr, count, modif);
	if (vse == NULL) {
		db_printf("usage: show vimage subsys "
		    "<struct vimage_subsys *>|subsysname\n");
		return;
	}

	db_show_vimage_print_subsys(vse);
}


DB_SHOW_ALL_COMMAND(vsubsys, db_show_all_vsubsys)
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
