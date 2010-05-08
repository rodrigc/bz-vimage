/*-
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*
 * 	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/ddb/db_variables.c,v 1.24 2009/07/14 22:48:30 rwatson Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <ddb/ddb.h>
#include <ddb/db_lex.h>
#include <ddb/db_variables.h>

static int	db_find_variable(struct db_variable **varp);

static struct db_variable db_vars[] = {
	{ "radix",	&db_radix, FCN_NULL },
	{ "maxoff",	&db_maxoff, FCN_NULL },
	{ "maxwidth",	&db_max_width, FCN_NULL },
	{ "tabstops",	&db_tab_stop_width, FCN_NULL },
	{ "lines",	&db_lines_per_page, FCN_NULL },
	{ "curcpu",	NULL, db_var_curcpu },
	{ "db_cpu",	NULL, db_var_db_cpu },
};
static struct db_variable *db_evars =
	db_vars + sizeof(db_vars)/sizeof(db_vars[0]);

#ifdef VIMAGE
#include <sys/malloc.h>
#include <sys/vimage.h>

static MALLOC_DEFINE(M_DDB_VIMAGE_VAR, "ddb_vimage_var",
    "DDB VIMAGE variables");

struct db_vimage_variable_l db_vimage_vars =
    LIST_HEAD_INITIALIZER(db_vimage_vars);

int
db_vimage_variable_register(struct vimage_subsys *vse)
{
	struct db_vimage_variable *vvp;
	char tmp[256];
	size_t size;

	/* cru|db_ + name + \0 */
	if ((3 + strlen(vse->name) + 1) >= sizeof(tmp))
		return (EINVAL);

	snprintf(tmp, sizeof(tmp), "cur%s", vse->name);
	size = sizeof(*vvp) + strlen(tmp) + 1;
	vvp = malloc(size, M_DDB_VIMAGE_VAR, M_WAITOK | M_ZERO);
	vvp->db_v.name = (char *)(vvp + 1);
	bcopy(tmp, vvp->db_v.name, strlen(tmp) + 1);
	vvp->db_v.fcn = db_var_curvimage;
	vvp->vse = vse;
	LIST_INSERT_HEAD(&db_vimage_vars, vvp, db_vimage_var_le);
	
	snprintf(tmp, sizeof(tmp), "db_%s", vse->name);
	size = sizeof(*vvp) + strlen(tmp) + 1;
	vvp = malloc(size, M_DDB_VIMAGE_VAR, M_WAITOK | M_ZERO);
	vvp->db_v.name = (char *)(vvp + 1);
	bcopy(tmp, vvp->db_v.name, strlen(tmp) + 1);
	vvp->db_v.fcn = db_var_db_vimage;
	vvp->vse = vse;
	LIST_INSERT_HEAD(&db_vimage_vars, vvp, db_vimage_var_le);

	return (0);
}

int
db_vimage_variable_unregister(struct vimage_subsys *vse)
{
	struct db_vimage_variable *vvp, *vvp1;

	LIST_FOREACH_SAFE(vvp, &db_vimage_vars, db_vimage_var_le, vvp1) {
		if (vvp->vse == vse) {
			LIST_REMOVE(vvp, db_vimage_var_le);
			free(vvp, M_DDB_VIMAGE_VAR);
		}
	}
	return (0);
}

static struct db_variable *
db_find_variable_vimage(char *varname)
{
	struct db_vimage_variable *vvp;

	LIST_FOREACH(vvp, &db_vimage_vars, db_vimage_var_le) {
		if (!strcmp(vvp->db_v.name, varname))
			return ((struct db_variable *)vvp);
	}
	return (NULL);
}
#endif

static int
db_find_variable(struct db_variable **varp)
{
	struct db_variable *vp;
	int t;

	t = db_read_token();
	if (t == tIDENT) {
		for (vp = db_vars; vp < db_evars; vp++) {
			if (!strcmp(db_tok_string, vp->name)) {
				*varp = vp;
				return (1);
			}
		}
#ifdef VIMAGE
		vp = db_find_variable_vimage(db_tok_string);
		if (vp != NULL) {
			*varp = vp;
			 return (1);
		}
#endif
		for (vp = db_regs; vp < db_eregs; vp++) {
			if (!strcmp(db_tok_string, vp->name)) {
				*varp = vp;
				return (1);
			}
		}
	}
	db_error("Unknown variable\n");
	return (0);
}

int
db_get_variable(db_expr_t *valuep)
{
	struct db_variable *vp;

	if (!db_find_variable(&vp))
		return (0);

	return (db_read_variable(vp, valuep));
}

int
db_set_variable(db_expr_t value)
{
	struct db_variable *vp;

	if (!db_find_variable(&vp))
		return (0);

	return (db_write_variable(vp, value));
}

int
db_read_variable(struct db_variable *vp, db_expr_t *valuep)
{
	db_varfcn_t *func = vp->fcn;

	if (func == FCN_NULL) {
		*valuep = *(vp->valuep);
		return (1);
	}
	return ((*func)(vp, valuep, DB_VAR_GET));
}

int
db_write_variable(struct db_variable *vp, db_expr_t value)
{
	db_varfcn_t *func = vp->fcn;

	if (func == FCN_NULL) {
		*(vp->valuep) = value;
		return (1);
	}
	return ((*func)(vp, &value, DB_VAR_SET));
}

void
db_set_cmd(db_expr_t dummy1, boolean_t dummy2, db_expr_t dummy3, char *dummy4)
{
	struct db_variable *vp;
	db_expr_t value;
	int t;

	t = db_read_token();
	if (t != tDOLLAR) {
		db_error("Unknown variable\n");
		return;
	}
	if (!db_find_variable(&vp)) {
		db_error("Unknown variable\n");
		return;
	}

	t = db_read_token();
	if (t != tEQ)
		db_unread_token(t);

	if (!db_expression(&value)) {
		db_error("No value\n");
		return;
	}
	if (db_read_token() != tEOL)
		db_error("?\n");

	db_write_variable(vp, value);
}

DB_SHOW_ALL_COMMAND(db_vars, db_show_all_db_vars)
{
	struct db_variable *vp;
#ifdef VIMAGE
	struct db_vimage_variable *vvp;
#endif

	for (vp = db_vars; vp < db_evars; vp++)
		db_printf("%s\n", vp->name);
#ifdef VIMAGE
	LIST_FOREACH(vvp, &db_vimage_vars, db_vimage_var_le)
		db_printf("%s\n", vvp->db_v.name);
#endif
}
