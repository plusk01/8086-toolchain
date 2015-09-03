/*
 * C compiler
 * ==========
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 *
 * History:
 *
 * 2002   Inline module developed
 */

/******************************************************************************
 *
 * This module performs the operations necessary to inline a function
 *
 *****************************************************************************/
#include "chdr.h"
#ifdef CPU_DEFINED
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

#ifdef DOINLINE

static int cost;

/*
 *   used as a paremeter to walkstmt() when no action is to be
 *   performed on the statement.
 */
static STMT *null_stmt P1 (STMT *, stmt)
{
    return stmt;
}


static EXPR * inline_node_cost P1 (EXPR *, ep)
{
    if (ep == NIL_EXPR) {
	return NIL_EXPR;
    }
    switch (ep->nodetype) {
    case en_fcon:
    case en_str:
    case en_register:
	break;
    case en_icon:
    case en_autocon:
    case en_sym:
    case en_labcon:
    case en_global:
    case en_nacon:
	break;
    case en_ref:
    case en_fieldref:
	break;
    case en_add:
    case en_sub:
	inline_node_cost (ep->v.p[1]);
	inline_node_cost (ep->v.p[0]);
	break;
    case en_ainc:
    case en_adec:
    case en_deref:
	inline_node_cost (ep->v.p[0]);
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
	inline_node_cost (ep->v.p[1]);
     /*lint -fallthrough*/
    case en_uminus:
    case en_compl:
    case en_not:
    case en_test:
    case en_cast:
	inline_node_cost (ep->v.p[0]);
	break;
    case en_fcall:
    case en_call:
    case en_usercall:
	inline_node_cost (ep->v.p[0]);
	inline_node_cost (ep->v.p[1]);
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    cost++;
    return ep;
}

static EXPR * inline_expression_cost P1 (EXPR *, ep)
{
    return walkexpr (ep, inline_node_cost);
}

/*
 *  Decend the statmement tree representing the function to be inlined and
 *  calculate the approximate "cost" of inlining the function.    This can
 *  be used to provide a machanism to decide if it is worth inlining the
 *  function.
 */
int inline_statement_cost P1 (STMT *, stmt)
{
    cost = 0;
    walkstmt (stmt, null_stmt, inline_expression_cost);
    return cost;
}

/*
 *  Save the inline statement tree into global memory so that we can keep
 *  the tree until it is required for inlining.
 */
void inline_save P2 (SYM *, sp, STMT *, stmt)
{
}

/*
 *  Perform any inline substitutions.
 */
STMT *inline_substitute P1 (STMT *, stmt)
{
    return stmt;
}

#endif /* CPUDEFINED */
#endif /* DOINLINE */
