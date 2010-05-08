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
__FBSDID("$FreeBSD: src/sys/ddb/db_sym.c,v 1.40 2010/04/14 23:06:07 julian Exp $");

#include <sys/param.h>
#include <sys/pcpu.h>
#include <sys/smp.h>
#include <sys/systm.h>
#include <sys/vimage.h>

#include <ddb/ddb.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>

#include <opt_ddb.h>

/*
 * Multiple symbol tables
 */
#ifndef MAXNOSYMTABS
#define	MAXNOSYMTABS	3	/* mach, ux, emulator */
#endif

static db_symtab_t	db_symtabs[MAXNOSYMTABS] = {{0,},};
static int db_nsymtab = 0;

static db_symtab_t	*db_last_symtab; /* where last symbol was found */

static c_db_sym_t	db_lookup( const char *symstr);
static char		*db_qualify(c_db_sym_t sym, char *symtabname);
static boolean_t	db_symbol_is_ambiguous(c_db_sym_t sym);
static boolean_t	db_line_at_pc(c_db_sym_t, char **, int *, db_expr_t);

static int db_cpu = -1;

/*
 * Validate the CPU number used to interpret per-CPU variables so we can
 * avoid later confusion if an invalid CPU is requested.
 */
int
db_var_db_cpu(struct db_variable *vp, db_expr_t *valuep, int op)
{

	switch (op) {
	case DB_VAR_GET:
		*valuep = db_cpu;
		return (1);

	case DB_VAR_SET:
		if (*(int *)valuep < -1 && *(int *)valuep > mp_maxid) {
			db_printf("Invalid value: %d", *(int*)valuep);
			return (0);
		}
		db_cpu = *(int *)valuep;
		return (1);

	default:
		db_printf("db_var_db_cpu: unknown operation\n");
		return (0);
	}
}

/*
 * Read-only variable reporting the current CPU, which is what we use when
 * db_cpu is set to -1.
 */
int
db_var_curcpu(struct db_variable *vp, db_expr_t *valuep, int op)
{

	switch (op) {
	case DB_VAR_GET:
		*valuep = curcpu;
		return (1);

	case DB_VAR_SET:
		db_printf("Read-only variable.\n");
		return (0);

	default:
		db_printf("db_var_curcpu: unknown operation\n");
		return (0);
	}
}

#ifdef VIMAGE
/*
 * Validate the virtual subsystem pointer used to interpret per-subsystem
 * global variable expansion.
 */
int
db_var_db_vimage(struct db_variable *vp, db_expr_t *valuep, int op)
{
	struct db_vimage_variable *vvp;
	struct vimage *v;
	void *p;

	vvp = (struct db_vimage_variable *)vp;
	switch (op) {
	case DB_VAR_GET:
		*valuep = (db_expr_t)vvp->vse->v_db_instance;
		return (1);

	case DB_VAR_SET:
		p = *(void **)valuep;
		/* Permit "unset. */
		if (p == NULL) {
			vvp->vse->v_db_instance = p;
			return (1);
		}
		LIST_FOREACH(v, &vvp->vse->v_instance_head, v_le) {
			if (p == v) {
				vvp->vse->v_db_instance = p;
				return (1);
			}
		}
		db_printf("db_var_db_vimage: value no valid %s "
		    "instance\n", vvp->vse->name);
		return (0);

	default:
		db_printf("db_var_db_vimage: unknown operation\n");
		return (0);
	}
}

/*
 * Read-only variable reporting the current virtual instance, which is what
 * we use when virtual subsystem v_db_instance member is set to NULL.
 */
int
db_var_curvimage(struct db_variable *vp, db_expr_t *valuep, int op)
{
	struct db_vimage_variable *vvp;

	vvp = (struct db_vimage_variable *)vp;
	switch (op) {
	case DB_VAR_GET:
		*valuep = (db_expr_t)*((void **)((uintptr_t)curthread +
		    vvp->vse->v_curvar));
		return (1);

	case DB_VAR_SET:
		db_printf("Read-only variable.\n");
		return (0);

	default:
		db_printf("db_var_curvimage: unknown operation\n");
		return (0);
	}
}
#endif

