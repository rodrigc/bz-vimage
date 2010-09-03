/*-
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if.c	8.5 (Berkeley) 1/9/95
 * $FreeBSD: src/sys/net/if_clone.c,v 1.20 2010/04/11 18:47:38 bz Exp $
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/limits.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/jail.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sx.h>

#include <net/if.h>
#include <net/if_clone.h>
#if 0
#include <net/if_dl.h>
#endif
#include <net/if_types.h>
#include <net/if_var.h>
#include <net/radix.h>
#include <net/route.h>
#include <net/vnet.h>

#ifdef DDB
#include <ddb/ddb.h>
#include <ddb/db_sym.h>
#endif

static void	if_clone_free(struct if_clone_instance *ifci);
static int	if_clone_createif(struct if_clone *ifc, char *name, size_t len,
		    caddr_t params);

static struct mtx	if_cloners_mtx;
static struct sx	if_cloners_master_sx;
static int		if_cloners_count;
static LIST_HEAD(, if_clone)	if_cloners_master =
    LIST_HEAD_INITIALIZER(if_cloners_master);
static VNET_DEFINE(LIST_HEAD(, if_clone_instance), if_cloners) =
    LIST_HEAD_INITIALIZER(if_cloners);
#define	V_if_cloners		VNET(if_cloners)

#ifdef VIMAGE
static void
v_ifci_assert(void)
{

	VNET_ASSERT(curvnet != NULL, ("%s: curvnet not set", __func__));
}
#define	V_IFCI(ifc)							\
	(v_ifci_assert(), (struct if_clone_instance *)			\
	    (curvnet->v.v_data_base + (uintptr_t)ifc->ifc_data))
#else
#define	V_IFCI(ifc)		ifc->ifc_data
#endif

SX_SYSINIT(if_cloners_master_sx, &if_cloners_master_sx, "if_cloners_master_sx");
#define	IF_CLONERS_MASTER_WLOCK()	sx_xlock(&if_cloners_master_sx)
#define	IF_CLONERS_MASTER_WUNLOCK()	sx_xunlock(&if_cloners_master_sx)
#define	IF_CLONERS_MASTER_RLOCK()	sx_slock(&if_cloners_master_sx)
#define	IF_CLONERS_MASTER_RUNLOCK()	sx_sunlock(&if_cloners_master_sx)

MTX_SYSINIT(if_cloners_mtx, &if_cloners_mtx, "if_cloners lock", MTX_DEF);
#define IF_CLONERS_LOCK_ASSERT()	mtx_assert(&if_cloners_mtx, MA_OWNED)
#define IF_CLONERS_LOCK()		mtx_lock(&if_cloners_mtx)
#define IF_CLONERS_UNLOCK()		mtx_unlock(&if_cloners_mtx)

#define IF_CLONE_LOCK_INIT(ifci)		\
    mtx_init(&(ifci)->ifci_mtx, "if_clone_instance lock", NULL, MTX_DEF)
#define IF_CLONE_LOCK_DESTROY(ifc)	mtx_destroy(&(ifci)->ifci_mtx)
#define IF_CLONE_LOCK_ASSERT(ifc)	mtx_assert(&(ifci)->ifci_mtx, MA_OWNED)
#define IF_CLONE_LOCK(ifc)		mtx_lock(&(ifci)->ifci_mtx)
#define IF_CLONE_UNLOCK(ifc)		mtx_unlock(&(ifci)->ifci_mtx)

#define IF_CLONE_ADDREF(ifci)						\
	do {								\
		IF_CLONE_LOCK(ifci);					\
		IF_CLONE_ADDREF_LOCKED(ifci);				\
		IF_CLONE_UNLOCK(ifci);					\
	} while (0)
#define IF_CLONE_ADDREF_LOCKED(ifci)					\
	do {								\
		IF_CLONE_LOCK_ASSERT(ifci);				\
		KASSERT((ifci)->ifci_refcnt >= 0,			\
		    ("negative refcnt %ld", (ifci)->ifci_refcnt));	\
		(ifci)->ifci_refcnt++;					\
	} while (0)
#define IF_CLONE_REMREF(ifci)						\
	do {								\
		IF_CLONE_LOCK(ifci);					\
		IF_CLONE_REMREF_LOCKED(ifci);				\
	} while (0)
#define IF_CLONE_REMREF_LOCKED(ifci)					\
	do {								\
		IF_CLONE_LOCK_ASSERT(ifci);				\
		KASSERT((ifci)->ifci_refcnt > 0,			\
		    ("bogus refcnt %ld", (ifci)->ifci_refcnt));		\
		if (--(ifci)->ifci_refcnt == 0) {			\
			IF_CLONE_UNLOCK(ifci);				\
			if_clone_free(ifci);				\
		} else {						\
			/* silently free the lock */			\
			IF_CLONE_UNLOCK(ifci);				\
		}							\
	} while (0)

