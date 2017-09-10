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
 *		  originally by M. Brandt
 * 1990   68000 C compiler further bug fixes
 *		  started i386 port (December)
 * 1991   i386 port finished (January)
 *		  further corrections in the front end and in the 68000
 *		  code generator.
 *		  The next port will be a SPARC port
 */

/*****************************************************************************/

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/*********************************************** Static Function Definitions */

static IVAL xfold P_ ((EXPR *));
static EXPR *dooper P_ ((EXPR *));
static EXPR *fold_const P_ ((EXPR *));

/*****************************************************************************/

/*
 *	 This function handles the adjustment of integer constants upon
 *	 casts. It forces the constant into the range acceptable for
 *	 the given type.
 */

static IVAL strip_icon P2 (IVAL, i, const TYP *, tp)
{
    switch (tp->type) {
    case bt_bool:
	i = (IVAL) (i ? 1 : 0);
	break;
    case bt_charu:
    case bt_uchar:		/* 0 .. 255 */
	i &= (IVAL) 0xFFL;
	break;
    case bt_char:		/* -128 .. 127 */
    case bt_schar:
	i &= (IVAL) 0xFFUL;
	if ((UVAL) i > 0x7FUL) {
	    i -= (IVAL) 0x100UL;
	}
	break;
    case bt_ushort:		/* 0 .. 65535 */
    case bt_uint16:
	i &= (IVAL) 0xFFFFL;
	break;
    case bt_short:		/* -32768 .. 32767 */
    case bt_int16:
	i &= (IVAL) 0xFFFFL;
	if ((UVAL) i > 0x7FFFUL) {
	    i -= (IVAL) 0x10000UL;
	}
	break;
#ifdef LONGLONG_SUPPORT
    case bt_uint32:
    case bt_ulong:
	i = (IVAL) ((UVAL) i & (UVAL) 0xFFFFFFFFUL);
	break;
    case bt_int32:
    case bt_long:
	i = (IVAL) ((UVAL) i & (UVAL) 0xFFFFFFFFUL);
	if ((UVAL) i > 0x7FFFFFFFUL) {
	    
#ifdef LONGLONG_BOOTSTRAP
	    i -= (IVAL) 0x40000000UL;
	    i -= (IVAL) 0x40000000UL;
	    i -= (IVAL) 0x40000000UL;
	    i -= (IVAL) 0x40000000UL;
	    
#else
	    i -= (IVAL) 0x100000000UL;
#endif	/* LONGLONG_BOOTSTRAP */
	}
	break;
#endif /* LONGLONG_SUPPORT */
    default:
	break;
    }
    return i;
}

/*
 *	 dooper() will execute a constant operation in a node and return
 *	 the node to be the result of the operation.
 */