/*
 * Add symbol table, with given name, to list of symbol tables.
 */
void
db_add_symbol_table(start, end, name, ref)
	char *start;
	char *end;
	char *name;
	char *ref;
{
	if (db_nsymtab >= MAXNOSYMTABS) {
		printf ("No slots left for %s symbol table", name);
		panic ("db_sym.c: db_add_symbol_table");
	}

	db_symtabs[db_nsymtab].start = start;
	db_symtabs[db_nsymtab].end = end;
	db_symtabs[db_nsymtab].name = name;
	db_symtabs[db_nsymtab].private = ref;
	db_nsymtab++;
}

/*
 *  db_qualify("vm_map", "ux") returns "unix:vm_map".
 *
 *  Note: return value points to static data whose content is
 *  overwritten by each call... but in practice this seems okay.
 */
static char *
db_qualify(sym, symtabname)
	c_db_sym_t	sym;
	register char	*symtabname;
{
	const char	*symname;
	static char     tmp[256];

	db_symbol_values(sym, &symname, 0);
	snprintf(tmp, sizeof(tmp), "%s:%s", symtabname, symname);
	return tmp;
}


boolean_t
db_eqname(src, dst, c)
	const char *src;
	const char *dst;
	int c;
{
	if (!strcmp(src, dst))
	    return (TRUE);
	if (src[0] == c)
	    return (!strcmp(src+1,dst));
	return (FALSE);
}

boolean_t
db_value_of_name(name, valuep)
	const char	*name;
	db_expr_t	*valuep;
{
	c_db_sym_t	sym;

	sym = db_lookup(name);
	if (sym == C_DB_SYM_NULL)
	    return (FALSE);
	db_symbol_values(sym, &name, valuep);
	return (TRUE);
}

boolean_t
db_value_of_name_pcpu(name, valuep)
	const char	*name;
	db_expr_t	*valuep;
{
	static char     tmp[256];
	db_expr_t	value;
	c_db_sym_t	sym;
	int		cpu;

	if (db_cpu != -1)
		cpu = db_cpu;
	else
		cpu = curcpu;
	snprintf(tmp, sizeof(tmp), "pcpu_entry_%s", name);
	sym = db_lookup(tmp);
	if (sym == C_DB_SYM_NULL)
		return (FALSE);
	db_symbol_values(sym, &name, &value);
	if (value < DPCPU_START || value >= DPCPU_STOP)
		return (FALSE);
	*valuep = (db_expr_t)((uintptr_t)value + dpcpu_off[cpu]);
	return (TRUE);
}

#ifdef VIMAGE
extern LIST_HEAD(vimage_subsys_list_head, vimage_subsys) vimage_subsys_head;
#endif
boolean_t
db_value_of_name_vimage(name, valuep)
	const char	*name;
	db_expr_t	*valuep;
{
#ifdef VIMAGE
	static char     tmp[256];
	db_expr_t	value;
	c_db_sym_t	sym;
	struct vimage_subsys *vse;
	struct vimage	*v;

	LIST_FOREACH(vse, &vimage_subsys_head, vimage_subsys_le) {
		snprintf(tmp, sizeof(tmp), "%s%s", vse->v_symprefix, name);
		sym = db_lookup(tmp);
		if (sym == C_DB_SYM_NULL)
			continue;
		db_symbol_values(sym, &name, &value);

		/*
		 * In case we found the symbol but it's not part of this
		 * subsystem, do not try any others. The symprefix must be
		 * unique.  Include the extra space in checking as it could
		 * be used by the dynamic data allocator for modules.
		 */
		if (value < vse->v_start ||
		    value >= (vse->v_start + vse->v_size))
			return (FALSE);

		if (vse->v_db_instance != NULL)
			v = (struct vimage *)vse->v_db_instance;
		else
			v = *((struct vimage **)((uintptr_t)curthread +
			    vse->v_curvar));
		/* In case neither is set we cannot translate it. */
		if (v == NULL)
			return (FALSE);
		*valuep = (db_expr_t)((uintptr_t)value + v->v_data_base);
		return (TRUE);
	}
#endif
	return (FALSE);
}

