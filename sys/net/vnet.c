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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/jail.h>
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
#define	VNET_BYTES							\
	((uintptr_t)&__stop_set_vnet - (uintptr_t)&__start_set_vnet)

/*
 * VNET_MODMIN is the minimum number of bytes we will reserve for the sum of
 * global variables across all loaded modules.  As this actually sizes an
 * array declared as a virtualized global variable in the kernel itself, and
 * we want the virtualized global variable space to be page-sized, we may
 * have more space than that in practice.
 */
#define	VNET_MODMIN	8192

/*
 * Space to store virtualized global variables from loadable kernel modules,
 * and the free list to manage it.
 */
static VNET_DEFINE(char, modspace[VNET_MODMIN]);

struct vimage_subsys vnet_data;

/*
 * Allocate a virtual network stack.
 */
struct vnet *
vnet_alloc(void)
{

	return ((struct vnet *)vimage_alloc(&vnet_data));
}

/*
 * Destroy a virtual network stack.
 */
void
vnet_destroy(struct vnet *vnet)
{
	struct vimage *v;

	KASSERT(vnet->vnet_sockcnt == 0,
	    ("%s: vnet still has sockets", __func__));
	v = (struct vimage *)vnet;

	vimage_destroy(&vnet_data, v);
}

/*
 * Once on boot, initialize the modspace freelist to entirely cover modspace.
 */
static int
vnet_data_init(struct vimage_subsys *vse)
{
	struct vimage_data_free *df;
	size_t modextra;

	/* Already initialized? */
	if (!TAILQ_EMPTY(&vse->v_data_free_list))
		return (0);

	df = malloc(sizeof(*df), M_VIMAGE_DATA, M_WAITOK | M_ZERO);
	df->vnd_start = (uintptr_t)&VNET_NAME(modspace);
	df->vnd_len = VNET_MODMIN;
	TAILQ_INSERT_HEAD(&vse->v_data_free_list, df, vnd_link);

	modextra = vse->v_size - VNET_BYTES;
	if (modextra < sizeof(*df))
		return (0);
	
	df = malloc(sizeof(*df), M_VIMAGE_DATA, M_WAITOK | M_ZERO);
	df->vnd_start = vse->v_stop;
	df->vnd_len = modextra;
	TAILQ_INSERT_HEAD(&vse->v_data_free_list, df, vnd_link);

	return (0);
}

/*
 * Boot time initialization and allocation of virtual network stacks.
 */
struct vimage_subsys vnet_data =
{
	.name			= "vnet",
	.NAME			= "VNET",

	.setname		= VNET_SETNAME,
	.v_symprefix		= VNET_SYMPREFIX,

	.v_start		= (uintptr_t)&__start_set_vnet,
	.v_stop			= (uintptr_t)&__stop_set_vnet,

	.v_curvar		= offsetof(struct thread, td_vnet),
	.v_curvar_lpush		= offsetof(struct thread, td_vnet_lpush),

	.v_instance_size	= sizeof(struct vnet),

	/* Dynamic/module data allocator. */
	.v_data_free_list	=
	    TAILQ_HEAD_INITIALIZER(vnet_data.v_data_free_list),
	.v_data_init		= vnet_data_init,

	/* System initialization framework. */
	.v_sysint_constructors	=
	    TAILQ_HEAD_INITIALIZER(vnet_data.v_sysint_constructors),
	.v_sysint_destructors	=
	    TAILQ_HEAD_INITIALIZER(vnet_data.v_sysint_destructors),
	.v_sysinit_earliest	= SI_SUB_VNET,
};

static void
vnet_init_prelink(void *arg)
{

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

#ifdef DDB
/*
 * DDB(4).
 */
DB_SHOW_COMMAND(vnets, db_show_vnets)
{
	VNET_ITERATOR_DECL(v);

	VNET_FOREACH(v) {
		struct vnet *vnet = (struct vnet *)v;

		db_printf("vnet            = %p\n", v);
		db_printf(" vnet_data_mem  = %p\n", v->v_data_mem);
		db_printf(" vnet_data_base = 0x%jx\n",
		    (uintmax_t)v->v_data_base);
		db_printf(" vnet_ifcnt     = %u\n", vnet->vnet_ifcnt);
		db_printf(" vnet_sockcnt   = %u\n", vnet->vnet_sockcnt);
		db_printf("\n");
		if (db_pager_quit)
			break;
	}
}
#endif /* DDB */
