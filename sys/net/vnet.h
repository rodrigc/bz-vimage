/*-
 * Copyright (c) 2006-2009 University of Zagreb
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
 *
 * $FreeBSD: src/sys/net/vnet.h,v 1.31 2010/04/14 23:06:07 julian Exp $
 */

/*-
 * This header file extends the definitions of the general VIMAGE framework
 * and defines several sets of interfaces supporting virtualized network
 * stacks:
 *
 * - Definition of 'struct vnet' and functions and macros to allocate/free/
 *   manipulate it.
 *
 * - A virtual network stack memory allocator, which provides support for
 *   virtualized global variables via a special linker set, set_vnet.
 *
 * - Virtualized sysinits/sysuninits, which allow constructors and
 *   destructors to be run for each network stack subsystem as virtual
 *   instances are created and destroyed.
 *
 * - Virtualized eventhandler and sysctl support.
 *
 * If VIMAGE isn't compiled into the kernel, virtualized global variables
 * compile to normal global variables, and virtualized sysinits, sysctl
 * and eventhandler macros to regular, non-virtualized ones.
 */

#ifndef _NET_VNET_H_
#define	_NET_VNET_H_

#if defined(_KERNEL) || defined(_WANT_VNET)
#include <sys/vimage.h>

/*
 * Description of the virtual networks specific information that the generic
 * VIMAGE framework needs to know.
 */
extern struct vimage_subsys vnet_data;

/*
 * struct vnet describes a virtualized network stack instance and overloads the
 * generic struct vimage for that.  It is primarily a pointer to storage for
 * virtualized global variables with some network stack specific extensions.
 * Exported to userspcae for kvm(3).
 */
struct vnet {
	struct vimage		v;
	u_int			vnet_ifcnt;
	u_int			vnet_sockcnt;
};

/*
 * XXX These two virtual network stack linker-set related definitions are also
 * shared with kvm(3) for historical reasons.  New code might want to get the
 * information from 'struct vimage_subsys'.
 */
#define	VNET_SETNAME		"set_vnet"
#define	VNET_SYMPREFIX		"vnet_entry_"
#endif /* _KERNEL || _WANT_VNET */


#ifdef _KERNEL
#ifdef VIMAGE
#include <sys/proc.h>			/* for struct thread */

/*
 * Cache of the network stack for the base system. We can always use this
 * to compare any vnet to, no matter whether we can get to an ucred and the
 * prison or not.  It is especially used by DEFAULT_VNET() checks.
 */
extern struct vnet *vnet0;

/*
 * Functions to allocate and destroy virtual network stacks used by
 * the kernel jail framework.
 */
struct vnet *vnet_alloc(void);
void	vnet_destroy(struct vnet *vnet);

/*
 * The current virtual network stack.
 * XXX May we wish to move this to struct pcpu in the future and adjust the
 * VIMAGE framework?
 */
#define	curvnet			curthread->td_vnet

#define	IS_DEFAULT_VNET(arg)	((arg) == vnet0)

#define	CRED_TO_VNET(cr)	(cr)->cr_prison->pr_vnet
#define	TD_TO_VNET(td)		CRED_TO_VNET((td)->td_ucred)
#define	P_TO_VNET(p)		CRED_TO_VNET((p)->p_ucred)

/*
 * VIMAGE subsystem allocator, using a linker-set for each subsystem, which
 * will be put into a dedicated elf section, and allows global variables to be
 * automatically instantiated for each network stack instance.
 */
DECLARE_LINKER_SET(set_vnet);

#define	VNET_NAME(n)		vnet_entry_##n
#define	VNET_DECLARE(t, n)	extern t VNET_NAME(n)
#define	VNET_DEFINE(t, n)	t VNET_NAME(n) __section(VNET_SETNAME) __used
#define	_VNET_PTR(b, n)		(__typeof(VNET_NAME(n))*)		\
				    ((b) + (uintptr_t)&VNET_NAME(n))

#define	_VNET(b, n)		(*_VNET_PTR(b, n))

/*
 * Virtualized global variable accessor macros.
 */
#define	VNET_VNET_PTR(vnet, n)	_VNET_PTR((vnet)->v.v_data_base, n)
#define	VNET_VNET(vnet, n)	(*VNET_VNET_PTR((vnet), n))

#define	VNET_PTR(n)		VNET_VNET_PTR(curvnet, n)
#define	VNET(n)			VNET_VNET(curvnet, n)

#else /* !VIMAGE */

/*
 * Various virtual network stack macros compile to no-ops without VIMAGE.
 */
#define	curvnet			NULL

#define	IS_DEFAULT_VNET(arg)	1
#define	CRED_TO_VNET(cr)	NULL
#define	TD_TO_VNET(td)		NULL
#define	P_TO_VNET(p)		NULL

/*
 * Versions of the VNET macros that compile to normal global variables and
 * standard sysctl definitions.
 */
#define	VNET_NAME(n)		n
#define	VNET_DECLARE(t, n)	extern t n
#define	VNET_DEFINE(t, n)	t n
#define	_VNET_PTR(b, n)		&VNET_NAME(n)

/*
 * Virtualized global variable accessor macros.
 */
#define	VNET_VNET_PTR(vnet, n)	(&(n))
#define	VNET_VNET(vnet, n)	(n)

#define	VNET_PTR(n)		(&(n))
#define	VNET(n)			(n)
#endif /* VIMAGE */