static EXPR *dooper P1 (EXPR *, ep)
{
    EXPRTYPE type = ep->nodetype;

    ep->nodetype = ep->v.p[0]->nodetype;
    switch (ep->v.p[0]->nodetype) {
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
	RVAL    f;
#endif	/* FLOAT_BOOTSTRAP */

    case en_fcon:
#ifndef FLOAT_BOOTSTRAP
	FASSIGN (f, ep->v.p[0]->v.f);
	switch (type) {
	case en_uminus:
	    FASSIGN (ep->v.f, f);
	    FNEG (ep->v.f);
	    break;
	case en_test:
	    ep->v.i = FTST (f) ? (IVAL) 1 : (IVAL) 0;
	    ep->nodetype = en_icon;
	    break;
	case en_not:
	    ep->v.i = FTST (f) ? (IVAL) 0 : (IVAL) 1;
	    ep->nodetype = en_icon;
	    break;
	case en_cast:
	    if (is_real_floating_type (ep->etp)) {
		ep->v.f = f;
	    } else if (is_bool (ep->etp)) {
		ep->v.u = (UVAL) (f ? 1 : 0);
		ep->nodetype = en_icon;
	    } else {
		FTOL (ep->v.i, f);
		ep->v.i = strip_icon (ep->v.i, ep->etp);
		ep->nodetype = en_icon;
	    }
	    break;
	case en_add:
	    FADD3 (ep->v.f, f, ep->v.p[1]->v.f);
	    break;
	case en_sub:
	    FSUB3 (ep->v.f, f, ep->v.p[1]->v.f);
	    break;
	case en_mul:
	    FMUL3 (ep->v.f, f, ep->v.p[1]->v.f);
	    break;
	case en_div:
	    if (FTST (ep->v.p[1]->v.f)) {
		FDIV3 (ep->v.f, f, ep->v.p[1]->v.f);
	    } else {
		ep->nodetype = en_div;
	    }
	    break;
	case en_eq:
	    ep->v.i = (IVAL) FEQ (f, ep->v.p[1]->v.f);
	    ep->nodetype = en_icon;
	    break;
	case en_ne:
	    ep->v.i = (IVAL) FNE (f, ep->v.p[1]->v.f);
	    ep->nodetype = en_icon;
	    break;
	case en_land:
	    ep->v.i = (IVAL) (FTST (f) && FTST (ep->v.p[1]->v.f));
	    ep->nodetype = en_icon;
	    break;
	case en_lor:
	    ep->v.i = (IVAL) (FTST (f) || FTST (ep->v.p[1]->v.f));
	    ep->nodetype = en_icon;
	    break;
	case en_lt:
	    ep->v.i = (IVAL) FLT (f, ep->v.p[1]->v.f);
	    ep->nodetype = en_icon;
	    break;
	case en_le:
	    ep->v.i = (IVAL) FLE (f, ep->v.p[1]->v.f);
	    ep->nodetype = en_icon;
	    break;
	case en_gt:
	    ep->v.i = (IVAL) FGT (f, ep->v.p[1]->v.f);
	    ep->nodetype = en_icon;
	    break;
	case en_ge:
	    ep->v.i = (IVAL) FGE (f, ep->v.p[1]->v.f);
	    ep->nodetype = en_icon;
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	break;
	
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */
    case en_icon:
	if (is_unsigned_type (ep->v.p[0]->etp)) {
	    UVAL    u = ep->v.p[0]->v.u;

	    switch (type) {
	    case en_uminus:
		/*
		 *       unary minus on an unsigned is normally a mistake so we must
		 *       fool the compiler into not giving a warning.
		 */
		ep->v.u = (UVAL) (-(IVAL) u);
		break;
	    case en_test:
		ep->v.u = (u ? (UVAL) 1 : (UVAL) 0);
		break;
	    case en_not:
		ep->v.u = (u ? (UVAL) 0 : (UVAL) 1);
		break;
	    case en_compl:
		ep->v.u = (UVAL) strip_icon ((IVAL) ~u, ep->etp);
		break;
	    case en_cast:
		if (is_bool (ep->etp)) {
		    ep->v.u = (UVAL) (u ? 1 : 0);
#ifdef FLOAT_SUPPORT
		} else if (is_real_floating_type (ep->etp)) {
		    ep->nodetype = en_fcon;
		    UTOF (ep->v.f, u);
		    break;
#endif /* FLOAT_SUPPORT */
		} else {
		    ep->v.u = (UVAL) strip_icon ((IVAL) u, ep->etp);
		}
		break;
	    case en_add:
		ep->v.u = u + ep->v.p[1]->v.u;
		break;
	    case en_sub:
		ep->v.u = u - ep->v.p[1]->v.u;
		break;
	    case en_mul:
		ep->v.u = u * ep->v.p[1]->v.u;
		break;
	    case en_div:
		if (ep->v.p[1]->v.u == (UVAL) 0) {
		    ep->nodetype = en_div;
		} else {
		    ep->v.u = u / ep->v.p[1]->v.u;
		}
		break;
	    case en_mod:
		if (ep->v.p[1]->v.u == (UVAL) 0) {
		    ep->nodetype = en_mod;
		} else {
		    ep->v.u = u % ep->v.p[1]->v.u;
		}
		break;
	    case en_and:
		ep->v.u = u & ep->v.p[1]->v.u;
		break;
	    case en_or:
		ep->v.u = u | ep->v.p[1]->v.u;
		break;
	    case en_xor:
		ep->v.u = u ^ ep->v.p[1]->v.u;
		break;
	    case en_eq:
		ep->v.u = (UVAL) (u == ep->v.p[1]->v.u);
		break;
	    case en_ne:
		ep->v.u = (UVAL) (u != ep->v.p[1]->v.u);
		break;
	    case en_land:
		ep->v.u = (UVAL) (u && ep->v.p[1]->v.u);
		break;
	    case en_lor:
		ep->v.u = (UVAL) (u || ep->v.p[1]->v.u);
		break;
	    case en_lt:
		ep->v.u = (UVAL) (u < ep->v.p[1]->v.u);
		break;
	    case en_le:
		ep->v.u = (UVAL) (u <= ep->v.p[1]->v.u);
		break;
	    case en_gt:
		ep->v.u = (UVAL) (u > ep->v.p[1]->v.u);
		break;
	    case en_ge:
		ep->v.u = (UVAL) (u >= ep->v.p[1]->v.u);
		break;
	    case en_lsh:
		ep->v.u = u << ep->v.p[1]->v.u;
		break;
	    case en_rsh:
		ep->v.u = u >> ep->v.p[1]->v.u;
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	} else {
	    IVAL    i = ep->v.p[0]->v.i;

	    switch (type) {
	    case en_uminus:
		ep->v.i = -i;
		break;
	    case en_test:
		ep->v.i = (i) ? (IVAL) 1 : (IVAL) 0;
		break;
	    case en_not:
		ep->v.i = (i) ? (IVAL) 0 : (IVAL) 1;
		break;
	    case en_compl:
		ep->v.i = strip_icon ((IVAL) (~(UVAL) i), ep->etp);
		break;
	    case en_cast:
		if (is_bool (ep->etp)) {
		    ep->v.u = (UVAL) (i ? 1 : 0);
#ifdef FLOAT_SUPPORT
		} else if (is_real_floating_type (ep->etp)) {
		    ep->nodetype = en_fcon;
		    LTOF (ep->v.f, i);
		    break;
#endif /* FLOAT_SUPPORT */
		} else {
		    ep->v.i = strip_icon (i, ep->etp);
		}
		break;
	    case en_add:
		ep->v.i = i + ep->v.p[1]->v.i;
		break;
	    case en_sub:
		ep->v.i = i - ep->v.p[1]->v.i;
		break;
	    case en_mul:
		ep->v.i = i * ep->v.p[1]->v.i;
		break;
	    case en_div:
		if (ep->v.p[1]->v.i == (IVAL) 0) {
		    ep->nodetype = en_div;
		} else {
		    ep->v.i = i / ep->v.p[1]->v.i;
		}
		break;
	    case en_mod:
		if (ep->v.p[1]->v.i == (IVAL) 0) {
		    ep->nodetype = en_mod;
		} else {
		    ep->v.i = i % ep->v.p[1]->v.i;
		}
		break;
	    case en_and:
		ep->v.i = (IVAL) ((UVAL) i & (UVAL) ep->v.p[1]->v.i);
		break;
	    case en_or:
		ep->v.i = (IVAL) ((UVAL) i | (UVAL) ep->v.p[1]->v.i);
		break;
	    case en_xor:
		ep->v.i = (IVAL) ((UVAL) i ^ (UVAL) ep->v.p[1]->v.i);
		break;
	    case en_eq:
		ep->v.i = (IVAL) (i == ep->v.p[1]->v.i);
		break;
	    case en_ne:
		ep->v.i = (IVAL) (i != ep->v.p[1]->v.i);
		break;
	    case en_land:
		ep->v.i = (IVAL) (i && ep->v.p[1]->v.i);
		break;
	    case en_lor:
		ep->v.i = (IVAL) (i || ep->v.p[1]->v.i);
		break;
	    case en_lt:
		ep->v.i = (IVAL) (i < ep->v.p[1]->v.i);
		break;
	    case en_le:
		ep->v.i = (IVAL) (i <= ep->v.p[1]->v.i);
		break;
	    case en_gt:
		ep->v.i = (IVAL) (i > ep->v.p[1]->v.i);
		break;
	    case en_ge:
		ep->v.i = (IVAL) (i >= ep->v.p[1]->v.i);
		break;
	    case en_lsh:
		ep->v.i = (IVAL) ((UVAL) i << ep->v.p[1]->v.i);
		break;
	    case en_rsh:
		ep->v.i = (IVAL) (i >> ep->v.p[1]->v.i);
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	}
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return ep;
}


/*
 *	 return which power of two i is or -1.
 */
int pwrof2 P1 (IVAL, i)
{
    int     p;
    IVAL    q;

    q = (IVAL) 1;
    p = 0;
    while (q > (IVAL) 0) {
	if (q == i) {
	    return p;
	}
	q <<= 1L;
	++p;
    }
    return -1;
}


#ifdef CPU_DEFINED
/*
 *	 This routine attempts to optimize bitfield references ... if
 *	 it makes a change it returns a pointer to the new node,
 *	 otherwise it returns NULL;
 */
static EXPR *simplify_fieldref P3 (EXPR *, ep, TYP *, tp1, TYP *, tp2)
{
    if (g_is_bigendian ()) {
	if (ep->nodetype == en_fieldref) {
	    SIZE    algn = alignment_of_type (tp1) * bits_in_sizeunit;

	    if (((SIZE) ep->v.bit.offset % algn + (SIZE) ep->v.bit.width) <=
		tp1->size * bits_in_sizeunit) {
		SIZE    adjust = (SIZE) ep->v.bit.offset / algn;

		ep->v.bit.offset -= (BITSIZE) (adjust * bits_in_sizeunit);
		adjust = ep->etp->size - (tp1->size + adjust);
		if (adjust != 0L) {
		    ep->v.bit.p = opt0 (mk_node (en_add, ep->v.bit.p,
						mk_icon ((IVAL) adjust,
							 tp_pointer),
						ep->etp));
		}
		ep->etp = is_signed_type (ep->etp) ? tp1 : tp2;
		if ((ep->v.bit.offset == (BITSIZE) 0) &&
		    (ep->v.bit.width ==
		     (BITSIZE) (tp1->size * bits_in_sizeunit))) {
		    /*
		     *            Fits exactly within the type so change to an
		     *           ordinary reference as this is much more efficient
		     */
		    ep->nodetype = en_ref;
		}
	    }
	}
    }
    return ep;
}
#endif /* CPU_DEFINED */

/*
 *	 optnode() - delete useless expressions and combine constants.
 *
 *	 optnode will delete expressions such as
 *	  x + 0,
 *	  x - 0,
 *	  x * 0,
 *	  x * 1,
 *	  0 / x,
 *	  x / 1,
 *	  x mod 0,
 *	  etc from the tree pointed to by node and combine obvious
 *	 constant operations. It cannot combine name and label constants
 *	 but will combine icon type nodes.
 */
static EXPR *optnode P1 (EXPR *, ep)
{
    IVAL    val, sc;
    EXPR   *ep0, *ep1, *ep2;
    TYP    *tp = ep->etp;

    switch (ep->nodetype) {
    case en_uminus:
	ep0 = ep->v.p[0];
	if (ep0->nodetype == en_sub)
	{
	    /*
	     *  -(a-b)  => b-a
	     */
	    ep1 = ep0->v.p[0];
	    ep0->v.p[0] = ep0->v.p[1];
	    ep0->v.p[1] = ep1;
	    ep = optnode (ep0);
	    return ep;
	}
	/*lint -fallthrough*/
    case en_compl:
	ep0 = ep->v.p[0];
	if (ep0->nodetype == ep->nodetype) {
	    /*
	     *  -(-a)   => a
	     *  ~(~a)   => a
	     */
	    ep = ep0->v.p[0];
	    return optnode (ep);
	}
	switch (ep0->nodetype) {
	case en_cond:
	    
	    /*
	     *  -(a ? b : c)    =>  a ? -b : -c
	     *  ~(a ? b : c)    =>  a ? ~b : ~c
	     */ 
	    ep1 = ep0->v.p[1];
	    ep1->v.p[0] = mk_node (ep->nodetype, ep1->v.p[0], NIL_EXPR, tp);
	    ep1->v.p[1] = mk_node (ep->nodetype, ep1->v.p[1], NIL_EXPR, tp);
	    ep = ep0;
	    return optnode (ep);
	default:
	    if (is_icon (ep0) || is_fcon (ep0)) {
		ep = dooper (ep);
		return ep;
	    }
	}
	break;
    case en_test:
	ep0 = ep->v.p[0];
	switch (ep0->nodetype) {
	case en_test:
	    ep = ep0->v.p[0];
	    return optnode (ep);
        case en_not:
	    ep = ep0;
	    return optnode (ep);
	case en_icon:
	case en_fcon:
	    ep = dooper (ep);
	    return ep;
	case en_labcon:
	case en_sym:
	    ep->nodetype = en_icon;
	    ep->v.i = 1L;
	    return ep;
	default:
	    break;
	}
	break;
    case en_not:
	ep0 = ep->v.p[0];
	switch (ep0->nodetype) {
	case en_not:
	    /*
	     *  !(!a)  => a
	     */
	    ep->nodetype = en_test;
	    ep->v.p[0] = ep0->v.p[0];
	    return optnode (ep);
	case en_icon:
	case en_fcon:
	    ep = dooper (ep);
	    return ep;
	case en_test:
	    ep->v.p[0] = ep0->v.p[0];
	    return optnode (ep);
	case en_lor:
	    /*
	     *  !(a || b)  =>  !a && !b
	     */
	    ep = ep0;
	    ep->nodetype = en_land;
	    ep->v.p[0] = mk_node (en_not, ep->v.p[0], NIL_EXPR, tp);
	    ep->v.p[1] = mk_node (en_not, ep->v.p[1], NIL_EXPR, tp);
	    return optnode (ep);
	case en_land:
	    /*
	     *  !(a && b)  =>  !a || !b
	     */
	    ep = ep0;
	    ep->nodetype = en_lor;
	    ep->v.p[0] = mk_node (en_not, ep->v.p[0], NIL_EXPR, tp);
	    ep->v.p[1] = mk_node (en_not, ep->v.p[1], NIL_EXPR, tp);
	    return optnode (ep);
	case en_cond:
	    
	    /*
	     *  !(a ? b : c)    =>  a ? !b : !c
	     */ 
	    ep = ep0;
	    ep1 = ep->v.p[1];
	    ep1->v.p[0] = mk_node (en_not, ep1->v.p[0], NIL_EXPR, tp);
	    ep1->v.p[1] = mk_node (en_not, ep1->v.p[1], NIL_EXPR, tp);
	    return optnode (ep);
	case en_labcon:
	case en_sym:
	    ep->nodetype = en_icon;
	    ep->v.i = 0L;
	    return ep;
	default:
	    break;
	}
	break;
    case en_cast:
	ep0 = ep->v.p[0];
	if (is_compatible_type (tp, ep0->etp)) {
	    ep0->etp = tp;
	    return optnode (ep0);	/* redundant cast */
	}
	switch (ep0->nodetype) {
	case en_icon:
	case en_fcon:
	    ep = dooper (ep);
	    return ep;
	case en_register:
	    if ((ep0->etp->size == 4L) && (tp->size == 4L) &&
		is_integral_type (ep0->etp) && is_integral_type (tp)) {
		ep0->etp = tp;
		return ep0;
	    }
	    break;
	case en_add:
	case en_sub:
	case en_mul:
	case en_div:
	case en_mod:
	case en_and:
	case en_or:
	case en_xor:
	case en_lsh:
	case en_rsh:
	case en_land:
	case en_lor:
	case en_eq:
	case en_ne:
	case en_lt:
	case en_le:
	case en_gt:
	case en_ge:
	    /*
	     *  Find where the operands to an operator have been cast to
	     *  a wider type only for the result to be cast back to the
	     *  narrower type.
	     */
	    if (is_subtype (ep0->etp, tp)) {
		ep1 = ep0->v.p[0];
		ep2 = ep0->v.p[1];
		if ((ep1->nodetype == en_cast) && (ep2->nodetype == en_cast)) {
		    if (is_subtype (tp, ep1->v.p[0]->etp) &&
			is_subtype (tp, ep2->v.p[0]->etp)) {
			ep1->etp = tp;
			ep2->etp = tp;
			ep0->etp = tp;
			return optnode (ep0);
		    }
		} else if ((ep1->nodetype == en_cast) && (is_icon (ep2))) {
		    if (is_subtype (tp, ep1->v.p[0]->etp) &&
			is_constant_in_range (ep2, tp)) {
			ep1->etp = tp;
			ep0->etp = tp;
			return optnode (ep0);
		    }
		} else if (is_icon (ep1) && (ep2->nodetype == en_cast)) {
		    if (is_subtype (tp, ep2->v.p[0]->etp) &&
			is_constant_in_range (ep1, tp)) {
			ep2->etp = tp;
			ep0->etp = tp;
			return optnode (ep0);
		    }
		}
	    }
	    break;
	default:
	    break;
	}
	break;
    case en_add:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep->v.p[0]) ||
	    (ep0->nodetype == en_uminus) || (ep1->nodetype == en_autocon)) {
	    swap_nodes (ep);
	    ep0 = ep->v.p[0];
	    ep1 = ep->v.p[1];
	}
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (ep1->nodetype == en_uminus) {
	    /*
	     *  a + (-b)  =>  a - b
	     */
	    ep->v.p[1] = ep1->v.p[0];
	    ep->nodetype = en_sub;
	    return optnode (ep);
	}
	if (is_icon (ep1)) {
	    switch (ep0->nodetype) {
	    case en_autocon:
		ep = ep0;
		ep->v.i += ep1->v.i;
		ep->etp = tp;
		return ep;
	    default:
		if (ep1->v.i == 0L) {
		    /*
		     *  a + 0   => a
		     */
		    ep = ep0;
		    ep->etp = tp;
		    return ep;
		}
		break;
	    }
	}
	break;
    case en_sub:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (ep1->nodetype == en_uminus) {
	    /*
	     *    a - (-b)      =>      a + b
	     */
	    ep->v.p[1] = ep1->v.p[0];
	    ep->nodetype = en_add;
	    return optnode (ep);
	}
	if (ep1->nodetype == en_sub) {
	    /*
	     *    a - (b - c)  =>  a + (c - b)
	     */
	    swap_nodes (ep1);
	    ep->nodetype = en_add;
	    return optnode (ep);
	}
	if (is_icon (ep1) && (ep1->v.i == 0L)) {
	    /*
	     *    a - 0   =>  a
	     */
	    ep = ep0;
	    ep->etp = tp;
	    return ep;
	}
	if (is_icon (ep0) && (ep0->v.i == 0L)) {
	    /*
	     *    0 - a   =>  -a
	     */
	    ep->v.p[0] = ep1;
	    ep->nodetype = en_uminus;
	    return optnode (ep);
	}
	break;
    case en_mul:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep0) ||
	    (ep0->nodetype == en_uminus)) {
	    swap_nodes (ep);
	    ep0 = ep->v.p[0];
	    ep1 = ep->v.p[1];
	}
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (ep1->nodetype == en_uminus) {
	    /*
	     *    a * (-b)  => -(a * b)
	     */
	    ep->v.p[1] = ep1->v.p[0];
	    ep1->v.p[0] = ep;
	    return optnode (ep1);
	}
	if (is_icon (ep1)) {
	    val = ep1->v.i;
	    if (val == (IVAL) 0) {
		/*
		 *   a * 0  => a , 0
		 */
		ep = mk_node (en_comma, ep0, ep1, tp);
		return optnode (ep);
	    }
	    if (val == (IVAL) 1) {
		/*
		 *   a * 1  => a
		 */
		ep = ep0;
		return ep;
	    }
	    sc = (IVAL) pwrof2 (val);
	    if (sc != (IVAL) -1) {
		/*
		 *   a * (2**n)  => a << n
		 */
		ep->v.p[1]->v.i = sc;
		ep->nodetype = en_lsh;
		return optnode (ep);
	    }
	}
	break;
    case en_div:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (ep1->nodetype == en_uminus) {
	    /*
	     *    a / (-b)  => -(a / b)
	     */
	    ep->v.p[1] = ep1->v.p[0];
	    ep1->v.p[0] = ep;
	    return optnode (ep1);
	}
	if (is_icon (ep0)) {
	    if (ep0->v.i == 0L) {
		/*
		 *  0/x => x,0
		 */
		ep = mk_node (en_comma, ep1, ep0, tp);
		return optnode (ep);
	    }
	} else if (is_icon (ep1)) {
	    val = ep1->v.i;
	    if (val == (IVAL) 1) {
		/*
		 *      x/1  => x
		 */
		ep = ep0;
		return optnode (ep);
	    }
	    if (is_unsigned_type (tp)) {
		sc = (IVAL) pwrof2 (val);
		if (sc != (IVAL) -1) {
		    /*
		     * x / (y**2)  => x>>y
		     */
		    ep->v.p[1]->v.i = sc;
		    ep->nodetype = en_rsh;
		    return optnode (ep);
		}
	    }
	}
	break;
    case en_mod:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (is_icon (ep0)) {
	    if (ep0->v.i == 0L) {
		/*
		 *  0%x => x,0
		 */
		ep = mk_node (en_comma, ep1, ep0, tp);
		return optnode (ep);
	    }
	}
	if (is_icon (ep1)) {
	    if (is_unsigned_type (tp)) {
		sc = (IVAL) pwrof2 (ep1->v.i);
		if (sc != (IVAL) -1) {
		    /*
		     *  x % (y**2)      => x & (y-1)
		     */
		    ep->v.p[1]->v.u = bitmask ((BITSIZE) sc);
		    ep->nodetype = en_and;
		    return optnode (ep);
		}
	    }
	}
	break;
    case en_lor:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (is_icon (ep0)) {
	    if (ep0->v.i) {
		/*
		 *        1 || a  =>  1
		 */
		ep = ep0;
		ep->v.i = (IVAL) TRUE;
	    } else {
		/*
		 *        0 || a  =>  a
		 */
		ep = ep1;
	    }
	    return optnode (ep);
	}
	if (is_icon (ep1) && (ep0->v.i == 0L)) {
	    /*
	     *    a || 0  =>  a
	     */
	    ep = ep0;
	    return optnode (ep);
	}
	break;
    case en_land:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (is_icon (ep0)) {
	    /*
	     *    0 && a  =>  0
	     *    1 && a  =>  a
	     */
	    /* short-circuit! */
	    ep = (ep0->v.i) ? ep1 : ep0;
	    return optnode (ep);
	}
	if (is_icon (ep1) && (ep1->v.i != 0L)) {
	    /*
	     *    a && 1  =>  a
	     */
	    ep = ep0;
	    return optnode (ep);
	}
	break;
    case en_eq:
    case en_ne:
	if (is_icon (ep->v.p[0]) || is_fcon (ep->v.p[0])) {
	    swap_nodes (ep);
	}
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (is_icon (ep1) && (ep1->v.i == 0L)) {
	    /*
	     *    a == 0  =>  !a
	     *    a != 0  =>  a
	     */
	    ep->nodetype = (ep->nodetype == en_eq) ? en_not : en_test;
	    return optnode (ep);
	}
#ifdef FLOAT_SUPPORT
	if (is_fcon (ep1) && FEQ (ep1->v.f, F_zero)) {
	    ep->nodetype = (ep->nodetype == en_eq) ? en_not : en_test;
	    return optnode (ep);
	}
#endif /* FLOAT_SUPPORT */

	break;

    case en_and:
    case en_or:
    case en_xor:
	if (is_icon (ep->v.p[0])) {
	    swap_nodes (ep);
	}
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (is_icon (ep1)) {
	    switch (ep->nodetype) {
	    case en_and:
		if (ep1->v.i == 0L) {
		    /*
		     *  a & 0   => 0
		     */
		    ep = ep1;
		    
#ifndef LONGLONG_BOOTSTRAP
		} else if (ep1->v.i == (IVAL) ~(UVAL) 0x0UL) {
		    /*
		     *  a & ~0  => a
		     */
		    ep = optnode (ep0);
		    
#endif /* LONGLONG_BOOTSTRAP */
		}
		break;
	    case en_or:
		if (ep1->v.i == 0L) {
		    /*
		     *  a | 0   => a
		     */
		    ep = optnode (ep0);
		    
#ifndef LONGLONG_BOOTSTRAP
		} else if (ep1->v.i == (IVAL) ~(UVAL) 0x0UL) {
		    /*
		     *  a | ~0  => ~0
		     */
		    ep = ep1;
		    
#endif /* LONGLONG_BOOTSTRAP */
		}
		break;
	    case en_xor:
		if (ep1->v.i == 0L) {
		    /*
		     *  a ^ 0   => a
		     */
		    ep = optnode (ep0);
		    
#ifndef LONGLONG_BOOTSTRAP
		} else if (ep1->v.i == (IVAL) ~(UVAL) 0x0UL) {
		    /*
		     *  a ^ ~0  => ~a
		     */
		    ep->nodetype = en_compl;
		    ep = optnode (ep);
		    
#endif /* LONGLONG_BOOTSTRAP */
		}
		break;
	    default:
		CANNOT_REACH_HERE ();
	    }
	    return ep;
	}
	break;
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
	if (is_icon (ep->v.p[0])) {
	    swap_nodes (ep);
	}
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	/*
	 *       constant expressions
	 */
	if ((is_icon (ep0) && is_icon (ep1)) ||
	    (is_fcon (ep0) && is_fcon (ep1))) {
	    ep = dooper (ep);
	    return ep;
	}
	if (is_unsigned_type (ep0->etp) && is_icon (ep1)
	    && (ep1->v.u == Ox0UL)) {
	    switch (ep->nodetype) {
	    case en_lt:
		/*
		 *      a < 0   =>      0
		 */
		ep->v.i = 0L;
		ep->nodetype = en_icon;
		break;
	    case en_le:
		/*
		 *      a <= 0  =>      a == 0
		 */
		ep->nodetype = en_eq;
		ep = optnode (ep);
		break;
	    case en_gt:
		/*
		 *      a > 0   =>      a != 0
		 */
		ep->nodetype = en_ne;
		ep = optnode (ep);
		break;
	    case en_ge:
		/*
		 *      a >= 1  =>      1
		 */
		ep->v.i = 1L;
		ep->nodetype = en_icon;
		break;
	    default:
		CANNOT_REACH_HERE ();
	    }
	}
	break;
    case en_lsh:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep0) && is_icon (ep1)) {
	    ep = dooper (ep);
	    return ep;
	}
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *   a << 0 => a
		 */
		ep = optnode (ep0);
		return ep;
	    }
	    if (ep1->v.i >= (tp->size*8))
	    {
		/*
		 *   a << n => a, 0
		 */
		ep1->v.i = 0;
		ep1->nodetype = en_icon;
		ep->nodetype = en_comma;
		ep = optnode (ep);
		return ep;
	    }
	}
	break;
    case en_rsh:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep0) && is_icon (ep1)) {
	    ep = dooper (ep);
	    return ep;
	}
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *   a >> 0 => a
		 */
		ep = optnode (ep0);
		return ep;
	    }
	    if ((ep1->v.i >= (tp->size*8)) && is_unsigned_type (tp))
	    {
		/*
		 *   a >> n => a, 0 (unsigned)
		 */
		ep1->v.i = 0;
		ep1->nodetype = en_icon;
		ep->nodetype = en_comma;
		ep = optnode (ep);
		return ep;
	    }
	}
	break;
    case en_cond:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep0)) {
	    if (ep0->v.i) {
		/*
		 *              1 ? x, y  => x
		 */
		ep = optnode (ep1->v.p[0]);
	    } else {
		/*
		 *              0 ? x, y  => y
		 */
		ep = optnode (ep1->v.p[1]);
	    }
	    return ep;
	}
	break;
    case en_asadd:
    case en_assub:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *      x += 0  =>      x
		 */
		ep = optnode (ep0->v.p[0]);
		return ep;
	    }
	}
	break;
    case en_asand:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *      x &= 0  =>      x = 0
		 */
		ep->nodetype = en_assign;
		ep = optnode (ep);
		return ep;
	    }
	    
