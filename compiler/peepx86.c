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

#ifdef INTEL

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genx86.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define DEST_MODIFY ((unsigned char)1)
#define DEST_OVERWRITE	((unsigned char)2)
#define DEST_ALTERED	(DEST_MODIFY | DEST_OVERWRITE)

/*
 *	  The next two #define statements are to make the
 *		code in the branch optimisation clearer.  Tests
 *		have shown that the in-line size cost is about
 *		the same as making them functions so we keep
 *		them in-line for speed.
 */

#define is_same_instruction(ip1,ip2) \
	((ip1 != NIL_CODE) && (ip2 != NIL_CODE) && \
	(ip1->opcode == ip2->opcode) && \
	(ip1->length == ip2->length) && \
	(is_equal_oper (ip1->oper1, ip2->oper1)) && \
	(is_equal_oper (ip1->oper2, ip2->oper2)))


/* backup over any sequence of labels to previous instruction */
#define previous_instruction(ip) \
	do { \
	ip = ip->back; \
	} while (ip != NIL_CODE && ip->opcode == op_label)

#define branch(ip)	(ip->opcode >= op_bra && ip->opcode <= op_jbe)

/*********************************************** Static Function Definitions */

static CODE *code P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));
static CODE *find_label P_ ((LABEL));
static BOOL is_label_used P_ ((ADDRESS *, LABEL));
static BOOL is_address_used P_ ((ADDRESS *, ADDRESS *));
static BOOL is_dest_overwritten P_ ((ADDRESS *, CODE *));
static void add_peep P_ ((CODE *));
static void peep_delete P_ ((CODE *));
static void peep_test P_ ((CODE *));
static void peep_mov P_ ((CODE *));
static void peep_cmp P_ ((CODE *));
static void peep_lea P_ ((CODE *));
static void peep_xtend P_ ((CODE *));
static void peep_addsub P_ ((CODE *, IVAL));
static void peep_uctran P_ ((CODE *));
static void peep_bxx P_ ((CODE *));
static void peep_label P_ ((CODE *));
static void peep_line P_ ((CODE *));
static void check_label P_ ((CODE *, CODE *));
static void peep_bra P_ ((CODE *));
static void opt3 P_ ((unsigned));
static BOOL is_equal_oper P_ ((ADDRESS *, ADDRESS *));

/********************************************************** Static Variables */

