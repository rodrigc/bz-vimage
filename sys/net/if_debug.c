/*-
 * Copyright (c) 2010 Bjoern A. Zeeb <bz@FreeBSD.org>
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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
#include <sys/socket.h>
#include <sys/types.h>

#ifdef DDB
#include <ddb/ddb.h>
#include <ddb/db_lex.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#endif

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_var.h>
#include <net/vnet.h>

#ifdef DDB
struct ifindex_entry {
	struct  ifnet *ife_ifnet;
};
VNET_DECLARE(struct ifindex_entry *, ifindex_table);
#define	V_ifindex_table		VNET(ifindex_table)

static void
if_show_ifaddr(struct ifaddr *ifa)
{

#define	IFA_DB_RPINTF(f, e)	db_printf("\t   %s = " f "\n", #e, ifa->e);
#define	IFA_DB_RPINTF_PTR(f, e)	db_printf("\t   %s = " f "\n", #e, &ifa->e);
#define	IFA_DB_RPINTF_DPTR(f, e)	db_printf("\t  *%s = " f "\n", #e, *ifa->e);
	db_printf("\tifa = %p\n", ifa);
	IFA_DB_RPINTF("%p", ifa_addr);
	IFA_DB_RPINTF("%p", ifa_dstaddr);
	IFA_DB_RPINTF("%p", ifa_netmask);
	IFA_DB_RPINTF_PTR("%p", if_data);
	IFA_DB_RPINTF("%p", ifa_ifp);
	IFA_DB_RPINTF_PTR("%p", ifa_link);
	IFA_DB_RPINTF("%p", ifa_link.tqe_next);
	IFA_DB_RPINTF("%p", ifa_link.tqe_prev);
	IFA_DB_RPINTF_DPTR("%p", ifa_link.tqe_prev);
	IFA_DB_RPINTF("%p", ifa_rtrequest);
	IFA_DB_RPINTF("0x%04x", ifa_flags);
	IFA_DB_RPINTF("%u", ifa_refcnt);
	IFA_DB_RPINTF("%d", ifa_metric);
	IFA_DB_RPINTF("%p", ifa_claim_addr);
	IFA_DB_RPINTF_PTR("%p", ifa_mtx);

#undef IFA_DB_RPINTF_DPTR
#undef IFA_DB_RPINTF_PTR
#undef IFA_DB_RPINTF
}

DB_SHOW_COMMAND(ifaddr, db_show_ifaddr)
{
	struct ifaddr *ifa;

	ifa = (struct ifaddr *)addr;
	if (ifa == NULL) {
		db_printf("usage: show ifaddr <struct ifaddr *>\n");
		return;
	}

	if_show_ifaddr(ifa);
}

static void
if_show_ifnet(struct ifnet *ifp)
{
	struct ifaddr *ifa;

	if (ifp == NULL)
		return;
	db_printf("%s:\n", ifp->if_xname);
#define	IF_DB_PRINTF(f, e)	db_printf("   %s = " f "\n", #e, ifp->e);
#define	IF_DB_PRINTF_PTR(f, e)	db_printf("   %s = " f "\n", #e, &ifp->e);
#define	IF_DB_PRINTF_DPTR(f, e)	db_printf("  *%s = " f "\n", #e, *ifp->e);
	IF_DB_PRINTF("%p", if_softc);
	IF_DB_PRINTF("%p", if_l2com);
	IF_DB_PRINTF("%p", if_vnet);
	IF_DB_PRINTF("%p", if_link.tqe_next);
	IF_DB_PRINTF("%p", if_link.tqe_prev);
	IF_DB_PRINTF("%s", if_xname);
	IF_DB_PRINTF("%s", if_dname);
	IF_DB_PRINTF("%d", if_dunit);
	IF_DB_PRINTF("%u", if_refcount);
	IF_DB_PRINTF_PTR("%p", if_addrhead);
	IF_DB_PRINTF("%p", if_addrhead.tqh_first);
	IF_DB_PRINTF("%p", if_addrhead.tqh_last);
	IF_DB_PRINTF_DPTR("%p", if_addrhead.tqh_last);
	/* XXX-BZ need a DB show for that. */
	TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link) {
		if_show_ifaddr(ifa);
	}
	IF_DB_PRINTF("%u", if_pcount);
	IF_DB_PRINTF("%p", if_carp);
	IF_DB_PRINTF("%p", if_bpf);
	IF_DB_PRINTF("%u", if_index);
	IF_DB_PRINTF("%d", if_index_reserved);
	IF_DB_PRINTF("%p", if_vlantrunk);
	IF_DB_PRINTF("0x%08x", if_flags);
	IF_DB_PRINTF("0x%08x", if_capabilities);
	IF_DB_PRINTF("0x%08x", if_capenable);
	IF_DB_PRINTF("%p", if_linkmib);
	IF_DB_PRINTF("%zu", if_linkmiblen);
	IF_DB_PRINTF_PTR("%p", if_data);
	/* XXX-BZ need a DB show for that. */
	IF_DB_PRINTF_PTR("%p", if_multiaddrs);
	/* XXX-BZ need a DB show for that. */
	IF_DB_PRINTF("%d", if_amcount);

	/* XXX-BZ should look up function names as well. */
	IF_DB_PRINTF("%p", if_output);
	IF_DB_PRINTF("%p", if_input);
	IF_DB_PRINTF("%p", if_start);
	IF_DB_PRINTF("%p", if_ioctl);
	IF_DB_PRINTF("%p", if_init);
	IF_DB_PRINTF("%p", if_resolvemulti);
	IF_DB_PRINTF("%p", if_qflush);
	IF_DB_PRINTF("%p", if_transmit);
	IF_DB_PRINTF("%p", if_reassign);

	IF_DB_PRINTF("%p", if_home_vnet);
	IF_DB_PRINTF("%p", if_addr);
	IF_DB_PRINTF("%p", if_llsoftc);
	IF_DB_PRINTF("0x%08x", if_drv_flags);

	/* XXX-BZ factor this out into its own DB show */
	IF_DB_PRINTF("%p", if_snd.ifq_head);
	IF_DB_PRINTF("%p", if_snd.ifq_tail);
	IF_DB_PRINTF("%d", if_snd.ifq_len);
	IF_DB_PRINTF("%d", if_snd.ifq_maxlen);
	IF_DB_PRINTF("%d", if_snd.ifq_drops);
	IF_DB_PRINTF("%p", if_snd.ifq_drv_head);
	IF_DB_PRINTF("%p", if_snd.ifq_drv_tail);
	IF_DB_PRINTF("%d", if_snd.ifq_drv_len);
	IF_DB_PRINTF("%d", if_snd.ifq_drv_maxlen);
	IF_DB_PRINTF("%d", if_snd.altq_type);
	IF_DB_PRINTF("%x", if_snd.altq_flags);

	IF_DB_PRINTF("%p", if_broadcastaddr);
	IF_DB_PRINTF("%p", if_bridge);
	IF_DB_PRINTF("%p", if_label);
	IF_DB_PRINTF_PTR("%p", if_prefixhead);
	/* XXX-BZ iterate over all non-NULL. */
	IF_DB_PRINTF("%p", if_afdata);
	IF_DB_PRINTF("%d", if_afdata_initialized);
	IF_DB_PRINTF_PTR("%p", if_afdata_lock);
	IF_DB_PRINTF_PTR("%p", if_linktask);
	IF_DB_PRINTF_PTR("%p", if_addr_mtx);
	/* if_clones */
	/* if_groups */
	IF_DB_PRINTF("%p", if_pf_kif);
	IF_DB_PRINTF("%p", if_lagg);
	IF_DB_PRINTF("%u", if_alloctype);
	/* if_cspare */
	IF_DB_PRINTF("%s", if_description);
	/* if_pspare */
	/* if_ispare*/
