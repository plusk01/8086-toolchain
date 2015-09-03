/*
 * C compiler
 * ==========
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

/*****************************************************************************/

#include "config.h"

#ifdef POWERPC

#define GEN_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genppc.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define	AL_DEFAULT	(g_alignments[bt_ellipsis])

/*********************************************** Static Function Definitions */

static ADDRESS *g_expr P_ ((const EXPR *, FLAGS));

PRIVATE BOOL g_is_bigendian P_ ((void));
PRIVATE BOOL g_is_ascending_stack P_ ((void));
PRIVATE void g_auto_align P_ ((void));
PRIVATE void g_flush P_ ((SYM *));
PRIVATE void g_allocate P_ ((CSE *));
PRIVATE void g_entry P_ ((SIZE));
PRIVATE void g_epilogue P_ ((void));
PRIVATE void g_expression P_ ((const EXPR *));
PRIVATE void g_jfalse P_ ((const EXPR *, LABEL));
PRIVATE void g_jtrue P_ ((const EXPR *, LABEL));
PRIVATE void g_return P_ ((const EXPR *, TYP *));
PRIVATE void g_stack P_ ((SIZE));
PRIVATE void g_switch_compare P_ ((const EXPR *, STMT *));
PRIVATE void g_switch_table P_ ((const EXPR *, const SWITCH *, UVAL, UVAL));
PRIVATE void g_initialize P_ ((void));

/********************************************************** Static Variables */

static unsigned peep_option = PEEP_ALL;	/* peephole optimisations */
static SIZE max_stack_adjust = 0L;	/* largest amount stack is altered */

/*
 *    The following tables specify the alignment requirements of the
 *      basic types depending on the processor type.
 */
static SIZE alignments_ppc[] = {
    1L,				/* bt_void      */
    1L,				/* bt_char      */
    1L,				/* bt_charu     */
    1L,				/* bt_uchar     */
    1L,				/* bt_schar     */
    2L,				/* bt_short     */
    2L,				/* bt_ushort    */
    2L,				/* bt_int16     */
    2L,				/* bt_uint16    */
    2L,				/* bt_int32     */
    2L,				/* bt_uint32    */
    2L,				/* bt_long      */
    2L,				/* bt_ulong     */
    2L,				/* bt_float     */
    2L,				/* bt_double    */
    2L,				/* bt_longdouble */
    2L,				/* bt_pointer16 */
    2L,				/* bt_pointer32 */
    2L,				/* bt_struct    */
    2L,				/* bt_union     */
    2L,				/* bt_func      */
    2L,				/* bt_bitfield  */
    2L,				/* bt_ubitfield */
    1L,				/* bt_bbitfield */
    2L				/* bt_ellipsis - used for alignment suitable for all types */
};

#ifndef MULTIPLE_PROCESSORS
PRIVATE SIZE *g_alignments = &alignments_powerpc[0];

#endif /* MULTIPLE_PROCESSORS */

/*****************************************************************************/

static ADDRESS *mk_amode P1 (AMODE, mode)
{
    ADDRESS *ap;
    ap = (ADDRESS *) xalloc (sizeof (ADDRESS));

    ap->mode = mode;
    return ap;
}

static ADDRESS *mk_expr P2 (AMODE, mode, EXPR *, ep)
{
    ADDRESS *ap;

    ap = mk_amode (mode);
    ap->u.offset = ep;
    return ap;
}

/*
 * make a node to reference an immediate value i.
 */
static ADDRESS *mk_immed P1 (IVAL, i)
{
    return mk_expr (am_immed, mk_const (i));
}


/*
 * construct a reference node for an internal label number.
 */
ADDRESS *mk_label P1 (LABEL, lab)
{
    return mk_expr (am_direct, mk_lcon (lab));
}

/*
 *    make an address reference to a register.
 */
ADDRESS *mk_reg P1 (REG, r)
{
}

/*
 * make a node to reference a line number.
 */
static ADDRESS *mk_line P1 (LINE, i)
{
    return mk_expr (am_line, mk_const ((IVAL) i));
}

/*
 * make a node to reference a source line.
 */