#ifndef LONGLONG_BOOTSTRAP
	    if (ep1->v.i == (IVAL) ~(UVAL) 0x0UL) {
		/*
		 *      x &= ~0 =>      x
		 */
		ep = optnode (ep0->v.p[0]);
		return ep;
	    }
	    
#endif /* LONGLONG_BOOTSTRAP */
	}
	break;
    case en_asor:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *      x |= 0  =>      x
		 */
		ep = optnode (ep0->v.p[0]);
		return ep;
	    }
	    
#ifndef LONGLONG_BOOTSTRAP
	    if (ep1->v.i == (IVAL) ~(UVAL) 0x0UL) {
		/*
		 *      x |= ~0 =>      x = ~0
		 */
		ep->nodetype = en_assign;
		ep = optnode (ep);
		return ep;
	    }
	    
#endif /* LONGLONG_BOOTSTRAP */
	}
	break;
    case en_asmul:
    case en_asmul2:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *      x *= 0  =>      x = 0
		 */
		ep->nodetype = en_assign;
		ep = optnode (ep);
		return ep;
	    }
	    if (ep1->v.i == 1L) {
		/*
		 *      x *= 1  =>      x
		 */
		ep = optnode (ep0->v.p[0]);
		return ep;
	    }
	    sc = (IVAL) pwrof2 (ep1->v.i);
	    if (sc != (IVAL) -1) {
		/*
		 *   a *= (2**n)  => a <<= n
		 */
		ep->v.p[1]->v.i = sc;
		ep->nodetype = en_aslsh;
		return optnode (ep);
	    }
	}
	break;
    case en_comma:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep0)) {
	    /*
	     *  constant, x =>  x
	     */
	    ep = optnode (ep1);
	    return ep;
	}
	break;
    case en_asrsh:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *      x <<= 0 =>      x
		 */
		ep = optnode (ep0->v.p[0]);
		return ep;
	    }
	    if ((ep1->v.i > tp->size*8) && is_unsigned_type (tp)) {
		/*
		 *	x <<= n =>	x = 0
		 */
		ep1->v.i = 0;
		ep->nodetype = en_assign;
		ep = optnode (ep);
		return ep;
	    }
	}
	break;
    case en_aslsh:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *      x <<= 0 =>      x
		 */
		ep = optnode (ep0->v.p[0]);
		return ep;
	    }
	    if (ep1->v.i >= (tp->size*8)) {
		/*
		 *	x <<= n =>	x = 0
		 */
		ep1->v.i = 0;
		ep->nodetype = en_assign;
		ep = optnode (ep);
		return ep;
	    }
	}
	break;
    case en_asxor:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (ep1->v.i == 0L) {
		/*
		 *      x ^= 0  =>      x
		 */
		ep = optnode (ep0->v.p[0]);
		return ep;
	    }
