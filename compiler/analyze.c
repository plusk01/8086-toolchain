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

/*****************************************************************************
 *
 * This module will step through the parse tree and find all optimisable
 * expressions.  At present these expressions are limited to expressions
 * that are valid throughout the scope of the function. the list of
 * optimisable expressions is:
 *
 *      constants
 *      global and static addresses
 *      auto addresses
 *      contents of auto addresses.
 *
 * Contents of auto addresses are valid only if the address is never
 * referred to without dereferencing.
 *
 * Scan() will build a list of optimisable expressions which opt1 will
 * replace during the second optimization pass.
 *
 *****************************************************************************/

#include "chdr.h"
#ifdef CPU_DEFINED
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/********************************************************** Type Definitions */

typedef int WEIGHT;		/* weighting of expression complexity */

/********************************************************** Static Variables */

static int regptr = 0;		/* index into reglst */
static SYM *reglst[REG_LIST];
static int autoptr = 0;		/* index into autolst */
static SYM *autolst[AUTO_LIST];
static CSE *olist;		/* list of optimisable expressions */

/*********************************************** Static Function Definitions */

static BOOL is_equalsym P_ ((const SYM *, const SYM *));
static CSE *searchnode P_ ((const EXPR *));
static CSE *enternode P_ ((const EXPR *, BOOL));
static CSE *voidauto P_ ((const EXPR *));
static void scannode P_ ((const EXPR *, BOOL));
static STMT *scan P_ ((STMT *));
static void bsort P_ ((CSE **));
static EXPR *repcsenode P_ ((EXPR *));
static STMT *repcse P_ ((STMT *));
static WEIGHT swapnode P_ ((EXPR *));
static STMT *swap P_ ((STMT *));

/*****************************************************************************/

void rel_autolst P0 (void)
{
    int i;

    for (i = 0; i < AUTO_LIST; i++)
    {
	autolst [i] = NIL_SYM;
    }
    autoptr = 0;
}

void rel_reglst P0 (void)
{
    int i;

    for (i = 0; i < REG_LIST; i++)
    {
	reglst [i] = NIL_SYM;
    }
    regptr = 0;
}

void addoptinfo P2 (SYM *, sp, STORAGE, sc)
{
    TYP    *tp = typeof (sp);

    switch (tp->type) {
    case bt_pointer32:
	if (is_array_type (tp) && sc != sc_parms) {
	    break;
	}
    /*lint -fallthrough*/
    case bt_bool:
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_float:
    case bt_double:
	if (!is_volatile_qualified (tp)) {
	    switch (storageof (sp)) {
	    case sc_register:
		if (regptr < REG_LIST) {
		    reglst[regptr++] = sp;
		    DPRINTF (
			     (DEBUG_GLOBAL, "adding '%s' to reglst\n",
			      nameof (sp)));
		}
		break;
	    case sc_parms:
	    case sc_auto:
		if (autoptr < AUTO_LIST) {
		    autolst[autoptr++] = sp;
		    DPRINTF (
			     (DEBUG_GLOBAL, "adding '%s' to autolst\n",
			      nameof (sp)));
		}
		break;
	    default:
		break;
	    }
	}
	break;
    default:
	break;
    }
}

/*
 * Remove the expression from the register lists
 */
void deloptinfo P1 (const EXPR *, ep)
{
    int     i;

    assert (is_sym (ep));
    for (i = 0; i < autoptr; i++) {
	if (is_equalsym (autolst[i], ep->v.sp)) {
	    autolst[i] = NIL_SYM;
	    DPRINTF (
		     (DEBUG_GLOBAL, "removing '%s' from autolst\n",
		      nameof (ep->v.sp)));
	}
    }
    for (i = 0; i < regptr; i++) {
	if (is_equalsym (reglst[i], ep->v.sp)) {
#ifndef SYNTAX_CORRECT
	    message (ERR_ADDREGVAR, reglst[i]->name);
#endif /* SYNTAX_CORRECT */
	    reglst[i] = NIL_SYM;
	    DPRINTF ((DEBUG_GLOBAL, "removing '%s' from reglst\n",
		      nameof (ep->v.sp)));
	}
    }
}

