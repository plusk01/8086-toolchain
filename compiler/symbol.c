
/*
 * C compiler
 * ==========
 *
 * Copyright 1989, 1990, 1991 Christoph van Wuellen.
 * Credits to Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 *
 * History:
 *
 * 1989   starting an 68000 C compiler, starting with material
 *        originally by M. Brandt
 * 1990   68000 C compiler further bug fixes
 *        started i386 port (December)
 * 1991   i386 port finished (January)
 *        further corrections in the front end and in the 68000
 *        code generator.
 *        The next port will be a SPARC port
 */

/*****************************************************************************/

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define SMAXKEY	((unsigned)631)

/********************************************************** Type Definitions */

typedef unsigned int KEY;	/* type for hash key */

/********************************************************** Static Variables */

static LEVEL scope_level = (LEVEL) 0;
static struct scopetbl scope;
static SYM *symbols[SMAXKEY];
static SYM *tags[SMAXKEY];

/*********************************************** Static Function Definitions */

static SYM *copy_sym P_ ((const SYM *));
static SYM *findsym P_ ((const CHAR *, SYM *const *));
static SYM *get_label P_ ((const CHAR *));
static SYM *global_search P_ ((SYM *));
static KEY skey P_ ((register const CHAR *));
static void addsym P_ ((SYM *, TABLE *));
static void append P_ ((SYM **, TABLE *, SYM **));
static void check_same P_ ((const SYM *, const SYM *));
static void deletesym P_ ((const SYM *, SYM **));
static void insertsym P_ ((SYM *, SYM **));

/*****************************************************************************/

static void check_same P2 (const SYM *, sp1, const SYM *, sp2)
{
    if ((is_member (sp1) && !is_member (sp2)) ||
	(!is_member (sp1) && is_member (sp2))) {
	return;
    }
#ifndef SYNTAX_CORRECT
    if (!is_equal_type (typeof (sp1), typeof (sp2))) {
	message (ERR_REDECL, nameof (sp1));
    }
#endif /* SYNTAX_CORRECT */
}

SYM    *mk_sym P3 (const CHAR *, name, STORAGE, storage, TYP *, tp)
{
    SYM    *sp;

    sp = (SYM *) xalloc (sizeof (SYM));

    set_nextsym (sp, NIL_SYM);
    set_name (sp, name);
    set_type (sp, tp);
    sp->value.i = 0L;
    set_storage (sp, storage);
    sp->status = (STATUS) 0;
    sp->function_specifier = fs_none;
#ifdef SEQUENCE
    sp->sequence = (SEQNUM) 0;
#endif
    return sp;
}

static SYM *copy_sym P1 (const SYM *, sp)
{
    SYM    *sp2;

    sp2 = (SYM *) xalloc (sizeof (SYM));

    *sp2 = *sp;
    sp2->hnext = NIL_SYM;
    set_nextsym (sp2, NIL_SYM);
    return sp2;
}

static void addsym P2 (SYM *, sp, TABLE *, table)
{
    assert (sp != NIL_SYM);
    if (table->head == NIL_SYM) {
	/* The table is empty so far... */
	table->head = sp;
    } else {
	set_nextsym (table->tail, sp);
    }
    table->tail = sp;
    set_nextsym (sp, NIL_SYM);
}

/* calculate the key for the hash function */
static KEY skey P1 (register const CHAR *, p)
{
    register KEY sum;

    for (sum = (KEY) 0; *p;) {
	sum += (sum << 5) + (KEY) * p++;
    }
    return (sum % SMAXKEY);
}

/* insert a symbol into the specified hash table */
static void insertsym P2 (SYM *, sp, SYM **, hashtab)
{
    KEY     keyno = skey (nameof (sp));

    sp->hnext = hashtab[keyno];
    set_level (sp, scope_level);
    hashtab[keyno] = sp;
}

/* insert a symbol into the specified hash table, at the reverse end */
static void rinsertsym P2 (SYM *, sp, SYM **, hashtab)
{
    KEY     keyno = skey (nameof (sp));
    SYM   **spp = &hashtab[keyno];

    if (*spp) {
	while ((*spp)->hnext) {
	    spp = &(*spp)->hnext;
	}
	(*spp)->hnext = sp;
    } else {
	hashtab[keyno] = sp;
    }
}