static unsigned char op_flags[] = {
    /* op_movsbl */ DEST_OVERWRITE,
    /* op_movzbl */ DEST_OVERWRITE,
    /* op_movswl */ DEST_OVERWRITE,
    /* op_movzwl */ DEST_OVERWRITE,
    /* op_movsbw */ DEST_OVERWRITE,
    /* op_movzbw */ DEST_OVERWRITE,
    /* op_cdq */ DEST_OVERWRITE,
    /* op_cwd */ DEST_OVERWRITE,
    /* op_cbw */ DEST_OVERWRITE,
    /* op_mov */ DEST_OVERWRITE,
    /* op_xchg */ DEST_OVERWRITE,
    /* op_lea */ DEST_OVERWRITE,
    /* op_not */ DEST_MODIFY,
    /* op_neg */ DEST_MODIFY,
    /* op_add */ DEST_MODIFY,
    /* op_sub */ DEST_MODIFY,
    /* op_adc */ DEST_MODIFY,
    /* op_sbb */ DEST_MODIFY,
    /* op_imul */ DEST_MODIFY,
    /* op_idiv */ DEST_MODIFY,
    /* op_div */ DEST_MODIFY,
    /* op_and */ DEST_MODIFY,
    /* op_or */ DEST_MODIFY,
    /* op_xor */ DEST_MODIFY,
    /* op_inc */ DEST_MODIFY,
    /* op_dec */ DEST_MODIFY,
    /* op_cmp */ 0,
    /* op_push */ 0,
    /* op_pop */ DEST_OVERWRITE,
    /* op_jmp */ 0,
    /* op_loop */ 0,
    /* op_call */ 0,
    /* op_leave */ DEST_OVERWRITE,
    /* op_enter */ 0,
    /* op_ret */ 0,
    /* op_test */ 0,
    /* op_bra */ 0,
    /* op_je */ 0,
    /* op_jne */ 0,
    /* op_jl */ 0,
    /* op_jle */ 0,
    /* op_jg */ 0,
    /* op_jge */ 0,
    /* op_ja */ 0,
    /* op_jae */ 0,
    /* op_jb */ 0,
    /* op_jbe */ 0,
    /* op_rep */ 0,
    /* op_smov */ DEST_OVERWRITE,
    /* op_shl */ DEST_MODIFY,
    /* op_shr */ DEST_MODIFY,
    /* op_asl */ DEST_MODIFY,
    /* op_asr */ DEST_MODIFY,
    /* op_rol */ DEST_MODIFY,
    /* op_ror */ DEST_MODIFY,
    /* op_sahf */ DEST_MODIFY,
    /* op_sete */ DEST_OVERWRITE,
    /* op_setne */ DEST_OVERWRITE,
    /* op_setb */ DEST_OVERWRITE,
    /* op_setbe */ DEST_OVERWRITE,
    /* op_seta */ DEST_OVERWRITE,
    /* op_setae */ DEST_OVERWRITE,
    /* op_setl */ DEST_OVERWRITE,
    /* op_setle */ DEST_OVERWRITE,
    /* op_setg */ DEST_OVERWRITE,
    /* op_setge */ DEST_OVERWRITE,
    /* op_nop */ 0,
#ifdef FLOAT_IEEE
    /* op_fadd */ 0,
    /* op_faddp */ 0,
    /* op_fsub */ 0,
    /* op_fsubp */ 0,
    /* op_fdiv */ 0,
    /* op_fdivp */ 0,
    /* op_fmul */ 0,
    /* op_fmulp */ 0,
    /* op_fsubr */ 0,
    /* op_fsubrp */ 0,
    /* op_fdivr */ 0,
    /* op_fdivrp */ 0,
    /* op_fld */ 0,
    /* op_fldz */ 0,
    /* op_fst */ 0,
    /* op_fstp */ 0,
    /* op_fpop */ 0,
    /* op_fild */ 0,
    /* op_fildl */ 0,
    /* op_fistp */ 0,
    /* op_fistpl */ 0,
    /* op_ftst */ 0,
    /* op_fchs */ 0,
    /* op_fcomp */ 0,
    /* op_fcompp */ 0,
    /* op_fnstsw */ 0,
    /* op_fwait */ 0,
#endif				/* FLOAT_IEEE */
    /* op_line */ 0,
    /* op_label */ 0
};

static CODE *peep_head = NIL_CODE;
static CODE *next_ip;
static int changes;

/*****************************************************************************/

/*
 * find the end of a block of code.
 */
static CODE *block_end P1 (CODE *, ip)
{
    int     count = 0;

    while ((ip != NIL_CODE) && (ip->opcode != op_bra) &&
	   (ip->opcode != op_jmp) && (ip->opcode != op_ret)) {
	if (count == BRANCH_COUNT) {
	    return NIL_CODE;
	}
	if (branch (ip)) {
	    count++;
	}
	ip = ip->fwd;
    }
    return ip;
}

/*
 * find the node which contains the label 'lab'
 */
static CODE *find_label P1 (LABEL, lab)
{
    register CODE *ip;

    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	if (ip->opcode == op_label && ip->oper1->u.offset->v.l == lab) {
	    return ip;
	}
    }
    /* we should have found it */
    return NIL_CODE;
}

/*
 * Returns false if the <ea> does is not a label or else isn't equal to label
 */
static BOOL is_label_used P2 (ADDRESS *, ap, LABEL, label)
{
    return (ap != NIL_ADDRESS &&
	    ap->mode == am_direct &&
	    ap->u.offset->nodetype == en_labcon &&
	    ap->u.offset->v.l == label);
}

/*
 * Returns false if the <ea> of ap1 is not used in the <ea> of ap2,
 * otherwise it returns true.  If we arn't sure then returns true anyway.
 */
static BOOL is_address_used P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS) {
	return FALSE;
    }
    switch (ap1->mode) {
    case am_dreg:
    case am_areg:
	switch (ap2->mode) {
	case am_dreg:
	case am_areg:
	case am_ind:
	case am_indx:
	case am_indx2:
	    return ap2->preg == ap1->preg;
	case am_direct:
	case am_immed:
	case am_line:
	    return FALSE;
	default:
	    break;
	}
	break;
    case am_immed:
	return FALSE;
    default:
	break;
    }
    return TRUE;
}

/*
 * Checks to see if the addressing mode ap is overwritten with a new
 * value before the value is used.
 */
