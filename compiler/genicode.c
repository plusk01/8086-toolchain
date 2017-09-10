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
 * This module performs a symbolic dump of the parse tree
 *
 *****************************************************************************/

#include "config.h"

#ifdef ICODE

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/********************************************************* Macro Definitions */

#define	INDENTATION	2	/* number of columns to indent */

/*********************************************** Static Function Definitions */

static void type P_ ((const TYP *));
static void g_icode P_ ((const EXPR *, int));

/*****************************************************************************/

static void type P1 (const TYP *, tp)
{
    iprintf (" %ld ", tp->size);
    if (is_const_qualified (tp)) {
	iprintf ("const ");
    }
    if (is_volatile_qualified (tp)) {
	iprintf ("volatile ");
    }
    if (is_restrict_qualified (tp)) {
	iprintf ("restrict");
    }
    switch (tp->type) {
    case bt_bool:
	iprintf ("_Bool ");
	break;
    case bt_uchar:
	iprintf ("unsigned ");
	/*lint -fallthrough */
    case bt_char:
    case bt_charu:
	iprintf ("char ");
	break;
    case bt_schar:
	iprintf ("signed char ");
	break;
    case bt_ushort:
	iprintf ("unsigned ");
	/*lint -fallthrough */
    case bt_short:
	iprintf ("short ");
	break;
    case bt_uint16:
    case bt_uint32:
	iprintf ("unsigned ");
	/*lint -fallthrough */
    case bt_int16:
    case bt_int32:
	iprintf ("int ");
	break;
    case bt_ulong:
	iprintf ("unsigned ");
	/*lint -fallthrough */
    case bt_long:
	iprintf ("long ");
	break;
    case bt_pointer16:
    case bt_pointer32:
	if (is_array_type (tp)) {
	    iprintf ("array of ");
	} else {
	    iprintf ("pointer to ");
	}
	type (referenced_type (tp));
	break;
    case bt_longlong:
	iprintf ("long long ");
	break;
    case bt_ulonglong:
	iprintf ("unsigned long long ");
	break;
    case bt_struct:
	iprintf ("struct ");
	break;
    case bt_union:
	iprintf ("union ");
	break;
    case bt_func:
	iprintf ("function returning ");
	type (returned_type (tp));
	break;
    case bt_longdouble:
	iprintf ("long ");
	/*lint -fallthrough */
    case bt_double:
	iprintf ("double ");
	break;
    case bt_float:
	iprintf ("float ");
	break;
    case bt_floatcomplex:
	iprintf ("float _Complex ");
	break;
    case bt_doublecomplex:
	iprintf ("double _Complex ");
	break;
    case bt_longdoublecomplex:
	iprintf ("long double _Complex ");
	break;
    case bt_floatimaginary:
	iprintf ("float _Imaginary ");
	break;
    case bt_doubleimaginary:
	iprintf ("double _Imaginary ");
	break;
    case bt_longdoubleimaginary:
	iprintf ("long double _Imaginary ");
	break;
    case bt_void:
	iprintf ("void ");
	break;
    case bt_bitfield:
	iprintf ("int (bitfield) ");
	break;
    case bt_ubitfield:
	iprintf ("unsigned int (bitfield) ");
	break;
    case bt_bbitfield:
	iprintf ("_Bool (bitfield) ");
	break;
    case bt_ellipsis:
	iprintf ("... ");
	break;
    default:
	FATAL ((__FILE__, "type", "illegal type %d", tp->type));
    }
}

/*
 * general expression evaluation. symbolic dump.
 */
