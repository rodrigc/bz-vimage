/*-
 * Copyright (c) 2004-2009 University of Zagreb
 * Copyright (c) 2006-2009 FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed by the University of Zagreb and the
 * FreeBSD Foundation under sponsorship by the Stichting NLnet and the
 * FreeBSD Foundation.
 *
 * Copyright (c) 2009 Jeffrey Roberson <jeff@freebsd.org>
 * Copyright (c) 2009 Robert N. M. Watson
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/net/vnet.c,v 1.15 2010/04/14 23:06:07 julian Exp $");

#include "opt_ddb.h"
#include "opt_kdb.h"
#include "opt_kdtrace.h"

#include <sys/param.h>
#include <sys/kdb.h>
#include <sys/kernel.h>
#include <sys/jail.h>
#include <sys/sdt.h>
#include <sys/systm.h>
#include <sys/linker_set.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/sx.h>
#include <sys/vimage.h>

#include <machine/stdarg.h>

#ifdef DDB
#include <ddb/ddb.h>
#include <ddb/db_sym.h>
#endif

#include <net/if.h>
#include <net/if_var.h>
#include <net/vnet.h>

/*-
 * This file implements core functions for virtual network stacks:
 *
 * - Virtual network stack management functions.
 *
 * - Virtual network stack memory allocator, which virtualizes global
 *   variables in the network stack
 *
 * - Virtualized SYSINIT's/SYSUNINIT's, which allow network stack subsystems
 *   to register startup/shutdown events to be run for each virtual network
 *   stack instance.
 */

MALLOC_DEFINE(M_VNET, "vnet", "network stack control block");

/*
 * The virtual network stack list has two read-write locks, one sleepable and
 * the other not, so that the list can be stablized and walked in a variety
 * of network stack contexts.  Both must be acquired exclusively to modify
 * the list, but a read lock of either lock is sufficient to walk the list.
 */
struct rwlock		vnet_rwlock;
struct sx		vnet_sxlock;

#define	VNET_LIST_WLOCK() do {						\
	sx_xlock(&vnet_sxlock);						\
	rw_wlock(&vnet_rwlock);						\
} while (0)

#define	VNET_LIST_WUNLOCK() do {					\
	rw_wunlock(&vnet_rwlock);					\
	sx_xunlock(&vnet_sxlock);					\
} while (0)

struct vnet_list_head vnet_head;
struct vnet *vnet0;

/*
 * The virtual network stack allocator provides storage for virtualized
 * global variables.  These variables are defined/declared using the
 * VNET_DEFINE()/VNET_DECLARE() macros, which place them in the 'set_vnet'
 * linker set.  The details of the implementation are somewhat subtle, but
 * allow the majority of most network subsystems to maintain
 * virtualization-agnostic.
 *
 * The virtual network stack allocator handles variables in the base kernel
 * vs. modules in similar but different ways.  In both cases, virtualized
 * global variables are marked as such by being declared to be part of the
 * vnet linker set.  These "master" copies of global variables serve two
 * functions:
 *
 * (1) They contain static initialization or "default" values for global
 *     variables which will be propagated to each virtual network stack
 *     instance when created.  As with normal global variables, they default
 *     to zero-filled.
 *
 * (2) They act as unique global names by which the variable can be referred
 *     to, regardless of network stack instance.  The single global symbol
 *     will be used to calculate the location of a per-virtual instance
 *     variable at run-time.
 *
 * Each virtual network stack instance has a complete copy of each
 * virtualized global variable, stored in a malloc'd block of memory
 * referred to by vnet->vnet_data_mem.  Critical to the design is that each
 * per-instance memory block is laid out identically to the master block so
 * that the offset of each global variable is the same across all blocks.  To
 * optimize run-time access, a precalculated 'base' address,
 * vnet->vnet_data_base, is stored in each vnet, and is the amount that can
 * be added to the address of a 'master' instance of a variable to get to the
 * per-vnet instance.
 *
 * Virtualized global variables are handled in a similar manner, but as each
 * module has its own 'set_vnet' linker set, and we want to keep all
 * virtualized globals togther, we reserve space in the kernel's linker set
 * for potential module variables using a per-vnet character array,
 * 'modspace'.  The virtual network stack allocator maintains a free list to
 * track what space in the array is free (all, initially) and as modules are
 * linked, allocates portions of the space to specific globals.  The kernel
 * module linker queries the virtual network stack allocator and will
 * bind references of the global to the location during linking.  It also
 * calls into the virtual network stack allocator, once the memory is
 * initialized, in order to propagate the new static initializations to all
 * existing virtual network stack instances so that the soon-to-be executing
 * module will find every network stack instance with proper default values.
 */