static BOOL is_dest_overwritten P2 (ADDRESS *, ap, CODE *, ip)
{
    CODE   *ip2, *ip3;

    if (ap->mode != am_dreg && ap->mode != am_areg) {
	return FALSE;
    }
    for (ip3 = NIL_CODE, ip2 = ip->fwd; ip2 != NIL_CODE;) {
	switch (ip2->opcode) {
	case op_leave:
	    peep_delete (ip);
	    return TRUE;
	case op_lea:
	    if (ip2->oper2->preg == STACKPTR) {
		peep_delete (ip);
		return TRUE;
	    };
	    goto def;
	case op_ret:
	case op_je:		/* use of SP unknown */
	case op_jne:
	case op_jl:
	case op_jle:
	case op_jg:
	case op_jge:
	case op_ja:
	case op_jae:
	case op_jb:
	case op_jbe:
	case op_jmp:
	    return FALSE;
	case op_call:		/* implicit use of SP */
	case op_pop:
	case op_push:
	    if (ap->preg == STACKPTR) {
		return FALSE;
	    }
	    /*FALLTHRU */
	default:
	  def:
	    if (is_address_used (ap, ip2->oper1)
		|| is_address_used (ap, ip2->oper2)) {
		return FALSE;
	    }
	    /*FALLTHRU */
	case op_label:
	    ip2 = ip2->fwd;
	    break;
	case op_bra:
	    if (ip2 == ip3)	/* check to see if we are looping */
		return FALSE;
	    ip3 = ip2;		/* remember that we have been here */
	    ip2 = find_label (ip2->oper1->u.offset->v.l);
	    break;
	}
    }
    return FALSE;
}

/*
 * compare two address nodes and return true if they are equivalent.
 */
BOOL is_equal_address P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS && ap2 == NIL_ADDRESS) {
	return TRUE;
    }
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS) {
	return FALSE;
    }
    if (ap1->mode != ap2->mode) {
	return FALSE;
    }
    switch (ap1->mode) {
    case am_dreg:
    case am_areg:
    case am_freg:
    case am_ind:
	return (ap1->preg == ap2->preg);
    case am_indx:
	return (ap1->preg == ap2->preg &&
		is_equalnode (ap1->u.offset, ap2->u.offset));
    case am_indx2:
	return (ap1->preg == ap2->preg &&
		ap1->sreg == ap2->sreg &&
		is_equalnode (ap1->u.offset, ap2->u.offset));
    case am_mreg:
	return (ap1->preg == ap2->preg && ap1->sreg == ap2->sreg);
    case am_direct:
	return is_equalnode (ap1->u.offset, ap2->u.offset);
    default:
	break;
    }
    return FALSE;
}

static BOOL is_equal_oper P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS && ap2 == NIL_ADDRESS) {
	return TRUE;
    }
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS) {
	return FALSE;
    }
    if (ap1->mode != ap2->mode) {
	return FALSE;
    }
    switch (ap1->mode) {
    case am_dreg:
    case am_areg:
    case am_ind:
	return ap1->preg == ap2->preg;
    case am_freg:
	return TRUE;
    case am_indx:
	return ap1->preg == ap2->preg
	    && is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_direct:
	return is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_immed:
	return ap1->u.offset->v.i == ap2->u.offset->v.i;
    default:
	break;
    }
    return FALSE;
}

/*
 * Determine whether a move is redundant ... this is done by looking back
 * along the code list (following all branches to labels) to determine
 * whether the destination already contains the necessary result.
 */