#ifndef LONGLONG_BOOTSTRAP
	    if (ep1->v.i == (IVAL) ~(UVAL) 0x0UL) {
		/*
		 *	x ^= ~0	=>	~x
		 */
		ep->nodetype = en_compl;
		ep = optnode (ep);
		return ep;
	    }
#endif /* LONGLONG_BOOTSTRAP */
	}
	break;
    case en_asdiv:
    case en_asdiv2:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (ep1->v.i == 1L) {
		/*
		 *      x /= 1  =>      x
		 */
		ep = optnode (ep0->v.p[0]);
		return ep;
	    }
	}
	break;
    case en_asmod:
	ep0 = ep->v.p[0];
	ep1 = ep->v.p[1];
	if (is_icon (ep1)) {
	    if (is_unsigned_type (tp)) {
		sc = (IVAL) pwrof2 (ep1->v.i);
		if (sc != (IVAL) -1) {
		    /*
		     *  x %= (y**2)  => x &= (y-1)
		     */
		    ep->v.p[1]->v.u = bitmask ((BITSIZE) sc);
		    ep->nodetype = en_asand;
		    return optnode (ep);
		}
	    }
	}
	break;
    case en_fcall:
    case en_call:
    case en_usercall:
    case en_list:
    case en_assign:
    case en_ref:
    case en_ainc:
    case en_adec:
    case en_deref:
	break;
    case en_fieldref:
	/*
	 *        If possible reduce bit-field references to ordinary
	 *              references as they are much more efficient
	 */