/*
 * Various macros -- get and set the current network stack, iterator
 * macros, but also assertions.
 * A set of read locks to stabilize the list while traversing.
 *
 * XXX we need to decide whether to deprecate the lock macros or keep them
 * to leave the possibility for per-subsystem locking.
 */
#define	CURVNET_SET_QUIET(arg)	CURVIMAGE_SET_QUIET(&vnet_data, vnet, arg)
#define	CURVNET_SET(arg)	CURVIMAGE_SET(&vnet_data, vnet, arg)
#define	CURVNET_RESTORE()	CURVIMAGE_RESTORE(&vnet_data, vnet)

#define	VNET_ASSERT(exp, msg)		VIMAGE_ASSERT(exp, msg)

#ifdef VIMAGE
#define	VNET_ITERATOR_DECL(arg)		struct vimage *arg
#else /* !VIMAGE */
#define	VNET_ITERATOR_DECL(arg)
#endif /* VIMAGE */
#define	VNET_FOREACH(arg)		VIMAGE_FOREACH(&vnet_data, arg)

#define	VNET_LIST_RLOCK()		VIMAGE_LIST_RLOCK()
#define	VNET_LIST_RLOCK_NOSLEEP()	VIMAGE_LIST_RLOCK_NOSLEEP()
#define	VNET_LIST_RUNLOCK()		VIMAGE_LIST_RUNLOCK()
#define	VNET_LIST_RUNLOCK_NOSLEEP()	VIMAGE_LIST_RUNLOCK_NOSLEEP()

/*
 * EVENTHANDLER(9) extensions, include <sys/eventhandler.h> to expose those.
 */
#ifdef EVENTHANDLER_REGISTER
#ifdef VIMAGE
#define VNET_GLOBAL_EVENTHANDLER_REGISTER_TAG(tag, name, func, arg, priority) \
do {									\
	if (IS_DEFAULT_VNET(curvnet)) {					\
		(tag) = vimage_eventhandler_register(NULL, #name, func,	\
		    arg, priority,					\
		    vimage_global_eventhandler_iterator_func,		\
		    &vnet_data);					\
	}								\
} while(0)
#define VNET_GLOBAL_EVENTHANDLER_REGISTER(name, func, arg, priority)	\
do {									\
	if (IS_DEFAULT_VNET(curvnet)) {					\
		vimage_eventhandler_register(NULL, #name, func,		\
		    arg, priority,					\
		    vimage_global_eventhandler_iterator_func,		\
		    &vnet_data);					\
	}								\
} while(0)
#else /* !VIMAGE */
#define VNET_GLOBAL_EVENTHANDLER_REGISTER_TAG(tag, name, func, arg, priority) \
	(tag) = eventhandler_register(NULL, #name, func, arg, priority)
#define VNET_GLOBAL_EVENTHANDLER_REGISTER(name, func, arg, priority)	\
	eventhandler_register(NULL, #name, func, arg, priority)
#endif /* VIMAGE */
#endif /* EVENTHANDLER_REGISTER */

/*
 * Sysctl variants for VNET-virtualized global variables.  Include
 * <sys/sysctl.h> to expose these definitions.
 */
#ifdef SYSCTL_OID
#define	SYSCTL_VNET_INT(parent, nbr, name, access, ptr, val, descr)	\
	VIMAGE_SYSCTL_INT(parent, nbr, name, access, ptr, val, descr,	\
	    CTLFLAG_VNET)
#define	SYSCTL_VNET_PROC(parent, nbr, name, access, ptr, arg, handler,	\
	    fmt, descr)							\
	VIMAGE_SYSCTL_PROC(parent, nbr, name, access, ptr, arg,	handler,\
	    fmt, descr, CTLFLAG_VNET)
#define	SYSCTL_VNET_STRING(parent, nbr, name, access, arg, len, descr)	\
	VIMAGE_SYSCTL_STRING(parent, nbr, name, access, arg, len, descr,\
	    CTLFLAG_VNET)
#define	SYSCTL_VNET_STRUCT(parent, nbr, name, access, ptr, type, descr)	\
	VIMAGE_SYSCTL_STRUCT(parent, nbr, name, access, ptr, type, descr,\
	    CTLFLAG_VNET)
#define	SYSCTL_VNET_UINT(parent, nbr, name, access, ptr, val, descr)	\
	VIMAGE_SYSCTL_UINT(parent, nbr, name, access, ptr, val, descr,	\
	    CTLFLAG_VNET)
#define	VNET_SYSCTL_ARG(req, arg1)					\
	VIMAGE_SYSCTL_ARG(req, arg1, &vnet_data)
#endif /* SYSCTL_OID */

/*
 * Virtual sysinit mechanism, allowing network stack components to declare
 * startup and shutdown methods to be run when virtual network stack
 * instances are created and destroyed.
 */
#include <sys/kernel.h>

/*
 * SYSINIT/SYSUNINIT variants that provide per-vnet constructors and
 * destructors.
 */
#define	VNET_SYSINIT(ident, subsystem, order, func, arg)		\
    VIMAGE_SYSINIT(ident, subsystem, order, func, arg, vnet, &vnet_data)
#define	VNET_SYSUNINIT(ident, subsystem, order, func, arg)		\
    VIMAGE_SYSUNINIT(ident, subsystem, order, func, arg, vnet, &vnet_data)

#endif /* _KERNEL */

#endif /* !_NET_VNET_H_ */
