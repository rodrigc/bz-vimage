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
__FBSDID("$FreeBSD: src/sys/net/vnet.c,v 1.21 2011/01/11 13:59:06 jhb Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/jail.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/vimage.h>

#ifdef DDB
#include <ddb/ddb.h>
#endif

#include <net/vnet.h>

/*
 * Import the VIMAGE module data free list malloc accounting type for
 * vnet_data_init().
 */
MALLOC_DECLARE(M_VIMAGE_DATA);

/*
 * This file includes the virtual network stack (VNET) specific implementations
 * for the generic VIMAGE kernel virtualization framework.  This includes:
 * - struct vimage_subsys vnet_data, the definitions of the vnet subsystem,
 * - instance allocation and destroy wrapper functions with special vnet checks,
 * - the function to setup space proovided to the kernel linkers to put
 *   virtualized VNET objects of modules into,
 * - VNET subsystem registaration,
 * - allocating and initializing the base system network stack, and
 * - VNET specific debugging support.
 */

/*
 * Cache of the network stack for the base system. We can always use this
 * to compare any vnet to, no matter whether we can get to an ucred and the
 * prison or not.  It is espcially used by DEFAULT_VNET() checks.
 */
struct vnet *vnet0;

static int vnet_data_init(struct vimage_subsys *);

/*
 * This struct describes the virtual networks specific information
 * that the generic VIMAGE framework needs to know.
 */
struct vimage_subsys vnet_data =
{
	.name			= "vnet",
	.NAME			= "VNET",

	.flags			= VSE_FLAG_ASYNC_SHUTDOWN,

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

/*
 * Allocate a virtual network stack instance.
 */
struct vnet *
vnet_alloc(void)
{

	return ((struct vnet *)vimage_alloc(&vnet_data));
}

/*
 * Destroy a virtual network stack instance.
 */
void
vnet_destroy(struct prison *pr)
{
	struct vnet *vnet;

	vnet = pr->pr_vnet;
	KASSERT(vnet->vnet_sockcnt == 0,
	    ("%s: vnet %p still has sockets", __func__, vnet));

	vimage_destroy(&vnet_data, &vnet->v, pr);
}

/*
 * Once on subsystem registration the VIMAGE framework will call to
 * initialize the modspace freelist. Try to use as much memory available
 * for modspace.
 *
 * This function is virtual network stack specific as the variable
 * name, size, possible external space, .. are not easily unified
 * across all subsystems.  Use the programming interface VIMAGE defines.
 */

/*
 * Number of bytes of data in the 'set_vnet' linker set, and hence the total
 * size of all kernel virtualized global variables including modspace[]
 * but not including extra space malloced past the end of the set.
 */
#define	VNET_BYTES							\
	((uintptr_t)&__stop_set_vnet - (uintptr_t)&__start_set_vnet)

/*
 * VNET_MODMIN is the minimum number of bytes we reserve for the sum of
 * global variables across all loaded modules.  As this actually sizes an
 * array declared as a virtualized global variable (modspace[]) in the
 * linker set itself, and the VIMAGE framework wants the virtualized
 * global variable space to be page-sized, we may have more space than
 * that in practice though it might be distinct memory regions within
 * the page(s).
 */
#define	VNET_MODMIN	8192
static VNET_DEFINE(char, modspace[VNET_MODMIN]);

static int
vnet_data_init(struct vimage_subsys *vse)
{
	struct vimage_data_free *df;
#if 0
	size_t modextra;
#endif

	/* Already initialized? */
	if (!TAILQ_EMPTY(&vse->v_data_free_list))
		return (0);

	df = malloc(sizeof(*df), M_VIMAGE_DATA, M_WAITOK | M_ZERO);
	df->vnd_start = (uintptr_t)&VNET_NAME(modspace);
	df->vnd_len = VNET_MODMIN;
	TAILQ_INSERT_HEAD(&vse->v_data_free_list, df, vnd_link);

#if 0
	modextra = vse->v_size - VNET_BYTES;
	if (modextra < sizeof(*df))
		return (0);
	
	df = malloc(sizeof(*df), M_VIMAGE_DATA, M_WAITOK | M_ZERO);
	df->vnd_start = vse->v_stop;
	df->vnd_len = modextra;
	TAILQ_INSERT_HEAD(&vse->v_data_free_list, df, vnd_link);
#endif

	return (0);
}

/*
 * Make the virtual network subsystem known to the generic VIMAGE
 * framework.
 */
static void
vnet_init_prelink(void *arg)
{

	/* Warn people before take off - in case we crash early. */
	printf("WARNING: VIMAGE (virtualized network stack) is a highly "
	    "experimental feature.\n");

	vimage_subsys_register(&vnet_data);
}
SYSINIT(vnet_init_prelink, SI_SUB_VIMAGE_PRELINK, SI_ORDER_SECOND,
    vnet_init_prelink, NULL);

/*
 * Allocate and initialize the network stack for the base system.
 */
static void
vnet0_init(void *arg)
{

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
SYSINIT(vnet_init_done, SI_SUB_VNET_DONE, SI_ORDER_FIRST, vnet_init_done, NULL);


#ifdef DDB
/*
 * Virtual network stack specific ddb(4) commands.
 */

static void
db_show_vnet_print_vnet(struct vnet *vnet)
{

	db_printf("vnet            = %p\n", vnet);
	db_printf(" vnet_data_mem  = %p\n", vnet->v.v_data_mem);
	db_printf(" vnet_data_base = 0x%jx\n",
	    (uintmax_t)vnet->v.v_data_base);
	db_printf(" vnet_ifcnt     = %u\n", vnet->vnet_ifcnt);
	db_printf(" vnet_sockcnt   = %u\n", vnet->vnet_sockcnt);
	db_printf("\n");
}

DB_SHOW_COMMAND(vnet, db_show_vnet)
{
	struct vnet *vnet;
	db_expr_t value;

	if (have_addr) {
		vnet = (struct vnet *)addr;
	} else {
		value = 0;
		do {
			if (db_get_variable_s("$db_vnet", &value) &&
			    value != 0)
				break;
			if (db_get_variable_s("$curvnet", &value) &&
			    value != 0)
				break;
			db_printf("usage: show vnet <struct vnet *>\n");
			return;
		} while (0);
		vnet = (struct vnet *)value;
	}

	db_show_vnet_print_vnet(vnet);
}

DB_SHOW_ALL_COMMAND(vnets, db_show_all_vnets)
{
	VNET_ITERATOR_DECL(v);

	VNET_FOREACH(v) {
		struct vnet *vnet = (struct vnet *)v;

		db_show_vnet_print_vnet(vnet);
		if (db_pager_quit)
			break;
	}
}
#endif /* DDB */