static BOOL was_move_redundant P3 (CODE *, ip, CODE *, ip2, BOOL, memory)
{
    BOOL    altered, overwritten;
    LABEL   label;
    SWITCH *sw;

    for (ip2 = ip2->back; ip2 != NIL_CODE; ip2 = ip2->back) {
	switch (ip2->opcode) {
	case op_label:
	    label = ip2->oper1->u.offset->v.l;

	    /* first check code before the label */
	    if (!was_move_redundant (ip, ip2, memory)) {
		return FALSE;
	    }
	    /* ... and then check all branches to this label */
	    for (ip2 = peep_head; ip2 != NIL_CODE; ip2 = ip2->fwd) {
		if (branch (ip2)) {
		    if (is_label_used (ip2->oper1, label)) {
			OPCODE  op = ip2->opcode;

			ip2->opcode = op_nop;
			if (!was_move_redundant (ip, ip2, memory)) {
			    ip2->opcode = op;
			    return FALSE;
			}
			ip2->opcode = op;
		    }
		}
	    }

	    /* but if it is via a jump table we cannot determine it */
	    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
		LABEL   lab;

		for (lab = (LABEL) 0; lab < sw->numlabs; lab++) {
		    if (sw->labels[lab] == label) {
			return FALSE;
		    }
		}
	    }
	    return TRUE;

	case op_ret:
	case op_jmp:
	case op_bra:
	    /* should have at least hit a label before here! */
	case op_nop:
	    return TRUE;

	case op_call:
#ifdef ASM
	case op_asm:
#endif /* ASM */
	    return FALSE;

	case op_line:
	    break;

	default:
	    if (is_same_instruction (ip, ip2)) {
		return TRUE;
	    }
	    altered = (BOOL) (op_flags[ip2->opcode] & DEST_ALTERED);
	    overwritten = (BOOL) (op_flags[ip2->opcode] & DEST_OVERWRITE);
	    if (ip2->oper2) {
		/* two operand instruction */
		if (is_equal_address (ip->oper1, ip2->oper2)) {
		    if (overwritten && (ip->length <= ip2->length) &&
			is_equal_address (ip->oper2, ip2->oper1))
			return TRUE;
		    if (altered) {
			return FALSE;
		    }
		}
		if (altered && is_address_used (ip2->oper2, ip->oper1)) {
		    return FALSE;
		}
		switch (ip2->oper2->mode) {
		case am_dreg:
		case am_areg:
		case am_mreg:
		case am_direct:
		case am_immed:
		    if (altered && (is_equal_address (ip->oper2, ip2->oper2)
				    || is_equal_address (ip->oper1,
							 ip2->oper2))) {
			return FALSE;
		    }
		    break;
		case am_ind:
		case am_indx:
		    if (altered && memory) {
			return FALSE;
		    }
		    break;
		default:
		    break;
		}
	    }
	    if (ip2->oper1) {
		switch (ip2->oper1->mode) {
		case am_dreg:
		case am_areg:
		case am_mreg:
		case am_direct:
		case am_immed:
		    if (ip2->oper2 != NIL_ADDRESS) {
			break;
		    }
		    /* one operand instruction */
		    if (altered && (is_equal_address (ip->oper2, ip2->oper1)
				    || is_equal_address (ip->oper1,
							 ip2->oper1))) return
			    FALSE;
		    break;
		case am_ind:
		case am_indx:
		    if (ip2->oper2 != NIL_ADDRESS) {
			break;
		    }
		    /* one operand instruction */
		    if (altered && memory) {
			return FALSE;
		    }
		    break;
		default:
		    break;
		}
	    }
	    break;
	}
    }
    return FALSE;
}

