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

/******************************************************************************
 *
 *      This module contains the support routines for the code generates
 *      which are common to all the code generators.
 *
 *      Author:   K. D. Walker          February 1995.
 *
 *****************************************************************************/

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "outproto.h"

/*****************************************************************************/

/*
 * tests if it is a constant node, that means either en_icon, en_nacon or
 * en_labcon, or sums or differences of such nodes
 */
BOOL tst_const P1 (const EXPR *, ep)
{
    switch (ep->nodetype) {
    case en_icon:
    case en_nacon:
    case en_labcon:
    case en_sym:
	return TRUE;
    case en_add:
    case en_sub:
#ifdef RELOC_BUG
	/*
	 * Assembler cannot handle Label1-Label2
	 */
	if (islabel (ep->v.p[0]) && islabel (ep->v.p[1])) {
	    return FALSE;
	}
#endif /* RELOC_BUG */
	return tst_const (ep->v.p[0]) && tst_const (ep->v.p[1]);
    case en_cast:
    case en_uminus:
    case en_not:
    case en_test:
    case en_compl:
	return tst_const (ep->v.p[0]);
    default:
	break;
    }
    return FALSE;
}

/*
 * exchange the two operands in a node.
 */
void swap_nodes P1 (EXPR *, ep)
{
    EXPR   *temp;

    temp = ep->v.p[0];
    ep->v.p[0] = ep->v.p[1];
    ep->v.p[1] = temp;
    switch (ep->nodetype) {
    case en_lt:
	ep->nodetype = en_gt;
	break;
    case en_le:
	ep->nodetype = en_ge;
	break;
    case en_gt:
	ep->nodetype = en_lt;
	break;
    case en_ge:
	ep->nodetype = en_le;
	break;
    default:
	break;
    }
}

/*
 * Create a mask "width" bits wide
 */
UVAL bitmask P1 (BITSIZE, width)
{
    UVAL    mask = (UVAL) 0;

    while (width--)
	mask = mask + mask + (UVAL) 1;
    return mask;
}


#ifdef CPU_DEFINED

#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
typedef struct _ftree
{
    LABEL   label;
    RVAL    value;
    struct _ftree *less;
    struct _ftree *more;
} FTREE;

#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */

/*
 * make a constant expression node with the value i.
 */
EXPR   *mk_const P1 (IVAL, i)
{
    return mk_icon (i, tp_void);
}

/*
 * make a relative reference to a global node.
 */
EXPR   *mk_global P2 (const CHAR *, s, const EXPR *, ep)
{
    EXPR   *ep1, *ep2;
    SYM    *sp = internal_symbol (s, tp_void);

    ep1 = mk_node (en_nacon, NIL_EXPR, NIL_EXPR, tp_void);
    ep1->v.str = ep->v.str;
    ep2 = mk_node (en_nacon, NIL_EXPR, NIL_EXPR, tp_void);
    ep2->v.str = nameof (sp);
    ep1 = mk_node (en_sub, ep1, ep2, tp_void);
    return ep1;
}

/*
 * make an add node.
 */
EXPR   *mk_add P2 (EXPR *, ep1, EXPR *, ep2)
{
    return mk_node (en_add, ep1, ep2, tp_void);
}

#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
/*
 * Generate a label which points to the floating point constant.  This routine
 * ensures that only one copy of the constant is generated.
 */
LABEL mk_flabel P2 (const RVAL *, vp, const TYP *, tp)
{
    FTREE  *p, *q;
    int     local_global = global_flag;
    LABEL   lab;

    static FTREE *fptree = NULL;	/* Tree for floating point constants */
    static FTREE *dfptree = NULL;	/* Tree for double floating point constants */
    static FTREE *lfptree = NULL;	/* Tree for long double floating point constants */

    switch (tp->type) {
    case bt_float:
	p = fptree;
	break;
    case bt_double:
	p = dfptree;
	break;
    case bt_longdouble:
	p = lfptree;
	break;
    default:
	CANNOT_REACH_HERE ();
	p = NULL;   /*lint !e527*/  /* unreachable */
	break;
    }
    for (q = p; p; p = FLT (p->value, *(const RVAL *) vp) ? p->less : p->more) {
	if (FEQ (p->value, *(const RVAL *) vp)) {
	    return p->label;
	}
	q = p;
    }
    global_flag = 1;
    p = (FTREE *) xalloc (sizeof (FTREE));
    global_flag = local_global;
    p->label = lab = nextlabel++;
    FASSIGN (p->value, *(const RVAL *) vp);
    p->less = p->more = NULL;
    if (q == NULL) {
	switch (tp->type) {
	case bt_float:
	    fptree = p;
	    break;
	case bt_double:
	    dfptree = p;
	    break;
	case bt_longdouble:
	    lfptree = p;
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
    } else if (FLT (q->value, *(const RVAL *) vp)) {
	q->less = p;
    } else {
	q->more = p;
    }
    put_kseg (alignment_of_type (tp));
    put_label (lab);
    switch (tp->type) {
    case bt_float:
	put_float ((const RVAL *) vp);
	break;
    case bt_double:
	put_double ((const RVAL *) vp);
	break;
    case bt_longdouble:
	put_longdouble ((const RVAL *) vp);
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return lab;
}
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */

/*
 * ensure that all delayed adjustments to the stack have been performed.
 */
void sync_stack P0 (void)
{
    if (!is_parameter) {
	g_stack (stack_offset);
    }
}

#ifdef RELOC_BUG
/*
 * return true if the node addresses a label
 */
static BOOL islabel P1 (const EXPR *, ep)
{
    switch (ep->nodetype) {
    case en_nacon:
    case en_labcon:
	return TRUE;
    case en_cast:
	return islabel (ep->v.p[0]);
    }
    return FALSE;
}
#endif /* RELOC_BUG */

#ifdef DEBUGOPT
/*
 * makes a string  and allocates memory for it
 *
 * returns pointer to generated string
 */
const CHAR *mk_string P1 (const CHAR *, s)
{
    CHAR   *t;
    const CHAR *string;
    size_t  len;
    int     local_global = global_flag;

    /* work out the size of the string */
    for (len = (size_t) 0; s[len]; len++) {
	if (s[len] == (CHAR) '\n') {
	    break;
	}
    }

    global_flag = 0;		/* always allocate from local space. */
    t = (CHAR *) xalloc (len + (size_t) 1);
    string = (const CHAR *) t;
    while (len--)
	*t++ = *s++;
    *t = (CHAR) 0;
    global_flag = local_global;
    return (string);
}
#endif /* DEBUGOPT */

#endif /* CPU_DEFINED */