/*
 * Number of bytes of data in the 'set_vnet' linker set, and hence the total
 * size of all kernel virtualized global variables, and the malloc(9) type
 * that will be used to allocate it.
 */
#define	VNET_BYTES	(VNET_STOP - VNET_START)

MALLOC_DEFINE(M_VNET_DATA, "vnet_data", "VNET data");

/*
 * VNET_MODMIN is the minimum number of bytes we will reserve for the sum of
 * global variables across all loaded modules.  As this actually sizes an
 * array declared as a virtualized global variable in the kernel itself, and
 * we want the virtualized global variable space to be page-sized, we may
 * have more space than that in practice.
 */
#define	VNET_MODMIN	8192
#define	VNET_SIZE	roundup2(VNET_BYTES, PAGE_SIZE)
#define	VNET_MODEXTRA	(VNET_SIZE - VNET_BYTES)

/*
 * Space to store virtualized global variables from loadable kernel modules,
 * and the free list to manage it.
 */
static VNET_DEFINE(char, modspace[VNET_MODMIN]);

struct vimage_subsys vnet_data;

SDT_PROVIDER_DEFINE(vnet);
SDT_PROBE_DEFINE1(vnet, functions, vnet_alloc, entry, "int");
SDT_PROBE_DEFINE2(vnet, functions, vnet_alloc, alloc, "int", "struct vnet *");
SDT_PROBE_DEFINE2(vnet, functions, vnet_alloc, return, "int", "struct vnet *");
SDT_PROBE_DEFINE2(vnet, functions, vnet_destroy, entry, "int", "struct vnet *");
SDT_PROBE_DEFINE1(vnet, functions, vnet_destroy, return, "int");

void vnet_sysinit_iterator(struct vimage_sysinit *);

/*
 * Allocate a virtual network stack.
 */
struct vnet *
vnet_alloc(void)
{
	struct vnet *vnet;

	SDT_PROBE1(vnet, functions, vnet_alloc, entry, __LINE__);
	vnet = malloc(sizeof(struct vnet), M_VNET, M_WAITOK | M_ZERO);
	vnet->vnet_magic_n = VNET_MAGIC_N;
	SDT_PROBE2(vnet, functions, vnet_alloc, alloc, __LINE__, vnet);

	/*
	 * Allocate storage for virtualized global variables and copy in
	 * initial values form our 'master' copy.
	 */
	vnet->vnet_data_mem = malloc(VNET_SIZE, M_VNET_DATA, M_WAITOK);
	memcpy(vnet->vnet_data_mem, (void *)VNET_START, VNET_BYTES);

	/*
	 * All use of vnet-specific data will immediately subtract VNET_START
	 * from the base memory pointer, so pre-calculate that now to avoid
	 * it on each use.
	 */
	vnet->vnet_data_base = (uintptr_t)vnet->vnet_data_mem - VNET_START;

	/* Initialize / attach vnet module instances. */
	CURVNET_SET_QUIET(vnet);
	vimage_sysinit(&vnet_data);
	CURVNET_RESTORE();

	VNET_LIST_WLOCK();
	LIST_INSERT_HEAD(&vnet_head, vnet, vnet_le);
	VNET_LIST_WUNLOCK();

	SDT_PROBE2(vnet, functions, vnet_alloc, return, __LINE__, vnet);
	return (vnet);
}

/*
 * Destroy a virtual network stack.
 */
