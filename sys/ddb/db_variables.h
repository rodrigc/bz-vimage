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
 *
 * $FreeBSD: src/sys/ddb/db_variables.h,v 1.15 2009/07/14 22:48:30 rwatson Exp $
 */

/*
 * 	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

#ifndef _DDB_DB_VARIABLES_H_
#define	_DDB_DB_VARIABLES_H_

/*
 * Debugger variables.
 */
struct db_variable;
typedef	int	db_varfcn_t(struct db_variable *vp, db_expr_t *valuep, int op);
struct db_variable {
	char	*name;		/* Name of variable */
	db_expr_t *valuep;	/* value of variable */
				/* function to call when reading/writing */
	db_varfcn_t *fcn;
#define DB_VAR_GET	0
#define DB_VAR_SET	1
};
#define	FCN_NULL	((db_varfcn_t *)0)

extern struct db_variable	db_regs[];	/* machine registers */
extern struct db_variable	*db_eregs;

extern db_varfcn_t	db_var_curcpu;		/* DPCPU default CPU */
extern db_varfcn_t	db_var_db_cpu;		/* DPCPU active CPU */

int db_read_variable(struct db_variable *, db_expr_t *);
int db_write_variable(struct db_variable *, db_expr_t);

#ifdef VIMAGE
extern db_varfcn_t	db_var_curvimage;	/* Default virtual instance */
extern db_varfcn_t	db_var_db_vimage;	/* Active virtual instance */

LIST_HEAD(db_vimage_variable_l, db_vimage_variable);
struct db_vimage_variable {
	struct db_variable		db_v;
	struct vimage_subsys		*vse;
	LIST_ENTRY(db_vimage_variable)	db_vimage_var_le;
};

int db_vimage_variable_register(struct vimage_subsys *);
int db_vimage_variable_unregister(struct vimage_subsys *);

int db_lookup_vimage_set_variables(struct vimage_subsys *);
#endif

#endif /* _!DDB_DB_VARIABLES_H_ */