#define IFC_IFLIST_INSERT(_ifci, _ifp)					\
	LIST_INSERT_HEAD(&_ifci->ifci_iflist, _ifp, if_clones)
#define IFC_IFLIST_REMOVE(_ifci, _ifp)					\
	LIST_REMOVE(_ifp, if_clones)

static MALLOC_DEFINE(M_CLONE, "clone", "interface cloning framework");

/*
 * Lookup and create a clone network interface.
 */
int
if_clone_create(char *name, size_t len, caddr_t params)
{
	struct if_clone *ifc;
	int error;

	/* Try to find an applicable cloner for this request */
	IF_CLONERS_MASTER_RLOCK();
	LIST_FOREACH(ifc, &if_cloners_master, ifc_list) {
		if (ifc->ifc_match(ifc, name))
			break;
	}
	IF_CLONERS_MASTER_RUNLOCK();

	if (ifc == NULL)
		return (EINVAL);

	error = if_clone_createif(ifc, name, len, params);
	return (error);
}

/*
 * Create a clone network interface.
 */
static int
if_clone_createif(struct if_clone *ifc, char *name, size_t len, caddr_t params)
{
	int err;
	struct ifnet *ifp;
	struct if_clone_instance *ifci;

	if (ifunit(name) != NULL)
		return (EEXIST);

	err = (*ifc->ifc_create)(ifc, name, len, params);
	
	if (!err) {
		ifp = ifunit(name);
		if (ifp == NULL)
			panic("%s: lookup failed for %s", __func__, name);

		if_addgroup(ifp, ifc->ifc_name);

		ifci = V_IFCI(ifc);
		IF_CLONE_LOCK(ifci);
		IFC_IFLIST_INSERT(ifci, ifp);
		IF_CLONE_UNLOCK(ifci);
	}

	return (err);
}

/*
 * Lookup and destroy a clone network interface.
 */
int
if_clone_destroy(const char *name)
{
	int err;
	struct if_clone *ifc;
	struct ifnet *ifp;

	ifp = ifunit_ref(name);
	if (ifp == NULL)
		return (ENXIO);

	/* Find the cloner for this interface */
	IF_CLONERS_MASTER_RLOCK();
	LIST_FOREACH(ifc, &if_cloners_master, ifc_list) {
		if (strcmp(ifc->ifc_name, ifp->if_dname) == 0) {
			break;
		}
	}
	IF_CLONERS_MASTER_RUNLOCK();
	if (ifc == NULL) {
		if_rele(ifp);
		return (EINVAL);
	}

	err = if_clone_destroyif(ifc, ifp);
	if_rele(ifp);
	return err;
}

/*
 * Destroy a clone network interface.
 */
int
if_clone_destroyif(struct if_clone *ifc, struct ifnet *ifp)
{
	int err;
	struct ifnet *ifcifp;
	struct if_clone_instance *ifci;

	if (ifc->ifc_destroy == NULL)
		return(EOPNOTSUPP);

	/*
	 * Given that the cloned ifnet might be attached to a different
	 * vnet from where its cloner was registered, we have to
	 * switch to the vnet context of the target vnet.
	 */
	CURVNET_SET_QUIET(ifp->if_vnet);

	ifci = V_IFCI(ifc);
	IF_CLONE_LOCK(ifci);
	LIST_FOREACH(ifcifp, &ifci->ifci_iflist, if_clones) {
		if (ifcifp == ifp) {
			IFC_IFLIST_REMOVE(ifci, ifp);
			break;
		}
	}
	IF_CLONE_UNLOCK(ifci);
	if (ifcifp == NULL) {
		CURVNET_RESTORE();
		return (ENXIO);		/* ifp is not on the list. */
	}

	if_delgroup(ifp, ifc->ifc_name);

	err =  (*ifc->ifc_destroy)(ifc, ifp);

	if (err != 0) {
		if_addgroup(ifp, ifc->ifc_name);

		IF_CLONE_LOCK(ifci);
		IFC_IFLIST_INSERT(ifci, ifp);
		IF_CLONE_UNLOCK(ifci);
	}
	CURVNET_RESTORE();
	return (err);
}