/* find a symbol in the specified hash table */
static SYM *findsym P2 (const CHAR *, p, SYM *const *, hashtab)
{
    register SYM *sp;

    for (sp = hashtab[skey (p)]; sp; sp = sp->hnext) {
	if (p == nameof (sp)) {
	    break;
	}
    }
    return sp;
}

/* delete a symbol from the specified hash table */
static void deletesym P2 (const SYM *, sp, SYM **, hashtab)
{
    SYM   **hsp;

    for (hsp = &hashtab[skey (nameof (sp))]; *hsp; hsp = &(*hsp)->hnext) {
	if (*hsp == sp) {
	    *hsp = (*hsp)->hnext;
	    return;
	}
    }
}

/* returns true is the symbol is in the current scope, otherwise return false */
BOOL is_local_scope P1 (const SYM *, sp)
{
    return (levelof (sp) >= scope_level);
}

/* searches the current scope/table for the specified symbol */
SYM    *search P2 (const CHAR *, na, const TABLE *, table)
{
    register SYM *sp;

    for (sp = table->head; sp != NIL_SYM; sp = nextsym (sp)) {
	if (nameof (sp) == na) {
	    break;
	}
    }
    return sp;
}

/* looks to see if a symbol of this name can be found in the global scope */
static SYM *global_search P1 (SYM *, sp)
{
    SYM    *sp1;

    for (sp1 = sp; sp1 != NIL_SYM; sp1 = sp1->hnext) {
	if ((nameof (sp1) == nameof (sp)) && is_global_scope (sp1)) {
	    break;
	}
    }
    return sp1;
}