static void g_icode P2 (const EXPR *, ep, int, indent)
{
    int     j;

    for (j = indent; j >= 0; j--)
	iprintf (" ");
    if (ep == NIL_EXPR) {
	iprintf ("*NULL%s", newline);
	return;
    }
    switch (ep->nodetype) {
    case en_icon:
	iprintf ("*icon %ld", (long) ep->v.i);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
#ifdef FLOAT_SUPPORT
    case en_fcon:
	iprintf ("*fcon %20.10f", ep->v.f);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
#endif /* FLOAT_SUPPORT */
    case en_sym:
	iprintf ("*sym %s", nameof (ep->v.sp));
	type (ep->etp);
	iprintf ("%s", newline);
	break;
    case en_labcon:
	iprintf ("*labcon L%u", (unsigned) ep->v.l);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
    case en_nacon:
	iprintf ("*nacon %s", ep->v.str);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
    case en_autocon:
	iprintf ("*autocon %ld", (long) ep->v.i);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
    case en_global:
	iprintf ("*global %s", ep->v.str);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
    case en_ref:
	iprintf ("*ref");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_fieldref:
	iprintf ("*fref (%d,%d)", (int) ep->v.bit.offset,
		 (int) ep->v.bit.width);
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.bit.p, indent + INDENTATION);
	break;
    case en_register:
	iprintf ("*register %d", (int) ep->v.r);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
    case en_uminus:
	iprintf ("*uminus");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_compl:
	iprintf ("*compl");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_add:
	iprintf ("*add");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);
	break;
    case en_sub:
	iprintf ("*sub");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_and:
	iprintf ("*and");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_or:
	iprintf ("*or");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_xor:
	iprintf ("*xor");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_mul:
	iprintf ("*mul");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_div:
	iprintf ("*div");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_mod:
	iprintf ("*mod");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_lsh:
	iprintf ("*lsh");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_rsh:
	iprintf ("*rsh");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asadd:
	iprintf ("*asadd");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_assub:
	iprintf ("*assub");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asand:
	iprintf ("*asand");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asor:
	iprintf ("*asor");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asxor:
	iprintf ("*asxor");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_aslsh:
	iprintf ("*aslsh");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asrsh:
	iprintf ("*asrsh");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asmul:
	iprintf ("*asmul");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asmul2:
	iprintf ("*asmul2");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asdiv:
	iprintf ("*asdiv");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asdiv2:
	iprintf ("*asdiv2");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_asmod:
	iprintf ("*asmod");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_assign:
	iprintf ("*assign");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_ainc:
	iprintf ("*ainc value=%ld", (long) ep->v.p[1]->v.i);
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_adec:
	iprintf ("*adec value=%ld", (long) ep->v.p[1]->v.i);
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_land:
	iprintf ("*land");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_lor:
	iprintf ("*lor");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_eq:
	iprintf ("*eq");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_ne:
	iprintf ("*ne");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_lt:
	iprintf ("*lt");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_le:
	iprintf ("*le");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_gt:
	iprintf ("*gt");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_ge:
	iprintf ("*ge");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_test:
	iprintf ("*test");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_not:
	iprintf ("*not");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_cond:
	iprintf ("*cond");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	ep = ep->v.p[1];
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);;
	break;
    case en_comma:
	iprintf ("*comma");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	if (ep->v.p[1] != NIL_EXPR) {
	    g_icode (ep->v.p[1], indent + INDENTATION);
	} else {
	    iprintf ("*null");
	}
	break;
    case en_fcall:
	iprintf ("*fcall");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	if (ep->v.p[1] != NIL_EXPR) {
	    g_icode (ep->v.p[1], indent + INDENTATION);;
	}
	break;
    case en_call:
	iprintf ("*call");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	if (ep->v.p[1] != NIL_EXPR) {
	    g_icode (ep->v.p[1], indent + INDENTATION);;
	}
	break;
    case en_usercall:
	iprintf ("*usercall");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	if (ep->v.p[1] != NIL_EXPR) {
	    g_icode (ep->v.p[1], indent + INDENTATION);;
	}
	break;
    case en_cast:
	iprintf ("*cast");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_deref:
	iprintf ("*deref");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	break;
    case en_list:
	iprintf ("*list");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	if (ep->v.p[1] != NIL_EXPR) {
	    g_icode (ep->v.p[1], indent + INDENTATION);;
	}
	break;
    case en_literal:
	iprintf ("*literal");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);
	break;
    case en_litval:
	iprintf ("*litval");
	type (ep->etp);
	iprintf ("%s", newline);
	g_icode (ep->v.p[0], indent + INDENTATION);
	g_icode (ep->v.p[1], indent + INDENTATION);
	break;
    case en_str:
	iprintf ("*str \"%s\"", ep->v.str);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
    case en_size:
	iprintf ("*size %ld", (long) ep->v.i);
	type (ep->etp);
	iprintf ("%s", newline);
	break;
    default:
	FATAL ((__FILE__, "g_icode", "uncoded node %d", ep->nodetype));
    }
}