static ADDRESS *mk_linetxt P1 (const CHAR *, s)
{
    EXPR   *ep;

    ep = mk_node (en_str, NIL_EXPR, NIL_EXPR, tp_void);
    ep->v.str = s;
    return mk_expr (am_str, ep);
}


/*
 * add a compiler generated label to the peep list.
 */
PRIVATE void g_label P1 (LABEL, labno)
{
    sync_stack ();
    g_code (op_label, IL0, mk_label (labno), NIL_ADDRESS);
}

#ifdef DEBUGOPT
/*
 * add a source line number to the peep list.
 */
PRIVATE void g_line P2 (LINE, line, const CHAR *, linetxt)
{
    g_code (op_line, IL0, mk_line (line), mk_linetxt (linetxt));
}

#endif /*DEBUGOPT */

/*
 * add a conditional branch instruction to the peep list.
 */
static void g_cbranch P2 (OPCODE, op, LABEL, labno)
{
    sync_stack ();
    g_code (op, IL0, mk_label (labno), NIL_ADDRESS);
}

/*
 * add a branch instruction to the peep list.
 */
PRIVATE void g_branch P1 (LABEL, labno)
{
    g_cbranch (op_bra, labno);
}

/*
 * adjust the stack by "bytes" bytes.
 */
PRIVATE void g_stack P1 (SIZE, bytes)
{
    if (bytes != 0L) {
	/* adjust stack pointer */
	g_code (op_add, IL4, mk_immed (bytes), mk_reg (STACKPTR));
	stack_offset -= bytes;
	if (max_stack_adjust < bytes) {
	    max_stack_adjust = bytes;
	}
    }
}

/*
 * general expression evaluation. returns the addressing mode of the result.
 */
static ADDRESS *g_expr P2 (const EXPR *, ep, FLAGS, flags)
{
    return NIL_ADDRESS;
}

PRIVATE void g_expression P1 (const EXPR *, ep)
{
    initstack ();
    checkstack ();
}

PRIVATE void g_jfalse P2 (const EXPR *, ep, LABEL, label)
{
    initstack ();
    checkstack ();
}

PRIVATE void g_jtrue P2 (const EXPR *, ep, LABEL, label)
{
    initstack ();
    checkstack ();
}

PRIVATE void g_switch_table P4 (const EXPR *, ep, const SWITCH *, sw, UVAL,
				min_caselabel, UVAL, max_caselabel)
{
}

/*
 * Generate the body of a switch statement by comparing each case value
 * in turn.   The comparison is infact done by using subtraction as this
 * actually generates more efficient code (and would work best if the
 * labels were sorted!)
 */
PRIVATE void g_switch_compare P2 (const EXPR *, ep, STMT *, stmt)
{
}

/*
 *    Generate the code for setting up any local variables and
 *      the saving of any registers used.    This code is actually
 *      generated at the end of the function when the amount of
 *      stackspace actually required is known .... the peephole
 *      optimiser will move it back to the start of the function.
 */
PRIVATE void g_entry P1 (SIZE, frame_size)
{
}

/*
 *    Generate the code for a "return" statement.  This ensures
 *      that any returned result is loaded into the appropriate
 *      register.
 */
PRIVATE void g_return P2 (const EXPR *, stmtexp, TYP *, tp)
{
}

/*
 *    Generate the code required at the end of a function to
 *      restore any used registers and the return instruction itself.
 */
PRIVATE void g_epilogue P0 (void)
{
}

/*
 * allocate will allocate registers for the expressions that have a high
 * enough desirability.
 */
PRIVATE void g_allocate P1 (CSE *, olist)
{
}


/*
 *    Go through the common sub-expression tree and check to see if
 *      any registers must be loaded with a value.
 */

PRIVATE void g_preload P1 (CSE *, olist)
{
    CSE    *csp;
    EXPR   *ep;
    ADDRESS *ap, *ap2;

    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	if (csp->reg != NO_REG) {	/* see if preload needed */
	    ep = csp->exp;
	    if (!is_lvalue (ep) || (ep->v.p[0]->v.i > 0L)) {
		initstack ();
		ap = g_expr (ep, F_ALL);
		ap2 = mk_reg (csp->reg);
		g_code (op_move, (ILEN) ep->etp->size, ap, ap2);
		freeop (ap);
	    }
	}
    }
}