static void append P3 (SYM **, ptr_sp, TABLE *, scope_tab, SYM **, hashtab)
{
    SYM    *sp1 = NIL_SYM;
    SYM    *sp = *ptr_sp;

    if ((nameof (sp)[0] != (CHAR) 0) &&
	(sp1 = findsym (nameof (sp), hashtab)) != NIL_SYM) {
	if (is_extern (sp) &&
	    !is_local_scope (sp1) && !is_symbol_outscope (sp1)) {
	    /*ANSI 3.1.2.2
	     * If the declarator of an identifier for an object or a function
	     * contains the storage-class specifier extern, the identifier
	     * has the same linkage as any visible declaration of the
	     * identifier with file scope.  If there is no visible declaration
	     * with file scope, the identifier has external linkage.
	     */
	    sp1 = global_search (sp1);
	    if (sp1 != NIL_SYM) {
		check_same (sp, sp1);
		set_type (sp, typeof (sp1));
		set_storage (sp, storageof (sp1));
		sp->value = sp1->value;
		sp->status |= sp1->status;
		if (is_static (sp1)) {
		    symbol_output (sp);
		}
	    }
	}
    }
    if (sp1 != NIL_SYM) {
	if (is_local_scope (sp1)) {
	    if (scope_level == GLOBAL_SCOPE) {
		sp1->status &= (STATUS) ~((unsigned) SYM_OUTSCOPE);
	    }
	    check_same (sp, sp1);
	    set_type (sp1, composite_type (typeof (sp1), typeof (sp)));
	    if (is_func (typeof (sp))) {
		sp1->status |= sp->status;
		if (is_global (sp)) {
		    typeof (sp1)->state &= (STATE) (~(unsigned) STATE_ANSI);
		    if (is_ansi (typeof (sp))) {
			set_ansi (typeof (sp1));
		    }
		}
	    }
	    /*
	     * The new storage class depends on the old and on the new one.
	     */
	    if (storageof (sp) == storageof (sp1)) {
#ifndef SYNTAX_CORRECT
		switch (storageof (sp)) {
		case sc_const:
		case sc_auto:
		    message (ERR_REDECL, nameof (sp));
		    break;
		case sc_typedef:
		    if (is_equal_type (typeof (sp), typeof (sp1))) {
			message (WARN_REDECL, nameof (sp));
		    } else {
			message (ERR_REDECL, nameof (sp));
		    }
		    break;
#ifdef FACIST
		case sc_external:
		    message (WARN_DUPDECL, nameof (sp));
		    break;
#endif /* FACIST */
		default:
		    break;
		}
#endif /* SYNTAX_CORRECT */
		if (!is_symbol_outscope (sp1)) {
		    *ptr_sp = sp1;	/* caller uses old entry */
		    return;
		}
	    }
	    switch (storageof (sp)) {
	    case sc_const:
	    case sc_typedef:
#ifndef SYNTAX_CORRECT
		message (ERR_REDECL, nameof (sp));
#endif /* SYNTAX_CORRECT */
		*ptr_sp = sp1;	/* caller uses old entry */
		return;

	    case sc_external:
		if (scope_level == GLOBAL_SCOPE) {
		    sp1->status &= (STATUS) ~((unsigned) SYM_OUTSCOPE);
		}
		if (!is_symbol_outscope (sp1)) {
		    *ptr_sp = sp1;	/* caller uses old entry */
		    return;
		}
		break;

	    case sc_global:
#ifndef SYNTAX_CORRECT
		switch (storageof (sp1)) {
		case sc_typedef:
		case sc_const:
		    message (ERR_REDECL, nameof (sp));
		    return;
		case sc_static:
		    if ((lang_option == LANG_KANDR) || !is_func (typeof (sp))) {
			message (ERR_REDECL, nameof (sp));
		    } else if (is_func (typeof (sp))) {
			message (WARN_GLOBAL, nameof (sp));
		    }
		    break;
		default:
		    break;
		}
#endif /* SYNTAX_CORRECT */
		set_storage (sp1, sc_global);
		*ptr_sp = sp1;	/* caller uses old entry */
		return;

	    case sc_static:
#ifndef SYNTAX_CORRECT
		switch (storageof (sp1)) {
		case sc_typedef:
		case sc_const:
		case sc_global:
		    message (ERR_REDECL, nameof (sp));
		    return;
		case sc_external:
		    if (is_object_type (typeof (sp))) {
			message (ERR_LINKAGE, nameof (sp));
		    } else {
			message (WARN_EXTERN, nameof (sp));
		    }
		    break;
		default:
		    break;
		}
#endif /* SYNTAX_CORRECT */
		set_storage (sp1, sc_static);
		*ptr_sp = sp1;	/* caller uses old entry */
		return;

	    default:
#ifndef SYNTAX_CORRECT
		if (is_typedef (sp1)) {
		    message (ERR_REDECL, nameof (sp));
		}
#endif /* SYNTAX_CORRECT */
		*ptr_sp = sp1;	/* caller uses old entry */
		return;
	    }
#ifndef SYNTAX_CORRECT
	} else {
	    if (!is_member (sp)) {
		switch (storageof (sp1)) {
		case sc_external:
		case sc_static:
		    if (!is_extern (sp) && !is_static (sp)) {
			message (WARN_HIDE, nameof (sp));
		    }
		    break;
		case sc_global:
		    if (!is_global (sp)) {
			if (!is_function_type (typeof (sp))) {
			    message (WARN_HIDE, nameof (sp));
			}
		    }
		    break;
		case sc_auto:
		    message (WARN_HIDE, nameof (sp));
		    break;
		default:
		    break;
		}
	    }
#endif /* SYNTAX_CORRECT */
	}
#ifdef FACIST
#ifndef SYNTAX_CORRECT
    } else {
	switch (storageof (sp)) {
	case sc_global:
	    message (WARN_PREVDECL, nameof (sp));
	    break;
	default:
	    break;
	}
#endif /* SYNTAX_CORRECT */
#endif /* FACIST */
    }
    insertsym (sp, hashtab);
    addsym (sp, scope_tab);
}

void sym_append P1 (SYM **, ptr_sp)
{
    append (ptr_sp, &scope.local->symbols, symbols);
}

void tag_append P1 (SYM **, ptr_sp)
{
    append (ptr_sp, &scope.local->tags, tags);
}

void field_append P1 (SYM **, ptr_sp)
{
#ifndef SYNTAX_CORRECT
    SYM    *sp = sym_search (nameof (*ptr_sp));

    if ((sp != NIL_SYM) && is_local_scope (sp) && is_member (sp)) {
	message (ERR_REDECL, nameof (*ptr_sp));
	return;
    }
#endif /* SYNTAX_CORRECT */
    sym_append (ptr_sp);
}

SYM    *tag_search P1 (const CHAR *, na)
{
    return findsym (na, tags);
}

SYM    *sym_search P1 (const CHAR *, na)
{
    return findsym (na, symbols);
}

/*---------------------------------------------------------------------------*/

static TABLE labsyms;

static SYM *get_label P1 (const CHAR *, na)
{
    SYM    *sp = search (na, &labsyms);

    if (sp == NIL_SYM) {
	sp = mk_sym (na, sc_label, NIL_TYP);
	sp->value.l = nextlabel++;
	addsym (sp, &labsyms);
    }
    return sp;
}