/*
 * Register a network interface cloner.
 * For historic reason is called *attach() but should really be *register().
 */
static void
_if_clone_attach(struct if_clone *ifc)
{
	struct if_clone_instance *ifci;
	int len, maxclone;

	/*
	 * As we are properly registering each instance here, curvnet must be
	 * set. V_IFCI() will do the assertion for us.
	 */
	ifci = V_IFCI(ifc);
	ifci->ifci_ifcp = ifc;

	/*
	 * Compute bitmap size and allocate it.
	 */
	maxclone = ifc->ifc_maxunit + 1;
	len = maxclone >> 3;
	if ((len << 3) < maxclone)
		len++;
	ifci->ifci_units = malloc(len, M_CLONE, M_WAITOK | M_ZERO);
	ifc->ifc_bmlen = len;
	IF_CLONE_LOCK_INIT(ifci);
	IF_CLONE_ADDREF(ifci);
	LIST_INIT(&ifci->ifci_iflist);

	IF_CLONERS_LOCK();
	LIST_INSERT_HEAD(&V_if_cloners, ifci, ifci_list);
	IF_CLONERS_UNLOCK();

	if (ifc->ifc_attach != NULL)
		(*ifc->ifc_attach)(ifc);
	EVENTHANDLER_INVOKE(if_clone_event, ifc);
}

static void
vnet_if_clone_attach(const void *unused __unused)
{
	struct if_clone *ifc;

	if (IS_DEFAULT_VNET(curvnet))
		return;

	IF_CLONERS_MASTER_WLOCK();
	LIST_FOREACH(ifc, &if_cloners_master, ifc_list) {
		_if_clone_attach(ifc);
	}
	IF_CLONERS_MASTER_WUNLOCK();
}
VNET_SYSINIT(if_clone, SI_SUB_PROTO_IFATTACHDOMAIN, SI_ORDER_ANY,
    vnet_if_clone_attach, NULL);

void
if_clone_attach(struct if_clone *ifc)
{
	VNET_ITERATOR_DECL(vnet_iter);

	/*
	 * As we are only registering our master copy here curvnet should not
	 * be set.  It is upon boot unfortunately from vimage_alloc(), so we
	 * cannot assert this.
	 */

	IF_CLONERS_MASTER_WLOCK();
	LIST_INSERT_HEAD(&if_cloners_master, ifc, ifc_list);
	if_cloners_count++;

	VNET_LIST_RLOCK();
	VNET_FOREACH(vnet_iter) {
		CURVNET_SET_QUIET(vnet_iter);
		_if_clone_attach(ifc);
		CURVNET_RESTORE();
	}
	VNET_LIST_RUNLOCK();
	IF_CLONERS_MASTER_WUNLOCK();
}


/*
 * Unregister a network interface cloner.
 */
static void
_if_clone_detach(struct if_clone *ifc)
{
	struct if_clone_instance *ifci;

	/*
	 * As we are properly unregistering each instance here,
	 * curvnet must be set. V_IFCI() will do the assertion for us.
	 */
	ifci = V_IFCI(ifc);
	/* Allow all simples to be destroyed */
	if (ifc->ifc_attach == ifc_simple_attach)
		ifci->ifci_minifs = 0;

	/* destroy all interfaces for this cloner */
	while (!LIST_EMPTY(&ifci->ifci_iflist))
		if_clone_destroyif(ifc, LIST_FIRST(&ifci->ifci_iflist));
	
	IF_CLONE_REMREF(ifci);
}

static void
vnet_if_clone_detach(const void *unused __unused)
{
	struct if_clone *ifc;

	if (IS_DEFAULT_VNET(curvnet))
		return;

	IF_CLONERS_MASTER_WLOCK();
	LIST_FOREACH(ifc, &if_cloners_master, ifc_list) {
		_if_clone_detach(ifc);
	}
	IF_CLONERS_MASTER_WUNLOCK();
}
VNET_SYSUNINIT(if_clone, SI_SUB_PROTO_IFATTACHDOMAIN, SI_ORDER_ANY,
    vnet_if_clone_detach, NULL);