#ifdef CPU_DEFINED
	ep1 = simplify_fieldref (ep, tp_char, tp_uchar);
	if (ep1 == NIL_EXPR) {
	    ep1 = simplify_fieldref (ep, tp_short, tp_ushort);
	    if (ep1 == NIL_EXPR) {
		ep1 = simplify_fieldref (ep, tp_long, tp_ulong);
	    }
	}
	return ep1 ? ep1 : ep;
#else
	return ep;
#endif
    default:
	break;
    }
    return ep;
}

EXPR   *opt0 P1 (EXPR *, ep)
{
    return walkexpr (ep, optnode);
}

/*
 *	 xfold() will remove constant nodes and return the values to the
 *	 calling routines.
 */
static IVAL xfold P1 (EXPR *, ep)
{
    IVAL    i;

    if (ep == NIL_EXPR) {
	return 0L;
    }
    switch (ep->nodetype) {
    case en_icon:
	i = ep->v.i;
	ep->v.i = 0L;
	return i;
    case en_add:
	i = xfold (ep->v.p[0]) + xfold (ep->v.p[1]);
	return i;
    case en_sub:
	return xfold (ep->v.p[0]) - xfold (ep->v.p[1]);
    case en_mul:
	if (is_icon (ep->v.p[0])) {
	    return xfold (ep->v.p[1]) * ep->v.p[0]->v.i;
	} else if (is_icon (ep->v.p[1])) {
	    return xfold (ep->v.p[0]) * ep->v.p[1]->v.i;
	}
	/*lint -fallthrough */
    case en_lsh:
    case en_rsh:
    case en_div:
    case en_mod:
    case en_asadd:
    case en_assub:
    case en_asmul:
    case en_asmul2:
    case en_asdiv:
    case en_asdiv2:
    case en_asmod:
    case en_and:
    case en_land:
    case en_or:
    case en_lor:
    case en_xor:
    case en_asand:
    case en_asor:
    case en_asxor:
    case en_comma:
    case en_list:
    case en_fcall:
    case en_call:
    case en_usercall:
    case en_assign:
    case en_eq:
    case en_ne:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
	ep->v.p[1] = fold_const (ep->v.p[1]);
	/*lint -fallthrough */
    case en_ref:
    case en_compl:
    case en_test:
    case en_not:
    case en_deref:
    case en_cast:
	ep->v.p[0] = fold_const (ep->v.p[0]);
	return 0L;
    case en_fieldref:
	ep->v.bit.p = fold_const (ep->v.bit.p);
	return 0L;
    case en_uminus:
	return -xfold (ep->v.p[0]);
    default:
	break;
    }
    return 0L;
}

