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

#include "config.h"

#ifdef ARM

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genarm.h"
#include "outproto.h"

/*********************************************** Static Function Definitions */

static CODE *find_label P_ ((LABEL));
static BOOL uses_label P_ ((ADDRESS *, LABEL));
static int label_references P_ ((CODE *));
static void add_peep P_ ((CODE *));
static void peep_uctran P_ ((CODE *));
static void opt3 P_ ((unsigned));

/********************************************************** Static Variables */

static CODE *peep_head = NIL_CODE;
static CODE *next_ip;
static int changes;

#if 0
static CONDITION reverse_cc[] = {
    cc_nv,			/* cc_al */
    cc_cs,			/* cc_cc */
    cc_cc,			/* cc_cs */
    cc_ne,			/* cc_eq */
    cc_lt,			/* cc_ge */
    cc_le,			/* cc_gt */
    cc_ls,			/* cc_hi */
    cc_gt,			/* cc_le */
    cc_hi,			/* cc_ls */
    cc_ge,			/* cc_lt */
    cc_pl,			/* cc_mi */
    cc_eq,			/* cc_ne */
    cc_al,			/* cc_nv */
    cc_mi,			/* cc_pl */
    cc_vs,			/* cc_vc */
    cc_vc			/* cc_vs */
};

#endif

/*****************************************************************************/

/*
 * generate a code sequence into the peep list.
 */
void g_code P5 (OPCODE, op, CONDITION, cc, ADDRESS *, ap1, ADDRESS *, ap2,
		ADDRESS *, ap3)
{
    CODE   *ip;
    ip = (CODE *) xalloc (sizeof (CODE));

    ip->opcode = op;
    ip->cc = cc;
    ip->oper1 = ap1;
    ip->oper2 = ap2;
    ip->oper3 = ap3;
    add_peep (ip);
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
	    put_label (ip->oper1->offset->v.l);
	} else {
	    put_code (ip);
	}
    }
    peep_head = NIL_CODE;
    for (sw = swtables; sw; sw = sw->next) {
	put_kseg (alignment_of_type (tp_pointer));
	put_label (sw->tablab);
	ep2 = mk_lcon (UNDEF_LABEL);
#ifdef RELOC_BUG
	/* generate the switch jump table as a series of 4-byte addresses */
	for (i = 0; i < sw->numlabs; i++) {
	    ep2->v.l = sw->labels[i];
	    put_pointer (ep2);
	}
#else
	/* generate the switch jump table as a series of 2-byte offsets
	 * This limits the amount of code the can be generated in a
	 * function to less then 32K.  I believe that this is a reasonable
	 * restriction.
	 */
	{
	    EXPR   *ep, *ep1;

	    ep1 = mk_lcon (sw->beglab);
	    ep = mk_node (en_sub, ep2, ep1, tp_void);
	    for (i = (LABEL) 0; i < sw->numlabs; i++) {
		ep2->v.l = sw->labels[i];
		put_short (ep);
	    }
	}
#endif /* RELOC_BUG */
    }
    swtables = NIL_SWITCH;
}

/*
 * delete an instruction referenced by ip
 */
static void peep_delete P1 (CODE *, ip)
{
    if (ip == NIL_CODE) {
	FATAL ((__FILE__, "peep_delete", ""));
    }
    if (ip->back == NIL_CODE) {
	peep_head = ip->fwd;
	if (ip->fwd) {
	    ip->fwd->back = NIL_CODE;
	}
	next_ip = ip->fwd;
    } else {
	if ((ip->back->fwd = ip->fwd) != NIL_CODE) {
	    ip->fwd->back = ip->back;
	}
	next_ip = ip->back;
    }
    changes++;
}

/*
 * Returns false if the <ea> does is not a label or else isn't equal to label
 */
static BOOL uses_label P2 (ADDRESS *, ap, LABEL, label)
{
    return (ap != NIL_ADDRESS &&
	    ap->mode == am_direct &&
	    ap->offset->nodetype == en_labcon && ap->offset->v.l == label);
}

/*
 * find the node which contains the label 'lab'
 */
static CODE *find_label P1 (LABEL, lab)
{
    register CODE *ip;

    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	if (ip->opcode == op_label && ip->oper1->offset->v.l == lab) {
	    return ip;
	}
    }
    /* we should have found it */
    return NIL_CODE;
}

/*
 * counts the number of times that a label node is referenced
 */
static int label_references P1 (CODE *, ip)
{
    CODE   *target;
    SWITCH *sw;
    int     count = 0;
    LABEL   i, lab = ip->oper1->offset->v.l;

    for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	if ((target != ip) &&
	    (uses_label (target->oper1, lab) ||
	     uses_label (target->oper2, lab) ||
	     uses_label (target->oper3, lab))) count++;
    }
    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	for (i = (LABEL) 0; i < sw->numlabs; i++) {
	    if (sw->labels[i] == lab) {
		count++;
	    }
	}
    }
    return count;
}

/*
 * peephole optimization for unconditional transfers. deletes instructions
 * which have no path. applies to bra, jmp, and rts instructions.
 */