void
if_clone_detach(struct if_clone *ifc)
{
	VNET_ITERATOR_DECL(vnet_iter);

	IF_CLONERS_MASTER_WLOCK();
	LIST_REMOVE(ifc, ifc_list);
	if_cloners_count--;

	VNET_LIST_RLOCK();
	VNET_FOREACH(vnet_iter) {
		CURVNET_SET_QUIET(vnet_iter);
		_if_clone_detach(ifc);
		CURVNET_RESTORE();
	}
	VNET_LIST_RUNLOCK();
	IF_CLONERS_MASTER_WUNLOCK();
}

static void
if_clone_free(struct if_clone_instance *ifci)
{
	struct if_clone *ifc;

	ifc = ifci->ifci_ifcp;
	for (int bytoff = 0; bytoff < ifc->ifc_bmlen; bytoff++) {
		KASSERT(ifci->ifci_units[bytoff] == 0x00,
		    ("ifci_units[%d] is not empty", bytoff));
	}

	KASSERT(LIST_EMPTY(&ifci->ifci_iflist),
	    ("%s: ifci_iflist not empty", __func__));

	IF_CLONE_LOCK_DESTROY(ifci);
	free(ifci->ifci_units, M_CLONE);
}

/*
 * Provide list of interface cloners to userspace.
 */
int
if_clone_list(struct if_clonereq *ifcr)
{
	char *buf, *dst, *outbuf = NULL;
	struct if_clone *ifc;
	int buf_count, count, err = 0;

	if (ifcr->ifcr_count < 0)
		return (EINVAL);

	IF_CLONERS_MASTER_RLOCK();
	/*
	 * Set our internal output buffer size.  We could end up not
	 * reporting a cloner that is added between the unlock and lock
	 * below, but that's not a major problem.  Not caping our
	 * allocation to the number of cloners actually in the system
	 * could be because that would let arbitrary users cause us to
	 * allocate abritrary amounts of kernel memory.
	 */
	buf_count = (if_cloners_count < ifcr->ifcr_count) ?
	    if_cloners_count : ifcr->ifcr_count;
	IF_CLONERS_MASTER_RUNLOCK();

	outbuf = malloc(IFNAMSIZ*buf_count, M_CLONE, M_WAITOK | M_ZERO);

	IF_CLONERS_MASTER_RLOCK();
	ifcr->ifcr_total = if_cloners_count;
	if ((dst = ifcr->ifcr_buffer) == NULL) {
		/* Just asking how many there are. */
		goto done;
	}
	count = (if_cloners_count < buf_count) ?
	    if_cloners_count : buf_count;

	for (ifc = LIST_FIRST(&if_cloners_master), buf = outbuf;
	    ifc != NULL && count != 0;
	    ifc = LIST_NEXT(ifc, ifc_list), count--, buf += IFNAMSIZ) {
		strlcpy(buf, ifc->ifc_name, IFNAMSIZ);
	}

done:
	IF_CLONERS_MASTER_RUNLOCK();
	if (err == 0)
		err = copyout(outbuf, dst, buf_count*IFNAMSIZ);
	if (outbuf != NULL)
		free(outbuf, M_CLONE);
	return (err);
}

/*
 * A utility function to extract unit numbers from interface names of
 * the form name###.
 *
 * Returns 0 on success and an error on failure.
 */
int
ifc_name2unit(const char *name, int *unit)
{
	const char	*cp;
	int		cutoff = INT_MAX / 10;
	int		cutlim = INT_MAX % 10;

	for (cp = name; *cp != '\0' && (*cp < '0' || *cp > '9'); cp++);
	if (*cp == '\0') {
		*unit = -1;
	} else if (cp[0] == '0' && cp[1] != '\0') {
		/* Disallow leading zeroes. */
		return (EINVAL);
	} else {
		for (*unit = 0; *cp != '\0'; cp++) {
			if (*cp < '0' || *cp > '9') {
				/* Bogus unit number. */
				return (EINVAL);
			}
			if (*unit > cutoff ||
			    (*unit == cutoff && *cp - '0' > cutlim))
				return (EINVAL);
			*unit = (*unit * 10) + (*cp - '0');
		}
	}

	return (0);
}