void
vnet_destroy(struct vnet *vnet)
{

	SDT_PROBE2(vnet, functions, vnet_destroy, entry, __LINE__, vnet);
	KASSERT(vnet->vnet_sockcnt == 0,
	    ("%s: vnet still has sockets", __func__));

	VNET_LIST_WLOCK();
	LIST_REMOVE(vnet, vnet_le);
	VNET_LIST_WUNLOCK();

	CURVNET_SET_QUIET(vnet);
	vimage_sysuninit(&vnet_data);
	CURVNET_RESTORE();

	/*
	 * Release storage for the virtual network stack instance.
	 */
	free(vnet->vnet_data_mem, M_VNET_DATA);
	vnet->vnet_data_mem = NULL;
	vnet->vnet_data_base = 0;
	vnet->vnet_magic_n = 0xdeadbeef;
	free(vnet, M_VNET);
	SDT_PROBE1(vnet, functions, vnet_destroy, return, __LINE__);
}

/*
 * Once on boot, initialize the modspace freelist to entirely cover modspace.
 */
static int
vnet_data_init(struct vimage_subsys *vse)
{
	struct vimage_data_free *df;

	/* Already initialized? */
	if (!TAILQ_EMPTY(&vse->v_data_free_list))
		return (0);

	df = malloc(sizeof(*df), M_VIMAGE_DATA_FREE, M_WAITOK | M_ZERO);
	df->vnd_start = (uintptr_t)&VNET_NAME(modspace);
	df->vnd_len = VNET_MODMIN;
	TAILQ_INSERT_HEAD(&vse->v_data_free_list, df, vnd_link);

	if (VNET_MODEXTRA < sizeof(*df))
		return (0);
	
	df = malloc(sizeof(*df), M_VIMAGE_DATA_FREE, M_WAITOK | M_ZERO);
	df->vnd_start = VNET_STOP;
	df->vnd_len = VNET_MODEXTRA;
	TAILQ_INSERT_HEAD(&vse->v_data_free_list, df, vnd_link);

	return (0);
}

/*
 * When a new virtualized global variable has been allocated, propagate its
 * initial value to each already-allocated virtual network stack instance.
 */
static void
vnet_data_copy(void *start, size_t size)
{
	struct vnet *vnet;

	VNET_LIST_RLOCK();
	LIST_FOREACH(vnet, &vnet_head, vnet_le)
		memcpy((void *)((uintptr_t)vnet->vnet_data_base +
		    (uintptr_t)start), start, size);
	VNET_LIST_RUNLOCK();
}

/*
 * Boot time initialization and allocation of virtual network stacks.
 */
struct vimage_subsys vnet_data =
{
	.setname		= VNET_SETNAME,

	.v_curvar		= offsetof(struct thread, td_vnet),
	.v_curvar_lpush		= offsetof(struct thread, td_vnet_lpush),

	/* Dynamic/module data allocator. */
	.v_data_free_list	=
	    TAILQ_HEAD_INITIALIZER(vnet_data.v_data_free_list),
	.v_data_init		= vnet_data_init,
	.v_data_copy		= vnet_data_copy,

	/* System initialization framework. */
	.v_sysint_constructors	=
	    TAILQ_HEAD_INITIALIZER(vnet_data.v_sysint_constructors),
	.v_sysint_destructors	=
	    TAILQ_HEAD_INITIALIZER(vnet_data.v_sysint_destructors),
	.v_sysinit_earliest	= SI_SUB_VNET,
	.v_sysinit_iter		= vnet_sysinit_iterator,
};

static void
vnet_init_prelink(void *arg)
{

	rw_init(&vnet_rwlock, "vnet_rwlock");
	sx_init(&vnet_sxlock, "vnet_sxlock");
	LIST_INIT(&vnet_head);

	vimage_subsys_register(&vnet_data);
}
SYSINIT(vnet_init_prelink, SI_SUB_VIMAGE_PRELINK, SI_ORDER_SECOND,
    vnet_init_prelink, NULL);

static void
vnet0_init(void *arg)
{

	/* Warn people before take off - in case we crash early. */
	printf("WARNING: VIMAGE (virtualized network stack) is a highly "
	    "experimental feature.\n");

	/*
	 * We MUST clear curvnet in vi_init_done() before going SMP,
	 * otherwise CURVNET_SET() macros would scream about unnecessary
	 * curvnet recursions.
	 */
	curvnet = prison0.pr_vnet = vnet0 = vnet_alloc();
}
SYSINIT(vnet0_init, SI_SUB_VNET, SI_ORDER_FIRST, vnet0_init, NULL);

static void
vnet_init_done(void *unused)
{

	curvnet = NULL;
}