/*
 *	 reorganise an expression for optimal constant grouping.
 */
static EXPR *fold_const P1 (EXPR *, ep)
{
    EXPR   *ep1;
    IVAL    i;

    if (ep == NIL_EXPR) {
	return ep;
    }
    switch (ep->nodetype) {
    case en_add:
	if (is_icon (ep->v.p[0])) {
	    swap_nodes (ep);
	}
	if (is_icon (ep->v.p[1])) {
	    ep->v.p[1]->v.i += xfold (ep->v.p[0]);
	    return ep;
	}
	break;
    case en_sub:
	if (is_icon (ep->v.p[0])) {
	    ep->v.p[0]->v.i -= xfold (ep->v.p[1]);
	    return ep;
	}
	if (is_icon (ep->v.p[1])) {
	    ep->v.p[1]->v.i -= xfold (ep->v.p[0]);
	    return ep;
	}
	break;
    default:
	break;
    }
    i = xfold (ep);
    if (i != 0L) {
	/*
	 *       strip_icon() is in fact harmless here since this value is
	 *       just added to ep
	 *
	 *       consider in 16-bit mode:
	 *
	 *                       int day, year;
	 *                       day = 365 * (year - 1970);
	 *
	 *       and look at the code, which is transformed to
	 *
	 *                       day = 365*year + 1846;
	 *
	 *       which works if the multiplication returns the lower 16 bits
	 *       of the result correctly.
	 */
	i = strip_icon (i, ep->etp);
	ep1 = mk_icon (i, ep->etp);
	ep = mk_node (en_add, ep, ep1, ep->etp);
    }
    return ep;
}


/*
 *	 apply all constant optimizations.
 */
EXPR   *constantopt P1 (EXPR *, ep)
{
    ep = fold_const (ep);
    return opt0 (ep);
}