int
ifc_alloc_unit(struct if_clone *ifc, int *unit)
{
	struct if_clone_instance *ifci;
	int wildcard, bytoff, bitoff;
	int err = 0;

	ifci = V_IFCI(ifc);
	IF_CLONE_LOCK(ifci);

	bytoff = bitoff = 0;
	wildcard = (*unit < 0);
	/*
	 * Find a free unit if none was given.
	 */
	if (wildcard) {
		while ((bytoff < ifc->ifc_bmlen)
		    && (ifci->ifci_units[bytoff] == 0xff))
			bytoff++;
		if (bytoff >= ifc->ifc_bmlen) {
			err = ENOSPC;
			goto done;
		}
		while ((ifci->ifci_units[bytoff] & (1 << bitoff)) != 0)
			bitoff++;
		*unit = (bytoff << 3) + bitoff;
	}

	if (*unit > ifc->ifc_maxunit) {
		err = ENOSPC;
		goto done;
	}

	if (!wildcard) {
		bytoff = *unit >> 3;
		bitoff = *unit - (bytoff << 3);
	}

	if((ifci->ifci_units[bytoff] & (1 << bitoff)) != 0) {
		err = EEXIST;
		goto done;
	}
	/*
	 * Allocate the unit in the bitmap.
	 */
	KASSERT((ifci->ifci_units[bytoff] & (1 << bitoff)) == 0,
	    ("%s: bit is already set", __func__));
	ifci->ifci_units[bytoff] |= (1 << bitoff);
	IF_CLONE_ADDREF_LOCKED(ifci);

done:
	IF_CLONE_UNLOCK(ifci);
	return (err);
}

void
ifc_free_unit(struct if_clone *ifc, int unit)
{
	struct if_clone_instance *ifci;
	int bytoff, bitoff;

	/*
	 * Compute offset in the bitmap and deallocate the unit.
	 */
	bytoff = unit >> 3;
	bitoff = unit - (bytoff << 3);

	ifci = V_IFCI(ifc);
	IF_CLONE_LOCK(ifci);
	KASSERT((ifci->ifci_units[bytoff] & (1 << bitoff)) != 0,
	    ("%s: bit is already cleared", __func__));
	ifci->ifci_units[bytoff] &= ~(1 << bitoff);
	IF_CLONE_REMREF_LOCKED(ifci);	/* releases lock */
}

void
ifc_simple_attach(struct if_clone *ifc)
{
	int err;
	int unit;
	char name[IFNAMSIZ];
	struct if_clone_instance *ifci;

	ifci = V_IFCI(ifc);
	KASSERT(ifci->ifci_minifs - 1 <= ifc->ifc_maxunit,
	    ("%s: %s requested more units than allowed (%d > %d)",
	    __func__, ifc->ifc_name, ifci->ifci_minifs,
	    ifc->ifc_maxunit + 1));

	for (unit = 0; unit < ifci->ifci_minifs; unit++) {
		snprintf(name, IFNAMSIZ, "%s%d", ifc->ifc_name, unit);
		err = if_clone_createif(ifc, name, IFNAMSIZ, NULL);
		KASSERT(err == 0,
		    ("%s: failed to create required interface %s: %d",
		    __func__, name, err));
	}
}

int
ifc_simple_match(struct if_clone *ifc, const char *name)
{
	const char *cp;
	int i;
	
	/* Match the name */
	for (cp = name, i = 0; i < strlen(ifc->ifc_name); i++, cp++) {
		if (ifc->ifc_name[i] != *cp)
			return (0);
	}

	/* Make sure there's a unit number or nothing after the name */
	for (; *cp != '\0'; cp++) {
		if (*cp < '0' || *cp > '9')
			return (0);
	}

	return (1);
}

int
ifc_simple_create(struct if_clone *ifc, char *name, size_t len, caddr_t params)
{
	struct if_clone_instance *ifci;
	struct ifnet *ifp;
	char *dp;
	int wildcard;
	int unit;
	int err;

	err = ifc_name2unit(name, &unit);
	if (err != 0)
		return (err);

	wildcard = (unit < 0);

	err = ifc_alloc_unit(ifc, &unit);
	if (err != 0)
		return (err);

	if (ifc->ifc_if_type > 0) {
		ifp = if_alloc_curvnet(ifc->ifc_if_type);
		if (ifp == NULL) {
			ifci = V_IFCI(ifc);
			ifc_free_unit(ifc, unit);
			return (err);
		}
	} else
		ifp = NULL;

	err = ifc->ifcs_create(ifc, ifp, unit, params);
	if (err != 0) {
		if (ifc->ifc_if_type > 0)
			if_free_type(ifp, ifc->ifc_if_type);
		ifci = V_IFCI(ifc);
		ifc_free_unit(ifc, unit);
		return (err);
	}

	/* In the wildcard case, we need to update the name. */
	if (wildcard) {
		for (dp = name; *dp != '\0'; dp++);
		if (snprintf(dp, len - (dp-name), "%d", unit) >
		    len - (dp-name) - 1) {
			/*
			 * This can only be a programmer error and
			 * there's no straightforward way to recover if
			 * it happens.
			 */
			panic("if_clone_create(): interface name too long");
		}

	}

	return (0);
}