#ifdef VIMAGE
#include <sys/elf_common.h>
#include <sys/linker.h>

static int
db_lookup_vimage_set_variables_callback(linker_file_t lf, int i,
    linker_symvaltype_t *symval, void *p)
{
	struct vimage_subsys *vse;
	db_expr_t	value;
	uintptr_t	current;
	struct vimage	*v;

	if (db_pager_quit)
		return (EINTR);

	vse = (struct vimage_subsys *)p;
	value = (db_expr_t)symval->value;

	/* Return if it does not belong to out linker set. */
	if (value < vse->v_start ||
	    value >= (vse->v_start + vse->v_size))
		return (0);
	/* Return unless it is an STT_OBJECT. */
	if (symval->type != STT_OBJECT)
		return (0);
	/* Return unless name has our symprefix. */
	if (symval->name == NULL || strncmp(symval->name, vse->v_symprefix,
	    strlen(vse->v_symprefix)) != 0)
		return (0);

	/* Translate the current value. */
	if (vse->v_db_instance != NULL)
		v = (struct vimage *)vse->v_db_instance;
	else
		v = *((struct vimage **)((uintptr_t)curthread +
		    vse->v_curvar));
	if (v == NULL) {
		db_printf("Cannot figure out current instance to debug.");
		db_printf("Please set $db_%s to something meaningful.",
		    vse->name);
		return (ENOENT);
	}
	current = (uintptr_t)value + v->v_data_base;

	db_printf("%-40s %-6zu %p %p\n",
	    (symval->name + strlen(vse->v_symprefix)), symval->size,
	    (void *)value, (void *)current);

	return (0);
}

int
db_lookup_vimage_set_variables(struct vimage_subsys *vse)
{
	int error = 0;

	db_printf("Subsystem '%s' symbols, start=%p stop=%p\n", vse->name,
	    (void *)vse->v_start, (void *)vse->v_stop);
	db_printf("%-40s %-6s %-18s %-18s\n", "Name", "Size", "Default at",
	    "Current at");

	error = linker_ddb_list_symbols(db_lookup_vimage_set_variables_callback,
	    (void *)vse);
	return (error);
}
#endif

/*
 * Lookup a symbol.
 * If the symbol has a qualifier (e.g., ux:vm_map),
 * then only the specified symbol table will be searched;
 * otherwise, all symbol tables will be searched.
 */
static c_db_sym_t
db_lookup(symstr)
	const char *symstr;
{
	c_db_sym_t sp;
	register int i;
	int symtab_start = 0;
	int symtab_end = db_nsymtab;
	register const char *cp;

	/*
	 * Look for, remove, and remember any symbol table specifier.
	 */
	for (cp = symstr; *cp; cp++) {
		if (*cp == ':') {
			for (i = 0; i < db_nsymtab; i++) {
				int n = strlen(db_symtabs[i].name);

				if (
				    n == (cp - symstr) &&
				    strncmp(symstr, db_symtabs[i].name, n) == 0
				) {
					symtab_start = i;
					symtab_end = i + 1;
					break;
				}
			}
			if (i == db_nsymtab) {
				db_error("invalid symbol table name");
			}
			symstr = cp+1;
		}
	}

	/*
	 * Look in the specified set of symbol tables.
	 * Return on first match.
	 */
	for (i = symtab_start; i < symtab_end; i++) {
		sp = X_db_lookup(&db_symtabs[i], symstr);
		if (sp) {
			db_last_symtab = &db_symtabs[i];
			return sp;
		}
	}
	return 0;
}

/*
 * If TRUE, check across symbol tables for multiple occurrences
 * of a name.  Might slow things down quite a bit.
 */
static volatile boolean_t db_qualify_ambiguous_names = FALSE;

/*
 * Does this symbol name appear in more than one symbol table?
 * Used by db_symbol_values to decide whether to qualify a symbol.
 */