PRIVATE void g_flush P1 (SYM *, sp)
{
    put_literals ();
    if (sp) {
	put_cseg (alignment_of_type (typeof (sp)));
	put_name (sp);
    }
    flush_peep (peep_option);
}

PRIVATE void g_auto_align P0 (void)
{
    SIZE    default_alignment = AL_DEFAULT;

    if (lc_auto_max % default_alignment != 0L) {
	lc_auto_max += default_alignment - (lc_auto_max % default_alignment);
    }
}

PRIVATE BOOL g_is_bigendian P0 (void)
{
    return TRUE;
}

PRIVATE BOOL g_is_ascending_stack P0 (void)
{
    return FALSE;
}

/*
 *    This routine does any code generator specific transformations
 *      on the expression tree.
 *
 *      For example it can replace operator nodes with calls to runtime
 *      routines.   This allows the global optimiser to perform optimisations
 *      on such calls which wouldn't be possible if the calls were
 *      generated in the code generator routines themselves.
 */

PRIVATE EXPR *g_transform P1 (EXPR *, ep)
{
    TYP    *tp;

    if (ep == NIL_EXPR) {
	return ep;
    }
    tp = ep->etp;
    switch (ep->nodetype) {
    case en_fcon:
    case en_icon:
    case en_nacon:
    case en_labcon:
    case en_autocon:
    case en_sym:
    case en_register:
    case en_str:
	break;
    case en_add:
    case en_sub:
    case en_div:
    case en_mod:
    case en_mul:
    case en_asadd:
    case en_assub:
    case en_asdiv:
    case en_asmod:
    case en_asmul:
    case en_ainc:
    case en_adec:
    case en_cast:
    case en_eq:
    case en_ne:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
    case en_lsh:
    case en_rsh:
    case en_and:
    case en_or:
    case en_xor:
    case en_land:
    case en_lor:
    case en_cond:
    case en_comma:
    case en_list:
    case en_asor:
    case en_asxor:
    case en_asand:
    case en_aslsh:
    case en_asrsh:
    case en_fcall:
    case en_call:
    case en_usercall:
    case en_assign:
    case en_asmul2:
    case en_asdiv2:
	ep->v.p[1] = g_transform (ep->v.p[1]);
	/*FALLTHRU */
    case en_ref:
    case en_not:
    case en_test:
    case en_compl:
    case en_deref:
    case en_uminus:
	ep->v.p[0] = g_transform (ep->v.p[0]);
	return ep;
    case en_fieldref:
	ep->v.bit.p = g_transform (ep->v.bit.p);
	return ep;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return ep;
}

/*
 *   This routine is called after the global optimizer has done it's
 *   work re-organizing the expression tree.  This allows a code
 *   generator to make code generator specific changes to the expression
 *   tree which will result in better code generation.
 */

PRIVATE EXPR *g_order P1 (EXPR *, ep)
{
    return ep;
}

PRIVATE void g_initialize P0 (void)
{
}

/*
 *   This routine is called when the compiler is closing down.
 */
PRIVATE void g_terminate P0 (void)
{
}

/*
 *   Returns the current register usage.
 */
PRIVATE REGUSAGE *g_regusage P1 (TYP *, tp)
{
    return reg_usage;
}

#ifdef MULTIPLE_PROCESSORS
struct genfuncs powerpc_funcs = {
    g_expression,
    g_jtrue,
    g_jfalse,
    g_stack,
    g_switch_table,
    g_switch_compare,
    g_entry,
    g_return,
    g_epilogue,
    g_label,
    g_branch,
#ifdef DEBUGOPT
    g_line,
#endif				/*DEBUGOPT */
    g_allocate,
    g_preload,
    g_flush,
    g_auto_align,
    g_is_bigendian,
    g_is_ascending_stack,
    g_order,
    g_transform,
    g_initialize,
    g_terminate,
    g_regusage,
    &alignments_ppc[0]
};

#undef GEN_MODULE
#endif /* MULTIPLE_PROCESSORS */
#endif /* POWERPC */