/* ensure we have a label to branch to (create if needed) */
static void check_label P2 (CODE *, ip, CODE *, target)
{
    if (target->fwd->opcode == op_label) {
	ip->oper1->u.offset->v.l = target->fwd->oper1->u.offset->v.l;
    } else {
	CODE   *p;

	p = code (op_label, IL0, mk_label (nextlabel), NIL_ADDRESS);
	p->fwd = target->fwd;
	p->back = target;
	target->fwd = p->fwd->back = p;
	ip->oper1->u.offset->v.l = nextlabel++;
    }
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
 * generate a code sequence into the peep list.
 */
void g_code P4 (OPCODE, op, ILEN, len, ADDRESS *, ap1, ADDRESS *, ap2)
{
    add_peep (code (op, len, ap1, ap2));
}

#ifdef FLOAT_IEEE
/*
 * generate a floating point code sequence into the peep list.
 */
void g_fcode P4 (OPCODE, op, ILEN, len, ADDRESS *, ap1, ADDRESS *, ap2)
{
    if (len) {
	len++;
    }
    g_code (op, len, ap1, ap2);
}
#endif /* FLOAT_IEEE */

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
    EXPR   *ep;
    LABEL   i;

    /*
     * perform peephole optimizations
     */
    opt3 (level);
    /*
     * generate assembler output
     */
    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	if (ip->opcode == op_label) {
	    put_label (ip->oper1->u.offset->v.l);
	} else {
	    put_code (ip);
	}
    }
    peep_head = NIL_CODE;
    ep = mk_lcon (UNDEF_LABEL);
    for (sw = swtables; sw; sw = sw->next) {
	put_kseg (alignment_of_type (tp_pointer));
	put_label (sw->tablab);
	for (i = 0; i < sw->numlabs; i++) {
	    ep->v.l = sw->labels[i];
	    put_pointer (ep);
	}
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
 * deletes lea (Rn),Rn
 */
static void peep_lea P1 (CODE *, ip)
{
    if (ip->oper1->mode == am_ind && ip->oper1->preg == ip->oper2->preg) {
	peep_delete (ip);
	return;
    }
}


/*
 * optimizes and instructions
 */
static void peep_and P1 (CODE *, ip)
{
    CODE   *prev;
    IVAL    val;

    if ((ip->oper1->mode != am_immed) ||
	(ip->oper1->u.offset->nodetype != en_icon)) {
	return;
    }
    val = ip->oper1->u.offset->v.i;
    if (val == -1L) {
	/*
	 *      AND #-1, <ea>
	 *
	 *      only sets flags, which the code generator does not know
	 */
	peep_delete (ip);
	return;
    }
    prev = ip->back;
    if (prev == NIL_CODE) {
	return;
    }
    if ((prev->opcode == op_and) &&
	(prev->oper1->mode == am_immed) &&
	is_icon (prev->oper1->u.offset) &&
	is_equal_address (ip->oper2, prev->oper2)) {
	/*
	 *       AND #M, <ea>     =>      AND #(M&N), <ea>
	 *       AND #N, <ea>
	 */
	prev->oper1->u.offset->v.i &= val;
	if (prev->length < ip->length) {
	    prev->length = ip->length;
	}
	peep_delete (ip);
	return;
    }
}

static void peep_test P1 (CODE *, ip)
{
    CODE   *prev = ip->back;

    if (prev == NULL) {
	return;
    }
    if (!is_equal_address (ip->oper1, ip->oper2)) {
	return;
    }
    /* instruction is just setting the flags */
    switch (prev->opcode) {
    case op_add:
    case op_sub:
    case op_and:
    case op_or:
	if (is_equal_address (ip->oper2, prev->oper2)) {
	    peep_delete (ip);
	}
	break;
    default:
	break;
    }
}

static void peep_mov P1 (CODE *, ip)
{
    CODE   *prev, *next;
    BOOL    memory, altered;

    if (is_equal_address (ip->oper1, ip->oper2)) {
	/*
	 * delete mov src,src
	 */
	peep_delete (ip);
	return;
    }
    memory = FALSE;
    switch (ip->oper1->mode) {
    case am_ind:
    case am_indx:
	memory = TRUE;
	if (ip->oper2->mode != am_dreg && ip->oper2->mode != am_areg) {
	    break;
	}
	if (ip->oper1->preg == ip->oper2->preg) {
	    break;
	}
	/*FALLTHRU */
    case am_dreg:
    case am_areg:
	/*
	 * eliminate redundant moves into a register when intervening
	 * instructions only involve registers which cannot affect
	 * the register.
	 */
	if (ip->oper2->mode != am_dreg && ip->oper2->mode != am_areg) {
	    break;
	}
	for (next = ip->fwd; next != NIL_CODE; next = next->fwd) {
	    switch (next->opcode) {
	    case op_label:
	    case op_bra:
	    case op_call:
	    case op_jmp:
	    case op_ret:
	    case op_leave:
		// Section added because it optimizes use of AX, DX:AX, and EDX:EAX across mul and div. -WSF
		case op_imul: case op_div: case op_idiv:
		//
		return;
	    default:
		break;
	    }
	    if (is_same_instruction (ip, next)) {
		peep_delete (next);
		return;
	    }
	    altered = (BOOL) (op_flags[next->opcode] & DEST_ALTERED);
	    if (next->oper2) {
		/* two operand instruction */
		if (altered && is_equal_oper (ip->oper1, next->oper2)) {
		    return;
		}
		switch (next->oper2->mode) {
		case am_dreg:
		case am_areg:
		    if (altered && ((ip->oper2->preg == next->oper2->preg) ||
				    (ip->oper1->preg == next->oper2->preg)))
			return;
		    break;
		case am_ind:
		case am_indx:
		    if (altered && memory) {
			return;
		    }
		    break;
		default:
		    break;
		}
	    }
	    if (next->oper1) {
		switch (next->oper1->mode) {
		case am_dreg:
		case am_areg:
		    if (next->oper2 != NIL_ADDRESS) {
			break;
		    }
		    /* one operand instruction */
		    if (altered && ((ip->oper2->preg == next->oper1->preg) ||
				    (ip->oper1->preg == next->oper1->preg)))
			return;
		    break;
		case am_ind:
		case am_indx:
		    if (next->oper2 != NIL_ADDRESS) {
			break;
		    }
		    /* one operand instruction */
		    if (altered && memory) {
			return;
		    }
		    break;
		default:
		    break;
		}
	    }
	}
	break;
    case am_immed:
	if ((ip->oper2->mode == am_dreg || ip->oper2->mode == am_areg) &&
	    is_icon (ip->oper1->u.offset) && ip->oper1->u.offset->v.i == 0L) {
	    /*
	     * change mov #0, reg   =>        xor reg, reg
	     */
	    ip->oper1 = ip->oper2;
	    ip->opcode = op_xor;
	    return;
	}
	break;
    default:
	if ((prev = ip->back) == NIL_CODE) {
	    return;
	}
	/*
	 * think about
	 *
	 * movl (%eax),eax                 I will make is_equal_address very restrictive!
	 * movl eax,(%eax)
	 *
	 */
	if (prev->opcode == op_mov && prev->length == ip->length &&
	    is_equal_address (prev->oper1, ip->oper2) &&
	    is_equal_address (prev->oper2, ip->oper1)) {
	    /*
	     * change mov src,dest  =>        mov src, dest
	     *                mov dest,src
	     */
	    peep_delete (ip);
	    return;
	}
    }
}

/*
 * changes cmp $0,reg to test reg,reg if followed by je, jne, jg, jge, jl, jle
 */
static void peep_cmp P1 (CODE *, ip)
{
    CODE   *next;

    if (ip->oper1->mode == am_immed &&
	is_icon (ip->oper1->u.offset) &&
	ip->oper1->u.offset->v.i == 0L &&
	(ip->oper2->mode == am_dreg || ip->oper2->mode == am_areg) &&
	(next = ip->fwd) != NIL_CODE) {
	switch (next->opcode) {
	case op_je:
	case op_jne:
	case op_jg:
	case op_jge:
	case op_jl:
	case op_jle:
	    ip->opcode = op_test;
	    ip->oper1 = ip->oper2;
	    break;
	default:
	    break;
	}
    }
}

/*
 * changes things like
 *
 * movw src,%ax
 * movzwl %ax,%eax
 *
 * to
 *
 * movzwl src,%eax
 *
 * DO NOT DESTRUCT
 * movw %di,%ax
 * movzbl %al,%eax
 * since %di,%si have no byte-components.
 */
static void peep_xtend P1 (CODE *, ip)
{
    CODE   *prev = ip->back;
    ILEN    size;

    if (prev != NIL_CODE && prev->opcode == op_mov &&
	is_equal_address (prev->oper2, ip->oper1) &&
	is_equal_address (ip->oper1, ip->oper2)) {
	if ((prev->oper1->mode == am_dreg || prev->oper1->mode == am_areg) &&
	    (prev->oper1->preg == ESI || prev->oper1->preg == EDI))
	    return;
	ip->oper1 = prev->oper1;
	peep_delete (prev);
    }
    /*
     * the code generator, or this procedure, may generate code like
     *
     * movsbl $const, %reg
     *
     * this is not a legal instruction.
     * we will change it to a mov instruction here
     */
    if (ip->oper1->mode != am_immed) {
	return;
    }
    if (ip->opcode == op_movsbw || ip->opcode == op_movzbw) {
	size = IL2;
    } else {
	size = IL4;
    }
    ip->length = size;
    ip->opcode = op_mov;
}

/*
 * changes:
 *		add #1	=> inc
 *		add #-1 => dec
 *		sub #1	=> dec
 *		sub #-1 => inc
 */
static void peep_addsub P2 (CODE *, ip, IVAL, flag)
{
    if (ip->oper1->mode != am_immed
	|| ip->oper1->u.offset->nodetype != en_icon) {
	return;
    }
    if (ip->oper2->preg == STACKPTR && is_dest_overwritten (ip->oper2, ip)) {
	peep_delete (ip);
	return;
    }
    if (ip->fwd && (ip->fwd->opcode == op_adc || ip->fwd->opcode == op_sbb)) {
	return;
    }
    flag *= ip->oper1->u.offset->v.i;
    switch (flag) {
    case 1l:
	ip->opcode = op_inc;
	ip->oper1 = ip->oper2;
	ip->oper2 = NIL_ADDRESS;
	break;
    case -1l:
	ip->opcode = op_dec;
	ip->oper1 = ip->oper2;
	ip->oper2 = NIL_ADDRESS;
	break;
    case -2l:
	if (ip->length == IL2 &&
	    (ip->oper2->mode == am_dreg || ip->oper2->mode == am_areg) &&
	    ip->oper2->preg == ESP) {

	    ip->opcode = op_push;
	    ip->oper1 = mk_reg (ECX);
	    ip->oper2 = NIL_ADDRESS;
	}
	break;
    case -4l:
	if (ip->length == IL4 &&
	    (ip->oper2->mode == am_dreg || ip->oper2->mode == am_areg) &&
	    ip->oper2->preg == ESP) {

	    ip->opcode = op_push;
	    ip->oper1 = mk_reg (ECX);
	    ip->oper2 = NIL_ADDRESS;
	}
	break;
    default:
	break;
    }
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
 * optimizes conditional branch over a bra.
 */
static void peep_bxx P1 (CODE *, ip)
{
    static OPCODE revcond[] = { op_jne, op_je,
	op_jge, op_jg, op_jle, op_jl,
	op_jbe, op_jb, op_jae, op_ja
    };
    CODE   *next = ip->fwd;

    if (next == NIL_CODE) {
	return;
    }
    if (next->opcode == op_bra) {
	/* peep_uctran increases the 'hit' probability */
	peep_uctran (next);
	next = next->fwd;
	if (next == NIL_CODE) {
	    return;
	}
	if (next->opcode == op_label &&
	    ip->oper1->u.offset->v.l == next->oper1->u.offset->v.l) {
	    ip->fwd->opcode = revcond[(int) ip->opcode - (int) op_je];
	    peep_delete (ip);
	}
    }
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
    lab = ip->oper1->u.offset->v.l;
    switch (next->opcode) {
    case op_label:
	/* if a label is followed by a label then common them up */
	label = next->oper1->u.offset->v.l;
	for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	    if (is_label_used (target->oper1, label)) {
		target->oper1->u.offset->v.l = lab;
	    }
	    if (is_label_used (target->oper2, label)) {
		target->oper2->u.offset->v.l = lab;
	    }
	}
	for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	    for (i = (LABEL) 0; i < sw->numlabs; i++) {
		if (sw->labels[i] == label) {
		    sw->labels[i] = ip->oper1->u.offset->v.l;
		    changes++;
		}
	    }
	}
	peep_delete (next);
	break;

    case op_bra:
	prev = ip->back;
	/*
	 * To make this fast, assume that the label number is really
	 * next->oper1->u.offset->v.l
	 */
	label = next->oper1->u.offset->v.l;
	if (label == ip->oper1->u.offset->v.l) {
	    return;
	}
	target = peep_head;
	/*
	 * look for the label
	 */
	while (target != NIL_CODE) {
	    if (target->opcode == op_label
		&& target->oper1->u.offset->v.l == label) break;
	    target = target->fwd;
	}
	/* we should have found it */
	if (target == NIL_CODE) {
	    message (MSG_PEEPLABEL);
	    return;
	}
	/* move label */
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
	    if (prev->opcode == op_bra || prev->opcode == op_jmp
		|| prev->opcode == op_ret)
		peep_uctran (prev);
	}
	break;
    default:
	/* check that there are still references to this label */
	label = ip->oper1->u.offset->v.l;
	for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	    if ((target != ip) &&
		(is_label_used (target->oper1, lab) ||
		 is_label_used (target->oper2, lab)))
		return;
	}
	for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	    for (i = 0; i < sw->numlabs; i++) {
		if (sw->labels[i] == label) {
		    return;
		}
	    }
	}
	peep_delete (ip);
	break;
    }
}