static boolean_t
db_symbol_is_ambiguous(sym)
	c_db_sym_t	sym;
{
	const char	*sym_name;
	register int	i;
	register
	boolean_t	found_once = FALSE;

	if (!db_qualify_ambiguous_names)
		return FALSE;

	db_symbol_values(sym, &sym_name, 0);
	for (i = 0; i < db_nsymtab; i++) {
		if (X_db_lookup(&db_symtabs[i], sym_name)) {
			if (found_once)
				return TRUE;
			found_once = TRUE;
		}
	}
	return FALSE;
}

/*
 * Find the closest symbol to val, and return its name
 * and the difference between val and the symbol found.
 */
c_db_sym_t
db_search_symbol( val, strategy, offp)
	register db_addr_t	val;
	db_strategy_t		strategy;
	db_expr_t		*offp;
{
	register
	unsigned int	diff;
	size_t		newdiff;
	register int	i;
	c_db_sym_t	ret = C_DB_SYM_NULL, sym;

	newdiff = diff = ~0;
	for (i = 0; i < db_nsymtab; i++) {
	    sym = X_db_search_symbol(&db_symtabs[i], val, strategy, &newdiff);
	    if (newdiff < diff) {
		db_last_symtab = &db_symtabs[i];
		diff = newdiff;
		ret = sym;
	    }
	}
	*offp = diff;
	return ret;
}

/*
 * Return name and value of a symbol
 */
void
db_symbol_values(sym, namep, valuep)
	c_db_sym_t	sym;
	const char	**namep;
	db_expr_t	*valuep;
{
	db_expr_t	value;

	if (sym == DB_SYM_NULL) {
		*namep = 0;
		return;
	}

	X_db_symbol_values(db_last_symtab, sym, namep, &value);

	if (db_symbol_is_ambiguous(sym))
		*namep = db_qualify(sym, db_last_symtab->name);
	if (valuep)
		*valuep = value;
}


/*
 * Print a the closest symbol to value
 *
 * After matching the symbol according to the given strategy
 * we print it in the name+offset format, provided the symbol's
 * value is close enough (eg smaller than db_maxoff).
 * We also attempt to print [filename:linenum] when applicable
 * (eg for procedure names).
 *
 * If we could not find a reasonable name+offset representation,
 * then we just print the value in hex.  Small values might get
 * bogus symbol associations, e.g. 3 might get some absolute
 * value like _INCLUDE_VERSION or something, therefore we do
 * not accept symbols whose value is "small" (and use plain hex).
 */

db_expr_t	db_maxoff = 0x10000;

void
db_printsym(off, strategy)
	db_expr_t	off;
	db_strategy_t	strategy;
{
	db_expr_t	d;
	char 		*filename;
	const char	*name;
	db_expr_t	value;
	int 		linenum;
	c_db_sym_t	cursym;

	cursym = db_search_symbol(off, strategy, &d);
	db_symbol_values(cursym, &name, &value);
	if (name == 0)
		value = off;
	if (value >= DB_SMALL_VALUE_MIN && value <= DB_SMALL_VALUE_MAX) {
		db_printf("%+#lr", (long)off);
		return;
	}
	if (name == 0 || d >= (unsigned long)db_maxoff) {
		db_printf("%#lr", (unsigned long)off);
		return;
	}
#ifdef DDB_NUMSYM
	db_printf("%#lr = %s", (unsigned long)off, name);
#else
	db_printf("%s", name);
#endif
	if (d)
		db_printf("+%+#lr", (long)d);
	if (strategy == DB_STGY_PROC) {
		if (db_line_at_pc(cursym, &filename, &linenum, off))
			db_printf(" [%s:%d]", filename, linenum);
	}
}

static boolean_t
db_line_at_pc( sym, filename, linenum, pc)
	c_db_sym_t	sym;
	char		**filename;
	int		*linenum;
	db_expr_t	pc;
{
	return X_db_line_at_pc( db_last_symtab, sym, filename, linenum, pc);
}

int
db_sym_numargs(sym, nargp, argnames)
	c_db_sym_t	sym;
	int		*nargp;
	char		**argnames;
{
	return X_db_sym_numargs(db_last_symtab, sym, nargp, argnames);
}