/*****************************************************************************/

/*
 *   used as a parameter to walkstmt() when no action is to be
 *   performed on the expressions.
 */
static EXPR *null_expr P1 (EXPR *, ep)
{
    return ep;
}

/*
 *   used as a paremeter to walkstmt() when no action is to be
 *   performed on the statement.
 */
static STMT *null_stmt P1 (STMT *, stmt)
{
    return stmt;
}

/*****************************************************************************/

/*
 *   is_equalsym() will return TRUE if the symbols pointed to by sp1
 *   and sp2 are equivalent.
 */

static BOOL is_equalsym P2 (const SYM *, sp1, const SYM *, sp2)
{
    if (sp1 == NIL_SYM || sp2 == NIL_SYM) {
	return FALSE;
    }
    if (sp1 == sp2) {
	return TRUE;
    }
    if (storageof (sp1) != storageof (sp2)) {
	return FALSE;
    }
    switch (storageof (sp1)) {
    case sc_auto:
    case sc_parms:
    case sc_register:
	return (sp1->value.i == sp2->value.i);
    case sc_external:
    case sc_global:
	return (nameof (sp1) == nameof (sp2));
    default:
	return FALSE;
    }
}

/*
 *   is_equalnode() will return TRUE if the expressions pointed to by ep1
 *   and ep2 are equivalent.
 */
BOOL is_equalnode P2 (const EXPR *, ep1, const EXPR *, ep2)
{
    if (ep1 == NIL_EXPR || ep2 == NIL_EXPR) {
	return FALSE;
    }
    if (ep1->nodetype != ep2->nodetype) {
	return FALSE;
    }
    switch (ep1->nodetype) {
    case en_icon:
    case en_autocon:
	return (ep1->v.i == ep2->v.i);
    case en_labcon:
	return (ep1->v.l == ep2->v.l);
    case en_nacon:
    case en_global:
	return (ep1->v.str == ep2->v.str);
    case en_sym:
	return is_equalsym (ep1->v.sp, ep2->v.sp);
    case en_ref:
    case en_uminus:
    case en_cast:
	return is_equalnode (ep1->v.p[0], ep2->v.p[0]);
    case en_fieldref:
	return is_equalnode (ep1->v.bit.p, ep2->v.bit.p);
    case en_add:
    case en_sub:
	return is_equalnode (ep1->v.p[1], ep2->v.p[1]) &&
	    is_equalnode (ep1->v.p[0], ep2->v.p[0]);
    default:
	return FALSE;
    }
}


/*
 *   searchnode will search the common expression table for an entry that
 *   matches the node passed and return a pointer to it.
 */
static CSE *searchnode P1 (const EXPR *, ep)
{
    register CSE *csp;

    if (ep == NIL_EXPR) {
	return NIL_CSE;
    }
    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	if (is_equalnode (ep, csp->exp)) {
	    return csp;
	}
    }
    return NIL_CSE;
}

/*****************************************************************************/

/*
 *   returns the desirability of optimization for a subexpression.
 */
USES desire P1 (const CSE *, csp)
{
    const EXPR *ep = csp->exp;

    if (csp->voidf || (is_icon (ep) && ep->v.i < 32L && ep->v.i >= 0L))
	return (USES) 0;
    if (is_lvalue (ep)) {
	return (USES) ((int) csp->uses * 2);
    }
    return csp->uses;
}

/*
 *   bsort() implements a bubble sort on the expression list.
 */
static void bsort P1 (CSE **, lst)
{
    CSE    *csp1, *csp2;
    register USES uses;

    csp1 = *lst;
    if (csp1 == NIL_CSE || csp1->next == NIL_CSE) {
	return;
    }
    bsort (&(csp1->next));
    uses = desire (csp1);
    while ((csp1 != NIL_CSE) &&
	   (csp2 = csp1->next) != NIL_CSE && uses < desire (csp2)) {
	*lst = csp2;
	csp1->next = csp2->next;
	csp2->next = csp1;
	lst = &(csp2->next);
    }
}

/*****************************************************************************/