#undef IF_DB_PRINTF_DPTR
#undef IF_DB_PRINTF_PTR
#undef IF_DB_PRINTF
}

static struct ifnet *
db_get_ifp(db_expr_t addr, boolean_t have_addr, db_expr_t count, char *modif)
{
	struct ifnet *ifp;
#ifdef VIMAGE
	struct vnet *vnet;
	db_expr_t value;
#endif
	u_short idx;
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
	ifp = NULL;
	if (!have_addr) {
		/* Try to lookup the interface by name (on the right vnet). */
#ifdef VIMAGE
		value = 0;
		do {
			if (db_get_variable_s("$db_vnet", &value) &&
			    value != 0)
				break;
			if (db_get_variable_s("$curvnet", &value) &&
			    value != 0)
				break;
			/*
			 * We could iterate over all vnets but in that case
			 * people should use show all ifnets.
			 * XXX-BZ would do that if I could return all vnet|ifp.
			 * For now do not fail but use vnet0.
			 */
			db_printf("Neither $db_vnet nor $curvnet set. "
			    "Using vnet0.\n");
			value = (db_expr_t)vnet0;
		} while (0);
		vnet = (struct vnet *)value;
#endif
		CURVNET_SET_QUIET(vnet);
		for (idx = 1; idx <= V_if_index; idx++) {
			ifp = V_ifindex_table[idx].ife_ifnet;
			if (ifp == NULL)
				continue;
			if (!strcmp(ifp->if_xname, (const char *)addr))
				goto found;
		}
		ifp = NULL;
found:
		CURVNET_RESTORE();
	} else {
		ifp = (struct ifnet *)addr;
	}

	return (ifp);
}

DB_SHOW_COMMAND_FLAGS(ifnet, db_show_ifnet, CS_OWN)
{
	struct ifnet *ifp;

	ifp = db_get_ifp(addr, have_addr, count, modif);
	if (ifp == NULL) {
		db_printf("usage: show ifnet <struct ifnet *>|if_xname\n");
		return;
	}

	if_show_ifnet(ifp);
}

DB_SHOW_ALL_COMMAND(ifnets, db_show_all_ifnets)
{
	VNET_ITERATOR_DECL(vnet_iter);
	struct ifnet *ifp;
	u_short idx;

	VNET_FOREACH(vnet_iter) {
		CURVNET_SET_QUIET(vnet_iter);
#ifdef VIMAGE
		db_printf("vnet=%p\n", curvnet);
#endif
		for (idx = 1; idx <= V_if_index; idx++) {
			ifp = V_ifindex_table[idx].ife_ifnet;
			if (ifp == NULL)
				continue;
			db_printf( "%20s ifp=%p\n", ifp->if_xname, ifp);
			if (db_pager_quit)
				break;
		}
		CURVNET_RESTORE();
	}
}
#endif