/* delete branches to the following statement */
static void peep_bra P1 (CODE *, ip)
{
    CODE   *p = ip->fwd;
    CODE   *target;
    LABEL   label = ip->oper1->u.offset->v.l;
    int     count;

    /* delete branches to the following statement */
    while (p != NIL_CODE && p->opcode == op_label) {
	if (p->oper1->u.offset->v.l == label) {
	    peep_delete (ip);
	    return;
	}
	p = p->fwd;
    }

    if (!optimize_option) {
	return;
    }
    target = find_label (label);
    /* we should have found it */
    if (target == NIL_CODE) {
	FATAL ((__FILE__, "peep_bra", "target == 0"));
    }
    /* Space optimisation:
     * if the code before the target of the branch is itself a branch
     * then we can move the destination block of code to eliminate the
     * branch
     */
    p = target->back;
    if (p != NIL_CODE && ((p->opcode == op_bra)
			  || (p->opcode == op_jmp)
			  || (p->opcode == op_ret))) {
	p = block_end (target);
	if (p != NIL_CODE && p != ip) {
	    if (ip->fwd) {
		ip->fwd->back = p;
	    }
	    if (p->fwd) {
		p->fwd->back = target->back;
	    }
	    target->back->fwd = p->fwd;
	    p->fwd = ip->fwd;
	    target->back = ip;
	    ip->fwd = target;
	    peep_delete (ip);
	    return;
	}
    }
    /* Space optimisation:
     * if the code before the branch instruction is the same as the
     * instruction before the label the branch can be moved back an
     * instruction.
     */
    p = ip->back;
    previous_instruction (target);
    if (p == target) {
	return;
    }
    /* now skip back over identical instruction sequences */
    while (is_same_instruction (p, target)) {
	p = p->back;
	previous_instruction (target);
	peep_delete (p->fwd);
    }
    check_label (ip, target);
    label = ip->oper1->u.offset->v.l;

    /*      Space optimisation:
     *      Optimise for the situation where two branches to the same
     *      target label have identical instruction sequences
     *      leading up to the branch.       We can instead eliminate one
     *      of these instruction sequences by branching to the other one.
     */
    for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	if ((target != ip) &&
	    (target->opcode == op_bra) &&
	    (target->oper1->u.offset->v.l == label)) {

	    CODE   *t = target;

	    p = ip->back;
	    previous_instruction (t);
	    count = 0;
	    while (is_same_instruction (p, t)) {
		p = p->back;
		previous_instruction (t);
		peep_delete (p->fwd);
		count++;
	    }
	    if (count != 0) {
		check_label (ip, t);
		break;
	    }
	}
    }
    peep_uctran (ip);
}