SYSINIT(vnet_init_done, SI_SUB_VNET_DONE, SI_ORDER_FIRST, vnet_init_done,
    NULL);

/*
 * Support for special SYSINIT handlers registered via VNET_SYSINIT()
 * and VNET_SYSUNINIT().
 */
void
vnet_sysinit_iterator(struct vimage_sysinit *vs)
{
	struct vnet *vnet;

	/*
	 * Invoke the sysinit function on all the existing vnets. This
	 * happens upon (de)registereation of the sysinit handler.
	 */
	VNET_LIST_RLOCK();
	VNET_FOREACH(vnet) {
		CURVNET_SET_QUIET(vnet);
		vs->func(vs->arg);
		CURVNET_RESTORE();
	}
	VNET_LIST_RUNLOCK();
}

#ifdef VNET_DEBUG
struct vnet_recursion {
	SLIST_ENTRY(vnet_recursion)	 vnr_le;
	const char			*prev_fn;
	const char			*where_fn;
	int				 where_line;
	struct vnet			*old_vnet;
	struct vnet			*new_vnet;
};

static SLIST_HEAD(, vnet_recursion) vnet_recursions =
    SLIST_HEAD_INITIALIZER(vnet_recursions);

static void
vnet_print_recursion(struct vnet_recursion *vnr, int brief)
{

	if (!brief)
		printf("CURVNET_SET() recursion in ");
	printf("%s() line %d, prev in %s()", vnr->where_fn, vnr->where_line,
	    vnr->prev_fn);
	if (brief)
		printf(", ");
	else
		printf("\n    ");
	printf("%p -> %p\n", vnr->old_vnet, vnr->new_vnet);
}

void
vnet_log_recursion(struct vnet *old_vnet, const char *old_fn, int line)
{
	struct vnet_recursion *vnr;

	/* Skip already logged recursion events. */
	SLIST_FOREACH(vnr, &vnet_recursions, vnr_le)
		if (vnr->prev_fn == old_fn &&
		    vnr->where_fn == curthread->td_vnet_lpush &&
		    vnr->where_line == line &&
		    (vnr->old_vnet == vnr->new_vnet) == (curvnet == old_vnet))
			return;

	vnr = malloc(sizeof(*vnr), M_VNET, M_NOWAIT | M_ZERO);
	if (vnr == NULL)
		panic("%s: malloc failed", __func__);
	vnr->prev_fn = old_fn;
	vnr->where_fn = curthread->td_vnet_lpush;
	vnr->where_line = line;
	vnr->old_vnet = old_vnet;
	vnr->new_vnet = curvnet;

	SLIST_INSERT_HEAD(&vnet_recursions, vnr, vnr_le);

	vnet_print_recursion(vnr, 0);
#ifdef KDB
	kdb_backtrace();
#endif
}
#endif /* VNET_DEBUG */

/*
 * DDB(4).
 */
#ifdef DDB
DB_SHOW_COMMAND(vnets, db_show_vnets)
{
	VNET_ITERATOR_DECL(vnet_iter);

	VNET_FOREACH(vnet_iter) {
		db_printf("vnet            = %p\n", vnet_iter);
		db_printf(" vnet_magic_n   = 0x%x (%s, orig 0x%x)\n",
		    vnet_iter->vnet_magic_n,
		    (vnet_iter->vnet_magic_n == VNET_MAGIC_N) ?
			"ok" : "mismatch", VNET_MAGIC_N);
		db_printf(" vnet_ifcnt     = %u\n", vnet_iter->vnet_ifcnt);
		db_printf(" vnet_sockcnt   = %u\n", vnet_iter->vnet_sockcnt);
		db_printf(" vnet_data_mem  = %p\n", vnet_iter->vnet_data_mem);
		db_printf(" vnet_data_base = 0x%jx\n",
		    (uintmax_t)vnet_iter->vnet_data_base);
		db_printf("\n");
		if (db_pager_quit)
			break;
	}
}


#ifdef VNET_DEBUG
DB_SHOW_COMMAND(vnetrcrs, db_show_vnetrcrs)
{
	struct vnet_recursion *vnr;

	SLIST_FOREACH(vnr, &vnet_recursions, vnr_le)
		vnet_print_recursion(vnr, 1);
}
#endif
#endif /* DDB */