static void peep_uctran P1 (CODE *, ip)
{
    while (ip->fwd != NIL_CODE && ip->fwd->opcode != op_label)
	peep_delete (ip->fwd);
}

/*
 * if a label is followed by a branch to another label, the
 * branch statement can be deleted when the label is moved
 */
static void peep_label P1 (CODE *, ip)
{
    CODE   *prev, *next, *target;
    SWITCH *sw;
    LABEL   i, lab, label;

    if ((next = ip->fwd) == NIL_CODE) {
	return;
    }
    if (!optimize_option) {
	return;
    }
    lab = ip->oper1->offset->v.l;
    switch (next->opcode) {
    case op_label:
	/* if a label is followed by a label then common them up */
	label = next->oper1->offset->v.l;
	for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	    if (uses_label (target->oper1, label)) {
		target->oper1->offset->v.l = lab;
	    }
	    if (uses_label (target->oper2, label)) {
		target->oper2->offset->v.l = lab;
	    }
	    if (uses_label (target->oper3, label)) {
		target->oper3->offset->v.l = lab;
	    }
	}
	for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	    if (sw->beglab == label) {
		sw->beglab = lab;
	    }
	    for (i = (LABEL) 0; i < sw->numlabs; i++) {
		if (sw->labels[i] == label) {
		    sw->labels[i] = lab;
		}
	    }
	}
	peep_delete (next);
	break;

    case op_b:
	if (next->cc == cc_al) {
	    prev = ip->back;
	    /*
	     * To make this fast, assume that the label number is really
	     * next->oper1->offset->v.l
	     */
	    label = next->oper1->offset->v.l;
	    if (label == lab) {
		return;
	    }
	    target = find_label (label);
	    if (target == NIL_CODE) {
		message (MSG_PEEPLABEL);
		return;
	    }
	    /* move label */
	    if (target->fwd == ip) {
		return;
	    }
	    peep_delete (ip);
	    ip->fwd = target->fwd;
	    ip->back = target;
	    target->fwd = ip;
	    if (ip->fwd != NIL_CODE) {
		ip->fwd->back = ip;
	    }
	    /* possibly remove branches */
	    /* in fact, prev is always != 0 if peep_delete has succeeded */
	    if (prev != NIL_CODE) {
		if (prev->opcode == op_b && prev->cc == cc_al) {
		    peep_uctran (prev);
		}
	    }
	    break;
	}
	/*FALLTHRU */
    default:
	/* check that there are still references to this label */
	if (label_references (ip) == 0) {
	    peep_delete (ip);
	}
	break;
    }
}

/* delete branches to the following statement */
static void peep_bra P1 (CODE *, ip)
{
    CODE   *p;
    LABEL   label = ip->oper1->offset->v.l;

    /* delete branches to the following statement */
    for (p = ip->fwd; p != NIL_CODE && p->opcode == op_label; p = p->fwd) {
	if (p->oper1->offset->v.l == label) {
	    peep_delete (ip);
	    return;
	}
    }
#if 0
    p = ip->fwd;
    if (ip->cc == reverse_cc[p->cc]) {
	if (p->opcode == op_b) {
	    p->cc = cc_al;
	} else {
	    /*
	     * switch the branch with the following instruction
	     */
	    ip->back->fwd = p;
	    p->fwd->back = ip;
	    ip->fwd = p->fwd;
	    p->back = ip->back;
	    p->fwd = ip;
	    ip->back = p;
	    changes++;
	}
    }
#endif
}

static void peep_mov P1 (CODE *, ip)
{
    if (ip->oper1->mode == am_reg &&
	ip->oper1->preg == R14 &&
	ip->oper2->mode == am_reg && ip->oper2->preg == R15) {
	peep_uctran (ip);
    }
}

#if 0
/*
 * The ARM allows instructions to be executed conditionally.  If the
 * distance between a branch and label is short enough then remove the
 * branch and make the intervening instruction to be executed conditionally.
 */
static void peep_condition P1 (CODE *, ip)
{
    CODE   *next;

    for (next = ip->fwd; next != NIL_CODE; next = next->fwd) {
	if (next->opcode == op_label) {
	    break;
	}
	if (next->cc == reverse_cc[ip->cc]) {
	    continue;
	}
	if (next->cc == cc_al) {
	    next->cc = reverse_cc[ip->cc];
	    continue;
	}
	break;
    }
}
#endif

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
    do {
	changes = 0;
	next_ip = peep_head;
	while (next_ip != NIL_CODE) {
	    ip = next_ip;
	    next_ip = ip->fwd;
	    switch (ip->opcode) {
	    case op_label:
		peep_label (ip);
		break;
	    case op_b:
#if 0
		peep_condition (ip);
#endif
		peep_bra (ip);
		break;
	    case op_mov:
		peep_mov (ip);
		break;
	    default:
		break;
	    }
	}
#ifdef VERBOSE
	if (verbose_option && changes) {
	    message (MSG_PEEPCHANGES, changes);
	}
#endif /* VERBOSE */
    } while (changes);
}
#endif /* ARM */