LABEL lab_search P1 (const CHAR *, na)
{
    SYM    *sp = get_label (na);

    symbol_used (sp);
    return sp->value.l;
}

LABEL lab_define P1 (const CHAR *, na)
{
    SYM    *sp = get_label (na);

#ifndef SYNTAX_CORRECT
    if (is_symbol_defined (sp)) {
	message (ERR_DUPLABEL, na);
    }
#endif /* SYNTAX_CORRECT */
    symbol_defined (sp);
    return sp->value.l;
}

void check_labels P0 (void)
{
#ifndef SYNTAX_CORRECT
    const SYM *sp;

    for (sp = labsyms.head; sp != NIL_SYM; sp = nextsym (sp)) {
	if (!is_symbol_defined (sp)) {
	    message (ERR_UNDEFLAB, nameof (sp));
	} else if (!is_symbol_used (sp)) {
	    message (WARN_LABNOTUSED, nameof (sp));
	}
    }
#endif /* SYNTAX_CORRECT */
    labsyms.head = labsyms.tail = NIL_SYM;
}

/*---------------------------------------------------------------------------*/

BLOCK  *mk_block P0 (void)
{
    BLOCK  *block;
    block = (BLOCK *) xalloc (sizeof (BLOCK));

    *block = init_block;
    return block;
}

void beginblock P0 (void)
{
    BLOCK  *sb;

    sb = mk_block ();
    sb->prev = scope.local;
    sb->offset = lc_auto;
    scope.local = sb;
    if (scope.global == NIL_BLOCK) {
	scope.global = sb;
    }
    scope_level++;
}

