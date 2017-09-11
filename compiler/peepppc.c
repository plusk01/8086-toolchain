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

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genppc.h"
#include "outproto.h"

/********************************************************** Static Variables */

static CODE *peep_head = NIL_CODE;

/*****************************************************************************/

/*
 * compare two address nodes and return true if they are equivalent.
 */
BOOL is_equal_address P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS && ap1 == NIL_ADDRESS) {
	return TRUE;
    }
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS) {
	return FALSE;
    }
    switch (ap1->mode) {
    default:
	break;
    }
    return FALSE;
}

static BOOL is_equal_oper P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS && ap1 == NIL_ADDRESS) {
	return TRUE;
    }
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS) {
	return FALSE;
    }
    switch (ap1->mode) {
    default:
	break;
    }
    return FALSE;
}


static CODE *code P4 (OPCODE, op, ILEN, len, ADDRESS *, ap1, ADDRESS *, ap2)
{
    CODE   *ip;
    ip = (CODE *) xalloc (sizeof (CODE));

    ip->opcode = op;
    ip->length = len;
    ip->oper1 = ap1;
    ip->oper2 = ap2;
    return ip;
}

/*
 * add the instruction pointed to by new to the peep list.
 */
static void add_peep P1 (CODE *, ip)
{
    static CODE *peep_tail;

    if (peep_head == NIL_CODE) {
	peep_head = peep_tail = ip;
	ip->fwd = NIL_CODE;
	ip->back = NIL_CODE;
    } else {
	ip->fwd = NIL_CODE;
	ip->back = peep_tail;
	peep_tail->fwd = ip;
	peep_tail = ip;
    }
}

/*
 * generate a code sequence into the peep list.
 */
void g_code P4 (OPCODE, op, ILEN, len, ADDRESS *, ap1, ADDRESS *, ap2)
{
    add_peep (code (op, len, ap1, ap2));
}

/*
 * peephole optimizer. This routine calls the instruction specific
 * optimization routines above for each instruction in the peep list.
 */
static void opt3 P1 (unsigned, level)
{
    CODE   *ip;

    if (level == PEEP_NONE) {
	return;
    }
}

/*

 * output all code and labels in the peep list.
 */
void flush_peep P1 (unsigned, level)
{
    register CODE *ip;
    SWITCH *sw;
    EXPR   *ep2;
    LABEL   i;

    opt3 (level);		/* do the peephole optimizations */
    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	if (ip->opcode == op_label) {
	    put_label (ip->oper1->u.offset->v.l);
	} else
	    put_code (ip);
    }
    peep_head = NIL_CODE;
    for (sw = swtables; sw; sw = sw->next) {
	put_kseg (alignment_of_type (tp_pointer));
	put_label (sw->tablab);
	ep2 = mk_lcon (UNDEF_LABEL);
	/* generate the switch jump table as a series of 2-byte offsets
	 * This limits the amount of code that can be generated in a
	 * function to less then 32K.  I believe that this is a reasonable
	 * restriction.
	 */
	{
	    EXPR   *ep, *ep1;

	    ep1 = mk_lcon (sw->beglab);
	    ep = mk_node (en_sub, ep2, ep1, tp_void);
	    for (i = 0; i < sw->numlabs; i++) {
		ep2->v.l = sw->labels[i];
		put_short (ep);
	    }
	}
    }
    swtables = NIL_SWITCH;
}

#endif /* POWERPC */
