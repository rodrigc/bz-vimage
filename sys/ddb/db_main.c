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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/ddb/db_main.c,v 1.8 2008/09/25 19:50:14 jhb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cons.h>
#include <sys/linker.h>
#include <sys/kdb.h>
#include <sys/kernel.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/sysctl.h>

#include <machine/kdb.h>
#include <machine/pcb.h>
#include <machine/setjmp.h>

#include <ddb/ddb.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>

SYSCTL_NODE(_debug, OID_AUTO, ddb, CTLFLAG_RW, 0, "DDB settings");

static dbbe_init_f db_init;
static dbbe_trap_f db_trap;
static dbbe_trace_f db_trace_self_wrapper;

KDB_BACKEND(ddb, db_init, db_trace_self_wrapper, db_trap);

vm_offset_t ksym_start, ksym_end;

boolean_t
X_db_line_at_pc(db_symtab_t *symtab, c_db_sym_t sym, char **file, int *line,
    db_expr_t off)
{
	return (FALSE);
}

c_db_sym_t
X_db_lookup(db_symtab_t *symtab, const char *symbol)
{
	c_linker_sym_t lsym;
	Elf_Sym *sym;

	if (symtab->private == NULL) {
		return ((c_db_sym_t)((!linker_ddb_lookup(symbol, &lsym))
			? lsym : NULL));
	} else {
		sym = (Elf_Sym *)symtab->start;
		while ((char *)sym < symtab->end) {
			if (sym->st_name != 0 &&
			    !strcmp(symtab->private + sym->st_name, symbol))
				return ((c_db_sym_t)sym);
			sym++;
		}
	}
	return (NULL);
}

c_db_sym_t
X_db_search_symbol(db_symtab_t *symtab, db_addr_t off, db_strategy_t strat,
    db_expr_t *diffp)
{
	c_linker_sym_t lsym;
	Elf_Sym *sym, *match;
	unsigned long diff;

	if (symtab->private == NULL) {
		if (!linker_ddb_search_symbol((caddr_t)off, &lsym, &diff)) {
			*diffp = (db_expr_t)diff;
			return ((c_db_sym_t)lsym);
		}
		return (NULL);
	}

	diff = ~0UL;
	match = NULL;
	for (sym = (Elf_Sym*)symtab->start; (char*)sym < symtab->end; sym++) {
		if (sym->st_name == 0)
			continue;
		if (off < sym->st_value)
			continue;
		if (ELF_ST_TYPE(sym->st_info) != STT_OBJECT &&
		    ELF_ST_TYPE(sym->st_info) != STT_FUNC &&
		    ELF_ST_TYPE(sym->st_info) != STT_NOTYPE)
			continue;
		if ((off - sym->st_value) > diff)
			continue;
		if ((off - sym->st_value) < diff) {
			diff = off - sym->st_value;
			match = sym;
		} else {
			if (match == NULL)
				match = sym;
			else if (ELF_ST_BIND(match->st_info) == STB_LOCAL &&
			    ELF_ST_BIND(sym->st_info) != STB_LOCAL)
				match = sym;
		}
		if (diff == 0) {
			if (strat == DB_STGY_PROC &&
			    ELF_ST_TYPE(sym->st_info) == STT_FUNC &&
			    ELF_ST_BIND(sym->st_info) != STB_LOCAL)
				break;
			if (strat == DB_STGY_ANY &&
			    ELF_ST_BIND(sym->st_info) != STB_LOCAL)
				break;
		}
	}

	*diffp = (match == NULL) ? off : diff;
	return ((c_db_sym_t)match);
}

boolean_t
X_db_sym_numargs(db_symtab_t *symtab, c_db_sym_t sym, int *nargp,
    char **argp)
{
	return (FALSE);
}

void
X_db_symbol_values(db_symtab_t *symtab, c_db_sym_t sym, const char **namep,
    db_expr_t *valp)
{
	linker_symval_t lval;

	if (symtab->private == NULL) {
		linker_ddb_symbol_values((c_linker_sym_t)sym, &lval);
		if (namep != NULL)
			*namep = (const char*)lval.name;
		if (valp != NULL)
			*valp = (db_expr_t)lval.value;
	} else {
		if (namep != NULL)
			*namep = (const char *)symtab->private +
			    ((const Elf_Sym *)sym)->st_name;
		if (valp != NULL)
			*valp = (db_expr_t)((const Elf_Sym *)sym)->st_value;
	}
}

static int
db_init(void)
{
	uintptr_t symtab, strtab;
	Elf_Size tabsz, strsz;

	db_command_init();
	if (ksym_end > ksym_start && ksym_start != 0) {
		symtab = ksym_start;
		tabsz = *((Elf_Size*)symtab);
		symtab += sizeof(Elf_Size);
		strtab = symtab + tabsz;
		strsz = *((Elf_Size*)strtab);
		strtab += sizeof(Elf_Size);
		if (strtab + strsz <= ksym_end) {
			db_add_symbol_table((char *)symtab,
			    (char *)(symtab + tabsz), "elf", (char *)strtab);
		}
	}
	db_add_symbol_table(NULL, NULL, "kld", NULL);
	return (1);	/* We're the default debugger. */
}

static int
db_trap(int type, int code)
{
	jmp_buf jb;
	void *prev_jb;
	boolean_t bkpt, watchpt;
	const char *why;

	/*
	 * Don't handle the trap if the console is unavailable (i.e. it
	 * is in graphics mode).
	 */
	if (cnunavailable())
		return (0);

	bkpt = IS_BREAKPOINT_TRAP(type, code);
	watchpt = IS_WATCHPOINT_TRAP(type, code);

	if (db_stop_at_pc(&bkpt)) {
		if (db_inst_count) {
			db_printf("After %d instructions (%d loads, %d stores),\n",
			    db_inst_count, db_load_count, db_store_count);
		}
		prev_jb = kdb_jmpbuf(jb);
		if (setjmp(jb) == 0) {
			db_dot = PC_REGS();
			db_print_thread();
			if (bkpt)
				db_printf("Breakpoint at\t");
			else if (watchpt)
				db_printf("Watchpoint at\t");
			else
				db_printf("Stopped at\t");
			db_print_loc_and_inst(db_dot);
		}
		why = kdb_why;
		db_script_kdbenter(why != KDB_WHY_UNSET ? why : "unknown");
		db_command_loop();
		(void)kdb_jmpbuf(prev_jb);
	}

	db_restart_at_pc(watchpt);

	return (1);
}

static void
db_trace_self_wrapper(void)
{
	jmp_buf jb;
	void *prev_jb;

	prev_jb = kdb_jmpbuf(jb);
	if (setjmp(jb) == 0)
		db_trace_self();
	(void)kdb_jmpbuf(prev_jb);
}