int
ifc_simple_destroy(struct if_clone *ifc, struct ifnet *ifp)
{
	int unit;
	struct if_clone_instance *ifci;

	ifci = V_IFCI(ifc);
	unit = ifp->if_dunit;

	if (unit < ifci->ifci_minifs) 
		return (EINVAL);

	ifc->ifcs_destroy(ifp);

	if (ifc->ifc_if_type > 0)
		if_free_type(ifp, ifc->ifc_if_type);
	ifc_free_unit(ifc, unit);

	return (0);
}

#ifdef DDB
static void
db_ifci_print(struct if_clone_instance *ifci)
{

	db_printf(" ifci=%p\n", ifci);
	db_printf("  ifci_list=%p\n", &ifci->ifci_list);
	db_printf("  ifci_mtx=%p\n", &ifci->ifci_mtx);
	db_printf("  ifci_ifcp=%p\n", ifci->ifci_ifcp);
	db_printf("  ifci_units=%p\n", ifci->ifci_units);
	db_printf("  ifci_iflist=%p\n", &ifci->ifci_iflist);
	db_printf("  ifci_refcnt=%ld\n", ifci->ifci_refcnt);
	db_printf("  ifci_minifs=%d\n", ifci->ifci_minifs);
}

DB_SHOW_COMMAND(ifci, db_show_ifci)
{

	if (!have_addr || addr == 0) {
		db_printf("usage: show ifci <struct if_clone_instance *>\n");
		return;
	}
	db_ifci_print((struct if_clone_instance *)addr);
}

DB_SHOW_ALL_COMMAND(ifci, db_show_all_ifci)
{
	VNET_ITERATOR_DECL(vnet_iter);
	struct if_clone_instance *ifci;

	VNET_FOREACH(vnet_iter) {
		CURVNET_SET_QUIET(vnet_iter);
#ifdef VIMAGE
		db_printf("vnet=%p:\n", curvnet);
#endif
		LIST_FOREACH(ifci, &V_if_cloners, ifci_list) {
			db_ifci_print(ifci);
			if (db_pager_quit) 
				break;
		}
		CURVNET_RESTORE();
		db_printf("\n");
		if (db_pager_quit) 
			return;
	}
}

static void
db_ifc_print(struct if_clone *ifc)
{
	const char *fname;
	c_db_sym_t sym;
	db_expr_t offset;

	db_printf(" ifc=%p\n", ifc);
	db_printf("  ifc_list=%p\n", &ifc->ifc_list);
	db_printf("  ifc_name=%s\n", ifc->ifc_name);

#define	LSYM(_func)							\
	fname = NULL;							\
	sym = db_search_symbol((vm_offset_t)(ifc->_func), DB_STGY_PROC,	\
	    &offset);							\
	db_symbol_values(sym, &fname, NULL);				\
	db_printf("  (*" #_func ")=%p (%s)\n", ifc->_func,		\
	    (fname != NULL) ? fname : "");
	LSYM(ifc_attach);
	LSYM(ifc_match);
	LSYM(ifc_create);
	LSYM(ifc_destroy);

	LSYM(ifcs_create);
	LSYM(ifcs_destroy);
#undef LSYM

	db_printf("  ifc_data=%p\n", ifc->ifc_data);
	db_printf("  ifc_maxunit=%d\n", ifc->ifc_maxunit);
	db_printf("  ifc_bmlen=%d\n", ifc->ifc_bmlen);
	db_printf("  ifc_if_type=%u\n", ifc->ifc_if_type);
}

DB_SHOW_COMMAND(ifc, db_show_ifc)
{

	if (!have_addr || addr == 0) {
		db_printf("usage: show if_cloner <struct if_clone *>\n");
		return;
	}
	db_ifc_print((struct if_clone *)addr);
}

DB_SHOW_ALL_COMMAND(ifc, db_show_all_ifc)
{
	struct if_clone *ifc;

	db_printf("if_cloners_master:\n");
	LIST_FOREACH(ifc, &if_cloners_master, ifc_list) {
		db_ifc_print(ifc);
		if (db_pager_quit)
			return;
	}
}
#endif