void genicse P1 (CSE *, olist)
{
    CSE    *csp;
    EXPR   *ep;

    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	ep = csp->exp;
#ifdef DEBUG
	if (is_debugging (DEBUG_GLOBAL)) {
	    iprintf ("%d:%s", (int) desire (csp), newline);
	    g_icode (ep, INDENTATION);
	}
#endif
	if (csp->reg != NO_REG) {	/* see if preload needed */
	    iprintf ("register %d:%s", (int) csp->reg, newline);
	    if (!is_lvalue (ep) || (ep->v.p[0]->v.i > 0L)) {
		g_icode (ep, INDENTATION);
	    }
	}
    }
}

void genicode P2 (const STMT *, stmt, int, indent)
{
    int     j;

    for (; stmt != NIL_STMT; stmt = stmt->next) {
	for (j = indent; j >= 0; j--)
	    iprintf (" ");
#ifdef DEBUGOPT
	iprintf ("line %d: ", stmt->line);
#endif /*DEBUGOPT */
	switch (stmt->stype) {
	case st_label:
	    iprintf ("L%u:%s", (unsigned int) stmt->v2.l, newline);
	    genicode (stmt->v1.s, indent);
	    break;
	case st_goto:
	    iprintf ("$goto\tL%u%s", (unsigned int) stmt->v2.l, newline);
	    break;
	case st_expr:
	    iprintf ("$expression%s", newline);
	    g_icode (stmt->exp, indent + INDENTATION);
	    break;
	case st_return:
	    iprintf ("$return%s", newline);
	    g_icode (stmt->exp, indent + INDENTATION);
	    break;
	case st_if:
	    iprintf ("$if%s", newline);
	    g_icode (stmt->exp, indent + INDENTATION);
	    genicode (stmt->s1, indent + INDENTATION);
	    if (stmt->v1.s != NIL_STMT) {
		genicode (stmt->v1.s, indent + INDENTATION);
	    }
	    break;
	case st_while:
	    iprintf ("$while%s", newline);
	    g_icode (stmt->exp, indent + INDENTATION);
	    genicode (stmt->s1, indent + INDENTATION);
	    break;
	case st_do:
	    iprintf ("$do%s", newline);
	    genicode (stmt->s1, indent + INDENTATION);
	    g_icode (stmt->exp, indent + INDENTATION);
	    break;
	case st_for:
	    iprintf ("$for%s", newline);
	    g_icode (stmt->exp, indent + INDENTATION);
	    g_icode (stmt->v1.e, indent + INDENTATION);
	    g_icode (stmt->v2.e, indent + INDENTATION);
	    genicode (stmt->s1, indent + INDENTATION);
	    break;
	case st_continue:
	    iprintf ("$continue%s", newline);
	    break;
	case st_break:
	    iprintf ("$break%s", newline);
	    break;
	case st_switch:
	    iprintf ("$switch%s", newline);
	    g_icode (stmt->exp, indent + INDENTATION);
	    genicode (stmt->v1.s, indent + INDENTATION);
	    break;
	case st_compound:
	    iprintf ("$compound%s", newline);
	    genicode (stmt->s1, indent + INDENTATION);
	    break;
	case st_case:
	    iprintf ("$case %ld:5%s", (long) stmt->v2.i, newline);
	    genicode (stmt->v1.s, indent + INDENTATION);
	    break;
	case st_default:
	    iprintf ("$default:5%s", newline);
	    genicode (stmt->v1.s, indent + INDENTATION);
	    break;
#ifdef ASM
	case st_asm:
	    iprintf ("$asm:%s", newline);
	    g_icode (stmt->exp, indent + INDENTATION);
	    break;
#endif /* ASM */
	default:
	    FATAL (
		   (__FILE__, "genicode", "uncoded statement %d",
		    stmt->stype));
	}
    }
}
#endif /* ICODE */
