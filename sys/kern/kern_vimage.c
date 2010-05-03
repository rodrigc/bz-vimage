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

#include <sys/param.h>
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


static void *vimage_data_alloc(struct vimage_subsys *, size_t);
static void vimage_data_free(struct vimage_subsys *, void *, size_t);

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

	sx_init(&vimage_subsys_data_sxlock, "vimage subsys data alloc lock");
}
SYSINIT(vimage_init_prelink, SI_SUB_VIMAGE_PRELINK, SI_ORDER_FIRST,
    vimage_init_prelink, NULL);

/*
 * Boot or load time registration of virtualized subsystems.
 */
int
vimage_subsys_register(struct vimage_subsys *vs)
{
	int error;

	KASSERT(vs != NULL, ("%s: vs is NULL", __func__));
#if 0
	/* We cannot do this as we are running too early during boot. */
	KASSERT(curthread->td_ucred->cr_prison == &prison0,
	    ("%s: not called from prison0 td=%p prison=%p, vs=%p", __func__,
	    curthread, curthread->td_ucred->cr_prison, vs));
#endif

	/*
	 * Assert a few things but as we might be called from modules
	 * loaded, do not panic but just log and return an error.
	 * Where possible, initialize to defaults.
	 */
	if (vs->setname == NULL) {
		printf("%s: setname of vs=%p is NULL\n", __func__, vs);
		return (EINVAL);
	}
	if (strncmp(vs->setname, "set_", 4)) {
		printf("%s: setname of vs=%p not starting with "
		    "\"set_\": %s\n", __func__, vs, vs->setname);
		return (EINVAL);
	}
	/* The short name is needed for link_elf.c. */
	if (vs->name == NULL)
		vs->name = vs->setname + 4;	/* Skip "set_". */
	if (vs->v_data_init == NULL) {
		/* Disabling dynamic data region/module support. */
		vs->v_data_init = vimage_data_init_notsupp;
		vs->v_data_alloc = vimage_data_alloc_notsupp;
		vs->v_data_free = vimage_data_free_notsupp;
		vs->v_data_copy = vimage_data_copy_notsupp;
		printf("%s: dynamic data region/module support disabled "
		    "for vs=%p %s\n", __func__, vs, vs->setname);
	}
	if (vs->v_data_copy == NULL) {
		printf("%s: mandatory v_data_copy of vs=%p %s uninitialized\n",
		    __func__, vs, vs->setname);
		return (EINVAL);
	}
	if (vs->v_data_alloc == NULL)
		vs->v_data_alloc = vimage_data_alloc;
	if (vs->v_data_free == NULL)
		vs->v_data_free = vimage_data_free;

	/*
	 * Initalize dynamic data region for module support. Callee has to
	 * handle list initialization itself as we do not know varaible names
	 * of the region or extra space that might be available.
	 */
	error = (*vs->v_data_init)(vs);
	if (error)
		return (error);
	
	VIMAGE_SUBSYS_LIST_WLOCK();
	/*
	 * Ensure a subsytem with the setname has not yet been registered.
	 * We do not check for the vs pointer as that would still allow
	 * duplicate subsystem registration.
	 */
	if (vimage_subsys_get(vs->setname) != NULL) {
		VIMAGE_SUBSYS_LIST_WUNLOCK();
		printf("%s: vs=%p duplicate registration for %s\n",
		    __func__, vs, vs->setname);
		return (EEXIST);
	}
	refcount_init(&vs->refcnt, 1);
	LIST_INSERT_HEAD(&vimage_subsys_head, vs, vimage_subsys_le);
	VIMAGE_SUBSYS_LIST_WUNLOCK();

	return (0);
}

int
vimage_subsys_deregister(struct vimage_subsys *vs)
{
	struct vimage_subsys *vse;
	int error;

	error = ENOENT;
	VIMAGE_SUBSYS_LIST_WLOCK();
	LIST_FOREACH(vse, &vimage_subsys_head, vimage_subsys_le) {
		if (vs == vse) {
			if (refcount_release(&vs->refcnt) == 1) {
				LIST_REMOVE(vs, vimage_subsys_le);
				error = 0;
			} else {
				/*
				 * We cannot allow vs to possibly go away
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
vimage_data_alloc(struct vimage_subsys *vs, size_t size)
{
	struct vimage_data_free *df;
	void *s;

	s = NULL;
	size = roundup2(size, sizeof(void *));
	sx_xlock(&vimage_subsys_data_sxlock);
	TAILQ_FOREACH(df, &vs->v_data_free_list, vnd_link) {
		if (df->vnd_len < size)
			continue;
		if (df->vnd_len == size) {
			s = (void *)df->vnd_start;
			TAILQ_REMOVE(&vs->v_data_free_list, df, vnd_link);
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
vimage_data_free(struct vimage_subsys *vs, void *start_arg, size_t size)
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
	TAILQ_FOREACH(df, &vs->v_data_free_list, vnd_link) {
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
				TAILQ_REMOVE(&vs->v_data_free_list, dn,
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
		TAILQ_INSERT_TAIL(&vs->v_data_free_list, dn, vnd_link);
	sx_xunlock(&vimage_subsys_data_sxlock);
}

static int
vimage_data_init_notsupp(struct vimage_subsys *vs __unused)
{

	return (0);
}

static void *
vimage_data_alloc_notsupp(struct vimage_subsys *vs __unused,
    size_t size __unused)
{

	return (NULL);
}

static void
vimage_data_free_notsupp(struct vimage_subsys *vs __unused,
    void *start_arg __unused, size_t size __unused)
{

}

static void
vimage_data_copy_notsupp(void *start __unused, size_t size __unused)
{

}

/* end */