void endblock P0 (void)
{
    SYM    *sp;
    BLOCK  *block = scope.local;

    if (lc_auto > lc_auto_max) {
	lc_auto_max = lc_auto;
    }
#if 0
    /*
     * This will allow nested compound statements to start their definitions
     * at the same offset.  However there is currently a problem with this in
     * the global optimiser when an integer is at the same offset as a float -
     * the float is incorrectly optimised into a register!  So until a solution
     * is found just let the offsets be additive.
     */
    lc_auto = block->offset;
#endif

    for (sp = symbolsof (block); sp != NIL_SYM; sp = nextsym (sp)) {
	TYP    *tp = typeof (sp);

	switch (storageof (sp)) {
	case sc_global:
	    if (scope_level == GLOBAL_SCOPE) {
		if (!(sp->status & (SYM_DEFINED | SYM_OUTPUT))) {
		    if (is_array_type (tp) && is_unknown_size (tp)) {
			tp->size = referenced_type (tp)->size;
		    }
#ifndef SYNTAX_CORRECT
		    check_complete (tp);
#endif /* SYNTAX_CORRECT */
#ifdef CPU_DEFINED
		    put_storage (sp);	/* tentative definition */
		    put_reference (sp);
#endif /* CPU_DEFINED */
		}
	    }
	    break;
	case sc_external:
	    if (scope_level == GLOBAL_SCOPE) {
#ifdef CPU_DEFINED
		if (sp->status & (SYM_USED | SYM_DEFINED)) {
		    put_reference (sp);
		}
#endif /* CPU_DEFINED */
	    } else {
		SYM    *sp1;

		sp1 = global_search (sp);
		if (sp1) {
		    sp1->status |= (STATUS) (sp->status & (STATUS)
					     ~((unsigned) SYM_OUTPUT));
		} else {
		    /* add this entry to the global table */
		    global_flag++;
		    sp1 = copy_sym (sp);
		    set_type (sp1, copy_type (tp));
		    sp1->status =
			(STATUS) ((sp1->status | SYM_OUTSCOPE) & (STATUS)
				  ~((unsigned) SYM_OUTPUT));
		    set_level (sp1, GLOBAL_SCOPE);

		    addsym (sp1, &scope.global->symbols);

		    rinsertsym (sp1, symbols);
		    global_flag--;
		}
	    }
	    break;
	case sc_static:
	    if (!is_symbol_defined (sp)) {
		if (is_object_type (tp)) {
		    if (is_array_type (tp) && is_incomplete_type (tp)) {
			tp->size = referenced_type (tp)->size;
		    }
#ifndef SYNTAX_CORRECT
		    check_complete (tp);
#endif /* SYNTAX_CORRECT */
#ifdef CPU_DEFINED
		    if (!is_symbol_output (sp)) {
			put_storage (sp);	/* tentative definition */
			symbol_output (sp);
		    }
#endif /* CPU_DEFINED */
#ifndef SYNTAX_CORRECT
		} else {
		    if (scope_level == GLOBAL_SCOPE) {
			if (is_symbol_used (sp)) {
			    message (ERR_STATIC, nameof (sp));
			} else {
			    message (WARN_STATIC, nameof (sp));
			}
		    }
#endif /* SYNTAX_CORRECT */
		}
	    }
	    /*lint -fallthrough */
	case sc_auto:
	case sc_register:
	case sc_parms:
#ifndef SYNTAX_CORRECT
	    if (tp) {
		switch (tp->type) {
		case bt_void:	/* func(void) definitions */
		case bt_ellipsis:	/* func(...) definitions */
		    break;
		default:
		    if (!is_symbol_used (sp)) {
			message (WARN_NOTUSED, nameof (sp));
		    }
		}
	    }
#endif /* SYNTAX_CORRECT */
	    break;
	case sc_typedef:
	case sc_tag:
	case sc_label:
	case sc_member:
	case sc_const:
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
    }
#ifdef LIST
    summary (block, scope_level);
#endif /* LIST */
    VOIDCAST endparamblock ();
}

void beginparamblock P0 (void)
{
    beginblock ();
}

BLOCK  *endparamblock P0 (void)
{
    SYM    *sp;
    BLOCK  *block = scope.local;;
    for (sp = symbolsof (block); sp != NIL_SYM; sp = nextsym (sp)) {
	deletesym (sp, symbols);
    }
    for (sp = block->tags.head; sp != NIL_SYM; sp = nextsym (sp)) {
	deletesym (sp, tags);
    }
    scope.local = block->prev;
    scope_level--;
    return block;
}

void beginfuncblock P1 (const BLOCK *, block)
{
    SYM    *sp, *sp2;

    assert (block != NULL);

    beginblock ();
    for (sp = symbolsof (block); sp != NIL_SYM; sp = nextsym (sp)) {
	sp2 = copy_sym (sp);
	addsym (sp2, &scope.local->symbols);
	insertsym (sp2, symbols);
    }
    for (sp = block->tags.head; sp != NIL_SYM; sp = nextsym (sp)) {
	sp2 = copy_sym (sp);
	addsym (sp2, &scope.local->tags);
	insertsym (sp2, tags);
    }
}

void endfuncblock P0 (void)
{
    endblock ();
}

/*
 * There is only one tag table at any scoping level ... therefore although
 * structures have a new symbol table for members they share the same tag
 * table.
 */
void beginstructblock P1 (BLOCK *, block)
{
    if (block) {
	block->prev = scope.local;
	block->offset = lc_auto;
	scope.local = block;
	if (scope.global == NIL_BLOCK) {
	    scope.global = block;
	}
	scope_level++;
    } else {
	beginblock ();
    }
    scope.local->tags = scope.local->prev->tags;
}

BLOCK  *endstructblock P0 (void)
{
    SYM    *sp;
    BLOCK  *block = scope.local;

    for (sp = symbolsof (block); sp != NIL_SYM; sp = nextsym (sp)) {
	deletesym (sp, symbols);
    }
    scope.local = block->prev;
    scope.local->tags = block->tags;
    scope_level--;
    for (sp = symbolsof (block); sp != NIL_SYM; sp = nextsym (sp)) {
	if (is_const (sp)) {
	    SYM    *sp2 = copy_sym (sp);

	    set_level (sp2, scope_level);
	    sym_append (&sp2);
	}
    }
    block->tags.head = block->tags.tail = NIL_SYM;
    for (sp = scope.local->tags.head; sp != NIL_SYM; sp = nextsym (sp)) {
	set_level (sp, scope_level);
    }
    return block;
}

/*
 *   declare an internal symbol "name" of type "tp".
 */
SYM    *internal_symbol P2 (const CHAR *, name, TYP *, tp)
{
    SYM    *sp = sym_search (name);

    if (sp == NIL_SYM) {
	tp = mk_type (tp_func, tp);
	sp = mk_sym (name, sc_external, tp);
	sym_append (&sp);
	symbol_used (sp);
    }
    return sp;
}