/* delete multiple debugging line statements */
static void peep_line P1 (CODE *, ip)
{
    CODE   *ip2;

    if (ip->fwd == NIL_CODE) {
	return;
    }
    switch (ip->fwd->opcode) {
    case op_line:
	peep_delete (ip);
	break;
    case op_label:
	/* move the line number to after the label */
	ip2 = ip->fwd;
	if (ip->back) {
	    ip->back->fwd = ip2;
	} else {
	    peep_head = ip2;
	}
	if (ip2->fwd) {
	    ip2->fwd->back = ip;
	}
	ip2->back = ip->back;
	ip->fwd = ip2->fwd;
	ip2->fwd = ip;
	ip->back = ip2;
	break;
    default:
	break;
    }
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
    do {
	changes = 0;
	if (is_peep_phase (level, PEEP_INSTRUCTION)) {
	    /*
	     *       Instruction specific optisations
	     */
	    for (next_ip = peep_head; (ip = next_ip) != NIL_CODE;
		 next_ip = ip->fwd) {
		switch (ip->opcode) {
		case op_and:
		    peep_and (ip);
		    break;
		case op_test:
		    peep_test (ip);
		    break;
		case op_movzbw:
		case op_movsbw:
		case op_movzbl:
		case op_movsbl:
		case op_movzwl:
		case op_movswl:
		    peep_xtend (ip);
		    break;
		case op_lea:
		    peep_lea (ip);
		    break;
		case op_mov:
		    peep_mov (ip);
		    break;
		case op_add:
		    peep_addsub (ip, 1l);
		    break;
		case op_sub:
		    peep_addsub (ip, -1l);
		    break;
		case op_cmp:
		    peep_cmp (ip);
		    break;
		default:
		    break;
		}
	    }
	}
	if (is_peep_phase (level, PEEP_JUMPS)) {
	    /*
	     *       Flow control optimisations.
	     */
	    for (next_ip = peep_head; (ip = next_ip) != NIL_CODE;
		 next_ip = ip->fwd) {
		switch (ip->opcode) {
		case op_je:
		case op_jne:
		case op_jg:
		case op_jge:
		case op_jle:
		case op_jl:
		case op_ja:
		case op_jae:
		case op_jbe:
		case op_jb:
		    peep_bxx (ip);
		    break;
		case op_bra:
		    peep_bra (ip);
		    break;
		case op_jmp:
		case op_ret:
		    peep_uctran (ip);
		    break;
		case op_label:
		    peep_label (ip);
		    break;
		case op_line:
		    peep_line (ip);
		    break;
		default:
		    break;
		}
	    }
	}
#ifdef VERBOSE
	if (verbose_option && changes) {
	    message (MSG_PEEPCHANGES, changes);
	}
#endif /* VERBOSE */
    } while (changes);
}
#endif /* INTEL */