/*
 *   enternode() will enter a reference to an expression node into
 *   the common expression table.
 *
 *   duse is a flag indicating whether or not this reference will be
 *   dereferenced.
 */
static CSE *enternode P2 (const EXPR *, ep, BOOL, duse)
{
    CSE    *csp;

    if ((csp = searchnode (ep)) == NIL_CSE) {	/* add to tree */
	csp = (CSE *) xalloc (sizeof (CSE));

	csp->next = olist;
	csp->uses = (USES) 0;
	csp->duses = (USES) 0;
	csp->exp = copynode (ep);
	csp->voidf = FALSE;
	csp->reg = NO_REG;
	olist = csp;
	return csp;
    }
    /*
     *       Integer constants may be in the table with different sizes --
     *       keep the maximum size
     */
    if (is_icon (ep) && ep->etp->size > csp->exp->etp->size) {
	csp->exp->etp = ep->etp;
    }
    ++(csp->uses);
    if (duse) {
	++(csp->duses);
    }
    return csp;
}


/*
 *   voidauto() will void an auto dereference node which points to the
 *   same auto constant as ep.
 */

static CSE *voidauto P1 (const EXPR *, ep)
{
    CSE    *csp;

    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	if (is_lvalue (csp->exp) && is_equalnode (ep, csp->exp->v.p[0])) {
	    if (csp->voidf) {
		return NIL_CSE;
	    }
	    csp->voidf = TRUE;
	    return csp;
	}
    }
    return NIL_CSE;
}


/*
 *   scannode() will scan the expression pointed to by node for
 *   optimisable subexpressions.
 *
 *   When an optimisable expression is found it is entered into the tree.
 *   If a reference to an auto node is scanned the corresponding
 *   auto dereferenced node will be voided.
 *
 *   duse should be set if the expression will be dereferenced.
 */

static void scannode P2 (const EXPR *, ep, BOOL, duse)
{
    EXPR   *ep0;
    CSE    *csp, *csp1;

    if (ep == NIL_EXPR) {
	return;
    }
    switch (ep->nodetype) {
    case en_fcon:
    case en_str:
	break;
    case en_icon:
    case en_autocon:
    case en_sym:
	/*
	 *   look if the dereferenced use of the node is in the
	 *   list, remove it in this case.
	 */
	if ((csp = voidauto (ep)) != NIL_CSE) {
	    csp1 = enternode (ep, duse);
	    csp1->duses += csp->uses;
	    break;
	}
    /*lint -fallthrough*/
    case en_labcon:
    case en_global:
    case en_nacon:
	VOIDCAST enternode (ep, duse);
	break;
    case en_ref:
    case en_fieldref:
	ep0 = ep->v.p[0];
	if (is_sym (ep0) &&
	    (is_auto (ep0->v.sp) ||
	     is_parms (ep0->v.sp) || is_register (ep0->v.sp))) {
	    BOOL    first = (searchnode (ep) == NIL_CSE);

	    csp = enternode (ep, duse);
	    if (searchnode (ep0) != NIL_CSE) {
		/*
		 *   the non-dereferenced use of the auto node
		 *   is already in the list.
		 */
		csp->voidf = TRUE;
		scannode (ep0, TRUE);
	    } else if (first) {
		/*
		 *   look for register nodes
		 */
		int     i;
		SYM    *sp = ep0->v.sp;

		for (i = 0; i < regptr; ++i) {
		    if (is_equalsym (reglst[i], sp)) {
			csp->voidf--;	/* this is not in auto_lst */
			csp->uses += (USES) (90 * (100 - i));
			csp->duses += (USES) (30 * (100 - i));
			break;
		    }
		}

		/*
		 *   set voidf if the node is not in autolst
		 */
		csp->voidf++;
		for (i = 0; i < autoptr; ++i) {
		    if (is_equalsym (autolst[i], sp)) {
			csp->voidf--;
			break;
		    }
		}

		/*
		 *   Even if that item must not be put in a register,
		 *   it is legal to put its address therein
		 */
		if (csp->voidf) {
		    scannode (ep0, TRUE);
		}
	    }
	} else {
	    scannode (ep0, TRUE);
	}
	break;
    case en_add:
    case en_sub:
	scannode (ep->v.p[1], duse);
	scannode (ep->v.p[0], duse);
	break;
    case en_ainc:
    case en_adec:
    case en_deref:
	scannode (ep->v.p[0], duse);
	break;
    case en_asadd:
    case en_assub:
    case en_mul:
    case en_div:
    case en_lsh:
    case en_rsh:
    case en_mod:
    case en_and:
    case en_or:
    case en_xor:
    case en_lor:
    case en_land:
    case en_eq:
    case en_ne:
    case en_gt:
    case en_ge:
    case en_lt:
    case en_le:
    case en_asmul:
    case en_asmul2:
    case en_asdiv:
    case en_asdiv2:
    case en_asmod:
    case en_aslsh:
    case en_asrsh:
    case en_asand:
    case en_asor:
    case en_asxor:
    case en_cond:
    case en_comma:
    case en_list:
    case en_assign:
    case en_litval:
    case en_literal:
	scannode (ep->v.p[1], FALSE);
     /*lint -fallthrough*/
    case en_uminus:
    case en_compl:
    case en_not:
    case en_test:
    case en_cast:
	scannode (ep->v.p[0], FALSE);
	break;
    case en_fcall:
    case en_call:
    case en_usercall:
	scannode (ep->v.p[0], TRUE);
	scannode (ep->v.p[1], FALSE);
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
}

static EXPR *scanexpr P1 (EXPR *, ep)
{
    scannode (ep, FALSE);
    return ep;
}


/*
 *   scan() will gather all optimisable expressions into the
 *   expression list for a block of statements.
 */

static STMT *scan P1 (STMT *, stmt)
{
    return walkstmt (stmt, null_stmt, scanexpr);
}

/*****************************************************************************/

/*
 *   constant() will fold all constant expression in the statement
 *   tree.
 */
static STMT *constant P1 (STMT *, stmt)
{
    return walkstmt (stmt, null_stmt, constantopt);
}

/*****************************************************************************/

static EXPR *unsymbolnode P1 (EXPR *, ep)
{
    SYM    *sp;
    TYP    *tp;

    switch (ep->nodetype) {
    case en_sym:
	sp = ep->v.sp;
	tp = typeof (sp);
	switch (storageof (sp)) {
	case sc_static:
	    if (!is_func (tp)) {
		ep->nodetype = en_labcon;
		ep->v.l = sp->value.l;
		ep->etp = mk_type (tp_pointer, tp);
		break;
	    }
	/*lint -fallthrough*/
	case sc_global:
	case sc_external:
	    if (datamodel_option && !is_func (tp)) {
		ep->nodetype = en_global;
	    } else {
		ep->nodetype = en_nacon;
	    }
	    ep->v.str = nameof (sp);
	    ep->etp = mk_type (tp_pointer, tp);
	    break;
	case sc_const:
	    ep->nodetype = en_icon;
	    ep->v.i = sp->value.i;
	    break;
	case sc_register:
	case sc_auto:
	case sc_parms:
	    ep->etp = mk_type (tp_pointer, tp);
	    ep->nodetype = en_autocon;
	    ep->v.i = sp->value.i;
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	break;
    default:
	break;
    }
    return ep;
}

EXPR   *unsymbolexpr P1 (EXPR *, ep)
{
    return walkexpr (ep, unsymbolnode);
}

/*
 *    unsymbol() will remove all references to en_sym nodes.
 */
static STMT *unsymbol P1 (STMT *, stmt)
{
    CSE    *csp;

    stmt = walkstmt (stmt, null_stmt, unsymbolexpr);
    /* must also change the Common Sub-expression tree */
    for (csp = olist; csp; csp = csp->next) {
	csp->exp = walkexpr (csp->exp, unsymbolnode);
    }
    return stmt;
}

/*****************************************************************************/

static EXPR *transformnode P1 (EXPR *, ep)
{
    return g_transform (ep);
}

static EXPR *transformexpr P1 (EXPR *, ep)
{
    return walkexpr (ep, transformnode);
}

/*
 *   transform() will descend the statement tree calling a code
 *   generator specific routine to transform all expressions which
 *   are performed by run-time support routines.
 */
static STMT *transform P1 (STMT *, stmt)
{
    return walkstmt (stmt, null_stmt, transformexpr);
}

/*****************************************************************************/

/*
 *   repcsenode() will replace all allocated references within an
 *   expression with register nodes.
 */
static EXPR *repcsenode P1 (EXPR *, ep)
{
    CSE    *csp;

    switch (ep->nodetype) {
    case en_fcon:
    case en_icon:
    case en_nacon:
    case en_labcon:
    case en_autocon:
    case en_sym:
    case en_ref:
    case en_fieldref:
	if (((csp = searchnode (ep)) != NIL_CSE) && (csp->reg != NO_REG)) {
	    ep->nodetype = en_register;
	    ep->v.r = csp->reg;
	}
	break;
    default:
	break;
    }
    return ep;
}

static EXPR *repcseexpr P1 (EXPR *, ep)
{
    return walkexpr (ep, repcsenode);
}

/*
 *    repcse() will scan through a block of statements replacing
 *    the optimized expressions with their temporary references.
 */
static STMT *repcse P1 (STMT *, stmt)
{
    return walkstmt (stmt, null_stmt, repcseexpr);
}

/*****************************************************************************/

static WEIGHT swapnode P1 (EXPR *, ep)
{
    WEIGHT  lweight, rweight;

    if (ep == NIL_EXPR) {
	return (WEIGHT) 0;
    }
    ep = opt0 (ep);
    switch (ep->nodetype) {
    case en_register:
	return (WEIGHT) 1;
    case en_icon:
    case en_fcon:
    case en_nacon:
    case en_labcon:
    case en_autocon:
    case en_global:
    case en_sym:
    case en_str:
    case en_literal:
	return (WEIGHT) 2;
    case en_ref:
	return swapnode (ep->v.p[0]);
    case en_fieldref:
	return swapnode (ep->v.bit.p);
    case en_uminus:
    case en_not:
    case en_test:
    case en_compl:
    case en_ainc:
    case en_adec:
    case en_cast:
    case en_deref:
	return swapnode (ep->v.p[0]) + 1;
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
	/* almost commutative operators */
    case en_eq:
    case en_ne:
    case en_add:
    case en_mul:
    case en_and:
    case en_or:
    case en_xor:
	/* commutative operators */
	lweight = swapnode (ep->v.p[0]);
	rweight = swapnode (ep->v.p[1]);
	if (rweight > lweight) {
	    swap_nodes (ep);
	}
	return lweight + rweight;
    case en_sub:
    case en_div:
    case en_mod:
    case en_lsh:
    case en_rsh:
    case en_land:
    case en_lor:
    case en_cond:
    case en_comma:
    case en_list:
    case en_asadd:
    case en_assub:
    case en_asmul:
    case en_asmul2:
    case en_asdiv:
    case en_asdiv2:
    case en_asor:
    case en_asxor:
    case en_asand:
    case en_asmod:
    case en_aslsh:
    case en_asrsh:
    case en_assign:
	return swapnode (ep->v.p[0]) + swapnode (ep->v.p[1]);
    case en_fcall:
    case en_call:
    case en_usercall:
	return (WEIGHT) 10 + swapnode (ep->v.p[0]) + swapnode (ep->v.p[1]);
    default:
	CANNOT_REACH_HERE ();
    }
    return (WEIGHT)0;	    /*lint !e527*/  /* unreachable */
}

static EXPR *swapexpr P1 (EXPR *, ep)
{
    VOIDCAST swapnode (ep);

    return ep;
}

/*
 *   swap() will swap commutative expression nodes so that the most
 *   complex node will be evaluated first - thus possibly preventing
 *   an intermediate value from being temporarily pushed on the stack
 *   during expression evaluation.
 */
static STMT *swap P1 (STMT *, stmt)
{
    return walkstmt (stmt, null_stmt, swapexpr);
}

/*****************************************************************************/

static EXPR *ordernode P1 (EXPR *, ep)
{
    return g_order (ep);
}

static EXPR *orderexpr P1 (EXPR *, ep)
{
    return walkexpr (ep, ordernode);
}

static STMT *order P1 (STMT *, stmt)
{
    return walkstmt (stmt, null_stmt, orderexpr);
}

/*****************************************************************************/

static STMT *ordercasestmt P1 (STMT *, stmt)
{
    if (stmt == NIL_STMT) {
	return NIL_STMT;
    }

    switch (stmt->stype) {
    case st_case:
	if ((stmt->s1 != NIL_STMT) && stmt->s1 == stmt->v1.s) {
	    switch (stmt->s1->stype) {
	    case st_case:
		if (stmt->s1->v2.i < stmt->v2.i) {
		    /*
		     *	Put the labels into order as this may generate better
		     *	code.
		     */
		    IVAL    i = stmt->v2.i;

		    stmt->v2.i = stmt->s1->v2.i;
		    stmt->s1->v2.i = i;
		}
		break;
	    case st_default:
		/*
		 *  delete this case label as it is also handled by the default
		 *  label.
		 *
		 *  [The previous case label will be pointing to this entry so
		 *  copy the contents of the default node into this one.]
		 */
		*stmt = *stmt->s1;
		break;
	    default:
		break;
	    }
	}
	break;
    case st_default:
	if ((stmt->s1 != NULL) && (stmt->s1 == stmt->v1.s)) {
	    switch (stmt->s1->stype) {
	    case st_case:
		/*
		 *  delete this case label as it is also handled by the default
		 *  label.
		 */
		stmt->s1 = stmt->s1->v1.s;
		break;
	    default:
		break;
	    }
	}
	break;
    default:
	break;
    }
    return stmt;
}

/*
 *   descend the statement tree recursively and sort consecutive case
 *   statements into ascending order.
 *
 *   deletes a case label if it is on the same branch as the default
 *   label.
 */
static STMT *ordercase P1 (STMT *, stmt)
{
    return walkstmt (stmt, ordercasestmt, null_expr);
}

/*****************************************************************************/

/*
 *   globalopt() is the externally callable optimization routine.
 *   It will collect and allocate common subexpressions and substitute
 *   the tempref for all occurrences of the expression within
 *   the block.
 */
CSE    *globalopt P1 (STMT *, stmt)
{
    olist = NIL_CSE;

#ifdef ICODE
#ifdef DEBUG
    if (icode_option) {
	if (is_debugging (DEBUG_GLOBAL)) {
	    genicode (stmt, 0);
	}
    }
#endif /* DEBUG */
#endif /* ICODE */
    stmt = constant (stmt);		/* constant folding */
    stmt = transform (stmt);

#ifdef ICODE
#ifdef DEBUG
    if (icode_option) {
	if (is_debugging (DEBUG_GLOBAL)) {
	    genicode (stmt, 0);
	}
    }
#endif /* DEBUG */
#endif /* ICODE */
    if (opt_option) {
	stmt = scan (stmt);	/* collect expressions */
	bsort (&olist);		/* sort expressions list into usage order */
    }
    stmt = unsymbol (stmt);	/* replace all references to symbols */
#ifdef ICODE
#ifdef DEBUG
    if (icode_option) {
	if (is_debugging (DEBUG_GLOBAL)) {
	    genicode (stmt, 0);
	}
    }
#endif /* DEBUG */
#endif /* ICODE */
    if (opt_option) {
	g_allocate (olist);	/* allocate registers */
	stmt = repcse (stmt);	/* replace allocated expressions */
	stmt = swap (stmt);	/* swap commutative expression */
	stmt = order (stmt);	/* code generation order */
	stmt = ordercase (stmt);
    }
#ifdef ICODE
    if (icode_option) {
#ifdef DEBUG
	if (is_debugging (DEBUG_GLOBAL)) {
	    genicode (stmt, 0);
        }
#endif /* DEBUG */
	genicse (olist);
    }
#endif /* ICODE */
    rel_autolst ();
    rel_reglst ();
    return olist;
}
#endif /* CPU_DEFINED */
