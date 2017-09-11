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

#ifdef MC680X0

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "gen68k.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

typedef unsigned OPFLAG;

#define DEST_MODIFY ((OPFLAG)0000001)
#define DEST_OVERWRITE	((OPFLAG)0000002)
#define DEST_ALTERED	(DEST_MODIFY | DEST_OVERWRITE)
#define SET_FLAGS	((OPFLAG)0000004)
#define USE_FLAGS	((OPFLAG)0000010)
#define NOSET_FLAGS ((OPFLAG)0000020)
#define USES_A7 	((OPFLAG)0000040)
#define FX0 	((OPFLAG)0000100)
#define FX1 	((OPFLAG)0000200)
#define FX		(FX0|FX1)
#define FN0 	((OPFLAG)0000400)
#define FN1 	((OPFLAG)0001000)
#define FN		(FN0|FN1)
#define FZ0 	((OPFLAG)0002000)
#define FZ1 	((OPFLAG)0004000)
#define FZ		(FZ0|FZ1)
#define FV0 	((OPFLAG)0010000)
#define FV1 	((OPFLAG)0020000)
#define FV		(FV0|FV1)
#define FC0 	((OPFLAG)0040000)
#define FC1 	((OPFLAG)0100000)
#define FC		(FC0|FC1)

#define is_overwrite(op)	((op_flags[op] & DEST_OVERWRITE) != 0)
#define is_altered(op)		((op_flags[op] & DEST_ALTERED) != 0)
#define is_set_flags(op)	((op_flags[op] & SET_FLAGS) != 0)
#define is_use_flags(op)	((op_flags[op] & USE_FLAGS) != 0)
#define is_noset_flags(op)	((op_flags[op] & NOSET_FLAGS) != 0)
#ifdef PEEPFLOW
#define is_modify(op)		((op_flags[op] & DEST_MODIFY) != 0)
#define is_uses_sp(op)		((op_flags[op] & USES_A7) != 0)
#endif /* PEEPFLOW */

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

#define branch(ip)	(ip->opcode >= op_bra && ip->opcode <= op_bls)

/*********************************************** Static Function Definitions */

static CODE *block_end P_ ((CODE *));
static CODE *code P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));
static int label_references P_ ((const CODE *));
static BOOL is_label_used P_ ((const ADDRESS *, LABEL));
static BOOL is_address_used P_ ((const ADDRESS *, const ADDRESS *));
static BOOL is_dest_overwritten P_ ((ADDRESS *, const CODE *));
static BOOL is_equal_oper P_ ((const ADDRESS *, const ADDRESS *));
static void add_peep P_ ((CODE *));
static void peep_delete P_ ((const CODE *));
static void peep_pea P_ ((const CODE *));
static void peep_lea P_ ((CODE *));
static void peep_move P_ ((CODE *));
static void peep_movem P_ ((CODE *));
static void peep_add P_ ((CODE *));
static void peep_and P_ ((CODE *));
static void peep_or P_ ((const CODE *));
static void peep_clr P_ ((const CODE *));
static void peep_sub P_ ((CODE *));
static void peep_cmp P_ ((CODE *));
static void peep_tst P_ ((const CODE *));
static void peep_uctran P_ ((const CODE *));
static void peep_bxx P_ ((const CODE *));
static void peep_ext P_ ((const CODE *));
static void peep_label P_ ((CODE *));
static void check_label P_ ((const CODE *, CODE *));
static void peep_bra P_ ((CODE *));
static void peep_jmp P_ ((CODE *));
static void peep_line P_ ((CODE *));
static void opt3 P_ ((unsigned));

/********************************************************** Static Variables */

static CODE *peep_head = NIL_CODE;
static CODE *next_ip;
static int changes;

static OPFLAG op_flags[] = {
    /* op_move */ DEST_OVERWRITE | SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_moveq */ DEST_OVERWRITE | SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_movea */ DEST_OVERWRITE,
    /* op_add */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_addi */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_addq */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_adda */ DEST_MODIFY | NOSET_FLAGS,
    /* op_addx */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_sub */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_subi */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_subq */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FC,
    /* op_suba */ DEST_MODIFY | NOSET_FLAGS,
    /* op_subx */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FC,
    /* op_muls */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV | FC0,
    /* op_mulu */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV | FC0,
    /* op_divs */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV | FC0,
    /* op_divu */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV | FC0,
    /* op_and */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_or */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_eor */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_asl */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_lsr */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV0 | FC,
    /* op_jmp */ (OPFLAG) 0,
    /* op_jsr */ USES_A7 | SET_FLAGS,
    /* op_bsr */ USES_A7 | SET_FLAGS,
    /* op_movem */ DEST_OVERWRITE,
    /* op_rts */ USES_A7,
    /* op_rte */ USES_A7,
    /* op_bra */ (OPFLAG) 0,
    /* op_beq */ USE_FLAGS | FZ,
    /* op_bne */ USE_FLAGS | FZ,
    /* op_blt */ USE_FLAGS | FN | FV,
    /* op_ble */ USE_FLAGS | FN | FZ | FV,
    /* op_bgt */ USE_FLAGS | FN | FZ | FV,
    /* op_bge */ USE_FLAGS | FN | FV,
    /* op_bhi */ USE_FLAGS | FC,
    /* op_bhs */ USE_FLAGS | FZ | FC,
    /* op_blo */ USE_FLAGS | FC,
    /* op_bls */ USE_FLAGS | FZ | FC,
    /* op_tst */ SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_ext */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_extb */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_lea */ DEST_OVERWRITE,
    /* op_swap */ DEST_OVERWRITE | SET_FLAGS | FN | FZ | FV0 | FC0,
    /* op_neg */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_negx */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC, 
	/* op_not */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FV0,
    /* op_cmp */ SET_FLAGS | FN | FZ | FV | FC,
    /* op_cmpa */ SET_FLAGS | FN | FZ | FV | FC,
    /* op_clr */ DEST_OVERWRITE | SET_FLAGS | FN0 | FZ1 | FV0 | FC0,
    /* op_link */ DEST_OVERWRITE | USES_A7,
    /* op_unlk */ DEST_OVERWRITE | USES_A7,
    /* op_pea */ DEST_OVERWRITE,
    /* op_cmpi */ (OPFLAG) 0,
    /* op_dbra */ DEST_OVERWRITE,
    /* op_asr */ DEST_MODIFY | SET_FLAGS | FX | FN | FZ | FV | FC,
    /* op_rol */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC,
    /* op_ror */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC,
    /* op_roxl */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC,
    /* op_roxr */ DEST_MODIFY | SET_FLAGS | FN | FZ | FV0 | FC,
    /* op_seq */ DEST_OVERWRITE | USE_FLAGS | FZ,
    /* op_sne */ DEST_OVERWRITE | USE_FLAGS | FZ,
    /* op_slt */ DEST_OVERWRITE | USE_FLAGS | FN | FV,
    /* op_sle */ DEST_OVERWRITE | USE_FLAGS | FN | FZ | FV,
    /* op_sgt */ DEST_OVERWRITE | USE_FLAGS | FN | FZ | FV,
    /* op_sge */ DEST_OVERWRITE | USE_FLAGS | FN | FV,
    /* op_shi */ DEST_OVERWRITE | USE_FLAGS | FC,
    /* op_shs */ DEST_OVERWRITE | USE_FLAGS | FZ | FC,
    /* op_slo */ DEST_OVERWRITE | USE_FLAGS | FZ,
    /* op_sls */ DEST_OVERWRITE | USE_FLAGS | FZ | FC,
    /* op_nop */ (OPFLAG) 0,
    /* op_trap */ USES_A7 | SET_FLAGS,
#ifdef FLOAT_IEEE
    /* op_fadd */ DEST_MODIFY | SET_FLAGS,
    /* op_fsub */ DEST_MODIFY | SET_FLAGS,
    /* op_fdiv */ DEST_MODIFY | SET_FLAGS,
    /* op_fmul */ DEST_MODIFY | SET_FLAGS,
    /* op_fcmp */ SET_FLAGS,
    /* op_fmove */ DEST_OVERWRITE | SET_FLAGS,
    /* op_fmovem */ DEST_OVERWRITE,
#endif				/* FLOAT_IEEE */
#ifdef ASM
    /* op_asm */ DEST_OVERWRITE,
#endif				/* ASM */
    /* op_line */ (OPFLAG) 0,
    /* op_label */ (OPFLAG) 0,
};

/*****************************************************************************/

/*
 * find the end of a block of code.
 */
static CODE *block_end P1 (CODE *, ip)
{
    int     count = 0;

    while (ip != NIL_CODE && ip->opcode != op_bra
	   && ip->opcode != op_jmp
	   && ip->opcode != op_rts && ip->opcode != op_rte) {
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
CODE   *find_label P1 (LABEL, lab)
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
 * counts the number of times that a label node is referenced
 */
static int label_references P1 (const CODE *, ip)
{
    CODE   *target;
    SWITCH *sw;
    LABEL   i;
    LABEL   lab = ip->oper1->u.offset->v.l;
    int     count = 0;

    for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	if ((target != ip) &&
	    (is_label_used (target->oper1, lab) ||
	     is_label_used (target->oper2, lab)))
	    count++;
    }
    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	for (i = (LABEL) 0; i < sw->numlabs; i++) {
	    if (sw->labels[i] == lab) {
		count++;
	    }
	}
    }
    if (ip->back != NIL_CODE && ip->back->opcode == op_label) {
	count++;
    }
    if (ip->fwd != NIL_CODE && ip->fwd->opcode == op_label) {
	count++;
    }
    return count;
}

static BOOL is_label_used_in_expr P2 (const EXPR *, ep, LABEL, label)
{
    if (ep == NIL_EXPR) {
	return FALSE;
    }
    switch (ep->nodetype) {
    case en_labcon:
	return (ep->v.l == label);
    case en_add:
    case en_sub:
	return (is_label_used_in_expr (ep->v.p[0], label) ||
		is_label_used_in_expr (ep->v.p[1], label));
    case en_uminus:
    case en_cast:
	return (is_label_used_in_expr (ep->v.p[0], label));
    default:
	return FALSE;
    }
}

#if 0
/*
 * Returns false if ip2 alters the flags used by the conditional jump
 * in ip.
 */
static BOOL is_flags_unchanged P2 (CODE *, ip, CODE *, ip2)
{
    return (op_flags[ip2->opcode] & (SET_FLAGS | USE_FLAGS)) == 0;
}
#endif

/*
 * Returns false if the <ea> does not use the label specified.
 */

static BOOL is_label_used P2 (const ADDRESS *, ap, LABEL, label)
{
    if (ap == NIL_ADDRESS) {
	return FALSE;
    }
    switch (ap->mode) {
    case am_direct:
    case am_immed:
    case am_indx:
    case am_indx2:
    case am_indx3:
    case am_indx4:
    case am_indxpc:
    case am_indx2pc:
    case am_line:
    case am_str:
	return (is_label_used_in_expr (ap->u.offset, label));
    default:
	return FALSE;
    }
}

/*
 * Returns false if the register of ap1 is not used in the <ea> of ap2,
 * otherwise it returns true.  If we aren't sure then return true anyway.
 */
BOOL is_register_used P2 (REG, reg, const ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_dreg:
    case am_areg:
    case am_ainc:
    case am_adec:
    case am_ind:
    case am_indx:
    case am_freg:
    case am_indx2pc:
	return reg == ap->preg;
    case am_indx2:
    case am_indx4:
    case am_mreg:
	return reg == ap->preg || reg == ap->sreg;
    case am_smask:
    case am_rmask:
	return ((ap->u.mask & (((REGMASK) 1) << (int) reg)) != (REGMASK) 0);
    case am_immed:
    case am_direct:
    case am_indxpc:
    case am_none:
    case am_line:
	return FALSE;
    default:
	break;
    }
    return TRUE;
}

/*
 * Returns false if the <ea> of ap1 is not used in the <ea> of ap2, otherwise
 * it returns true.  If we aren't sure then return true anyway.
 */
static BOOL is_address_used P2 (const ADDRESS *, ap1, const ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS) {
	return FALSE;
    }
    switch (ap1->mode) {
    case am_dreg:
    case am_areg:
    case am_freg:
	return is_register_used (ap1->preg, ap2);
    case am_immed:
	return FALSE;
    default:
	break;
    }
    return TRUE;
}

/*
 * returns true if the value at the address ap1 id changed by the
 * addressing mode of ap2.
 */
static BOOL is_address_changed P2 (const ADDRESS *, ap1, const ADDRESS *, ap2)
{
    switch (ap2->mode) {
    case am_ainc:
    case am_adec:
	return is_address_used (ap1, ap2);
    default:
	break;
    }
    return FALSE;
}

/*
 * Checks to see if the addressing mode ap is overwritten with a new value
 * before the value is used (or the value is never used!).
 */
static BOOL is_dest_overwritten P2 (ADDRESS *, ap, const CODE *, ip)
{
    CODE   *ip2;
    BOOL    result = FALSE;
    OPCODE  op;

    if (ip == NIL_CODE) {
	return FALSE;
    }
    switch (ap->mode) {
    case am_areg:
    case am_dreg:
    case am_freg:
	for (ip2 = ip->fwd; ip2 != NIL_CODE; ip2 = ip2->fwd) {
	    switch (ip2->opcode) {
	    case op_unlk:
	    case op_rts:
	    case op_rte:
		if (is_temporary_data_register (ap->preg) ||
		    is_temporary_float_register (ap->preg)) {
		    /*
		     *               Might be a return value
		     */
		    return FALSE;
		} else {
		    return TRUE;
		}
	    case op_jmp:
	    case op_nop:
#ifdef ASM
	    case op_asm:	/* really it in unknown */
#endif /* ASM */
		return FALSE;
	    case op_beq:	/* use of A7 unknown */
	    case op_bne:
	    case op_bgt:
	    case op_bge:
	    case op_blt:
	    case op_ble:
	    case op_bls:
	    case op_blo:
	    case op_bhi:
	    case op_bhs:
		op = ip2->opcode;
		ip2->opcode = op_nop;	/* to prevent looping */
		result = is_dest_overwritten (ap, ip2) &&
		    is_dest_overwritten (ap,
					 find_label (ip2->oper1->u.offset->
						     v.l));
		ip2->opcode = op;
		return result;
	    case op_bra:
		op = ip2->opcode;
		ip2->opcode = op_nop;	/* to prevent looping */
		result =
		    is_dest_overwritten (ap,
					 find_label (ip2->oper1->u.offset->
						     v.l));
		ip2->opcode = op;
		return result;
	    case op_pea:	/* implicit use of A7 */
	    case op_link:
		if (ap->preg == STACKPTR) {
		    return FALSE;
		}
		break;
	    case op_jsr:
	    case op_bsr:
		if (is_temporary_register (ap->preg)) {
		    return TRUE;
		}
		/*lint -fallthrough */
	    default:
		if (is_address_used (ap, ip2->oper1)
		    || is_address_used (ap, ip2->oper2)) {
		    return FALSE;
		}
		break;
	    case op_label:
	    case op_line:
		break;
	    }
	}
	break;
    default:
	break;
    }
    return FALSE;
}

/*
 * compare two address nodes and return true if they are equivalent.
 */
BOOL is_equal_address P2 (const ADDRESS *, ap1, const ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS && ap2 == NIL_ADDRESS) {
	return TRUE;
    }
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS) {
	return FALSE;
    }
    if (ap1->mode != ap2->mode) {
	switch (ap1->mode) {
	case am_rmask:
	case am_smask:
	    switch (ap2->mode) {
	    case am_rmask:
	    case am_smask:
		break;
	    default:
		return FALSE;
	    }
	    break;
	default:
	    return FALSE;
	}
    }
    switch (ap1->mode) {
    case am_areg:
    case am_dreg:
    case am_ind:
    case am_freg:
	return ap1->preg == ap2->preg;
    case am_indx:
	return ap1->preg == ap2->preg &&
	    is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_indx2:
    case am_indx3:
    case am_indx4:
	return ap1->sreg == ap2->sreg &&
	    ap1->preg == ap2->preg &&
	    is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_rmask:
    case am_smask:
	return ap1->u.mask == ap2->u.mask;
    case am_mreg:
	return ap1->sreg == ap2->sreg && ap1->preg == ap2->preg;
    case am_direct:
    case am_immed:
	return is_equalnode (ap1->u.offset, ap2->u.offset);
    default:
	break;
    }
    return FALSE;
}

static BOOL is_equal_oper P2 (const ADDRESS *, ap1, const ADDRESS *, ap2)
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
    case am_areg:
    case am_dreg:
    case am_ind:
    case am_ainc:
    case am_adec:
    case am_freg:
    case am_indx2pc:
	return ap1->preg == ap2->preg;
    case am_indx:
	return ap1->preg == ap2->preg &&
	    is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_indx2:
    case am_indx3:
    case am_indx4:
	return ap1->preg == ap2->preg &&
	    ap1->sreg == ap2->sreg &&
	    is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_direct:
    case am_immed:
    case am_indxpc:
	return is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_smask:
    case am_rmask:
	return ap1->u.mask == ap2->u.mask;
    case am_none:
    case am_line:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}

/*
 * Determine whether a move was redundant ... this is done by looking back
 * along the code list (following all branches to labels) to determine
 * whether the destination already contains the necessary result.
 */
static BOOL was_move_redundant P3 (CODE *, ip, CODE *, ip2, BOOL, memory)
{
    BOOL    altered, overwritten;
    LABEL   label;
    SWITCH *sw;

    if (ip == NIL_CODE)
	return FALSE;
    if (ip2 == NIL_CODE)
	return FALSE;

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
		switch (ip2->opcode) {
		case op_beq:
		case op_bne:
		case op_bgt:
		case op_bge:
		case op_blt:
		case op_ble:
		case op_bls:
		case op_blo:
		case op_bhi:
		case op_bhs:
		case op_bra:
		    if (is_label_used (ip2->oper1, label)) {
			OPCODE  op = ip2->opcode;

			ip2->opcode = op_nop;
			if (!was_move_redundant (ip, ip2, memory)) {
			    ip2->opcode = op;
			    return FALSE;
			}
			ip2->opcode = op;
		    }
		    break;
		default:
		    break;
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
	case op_rts:
	case op_rte:
	case op_jmp:
	case op_bra:
	    /* should have at least hit a label before here! */
	case op_nop:
	    return TRUE;
	case op_jsr:
	case op_bsr:
	    return FALSE;
#ifdef ASM
	case op_asm:
	    return FALSE;
#endif /* ASM */
	case op_line:
	    break;
	default:
	    if (is_same_instruction (ip, ip2)) {
		return TRUE;
	    }
	    altered = is_altered (ip2->opcode);
	    overwritten = is_overwrite (ip2->opcode);
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
		if (altered) {
		    if (is_address_used (ip2->oper2, ip->oper1)) {
			return FALSE;
		    }
		    if (is_address_used (ip2->oper2, ip->oper2)) {
			return FALSE;
		    }
		}
		if (is_address_changed (ip->oper1, ip2->oper2) ||
		    is_address_changed (ip->oper2, ip2->oper2)) {
		    return FALSE;
		}
		switch (ip2->oper2->mode) {
		case am_dreg:
		case am_areg:
		case am_freg:
		case am_mreg:
		case am_direct:
		case am_immed:
		    if (altered && (is_equal_address (ip->oper2, ip2->oper2)
				    || is_equal_address (ip->oper1,
							 ip2->oper2))) {
			return FALSE;
		    }
		    break;
		case am_ainc:
		case am_adec:
		    if (is_equal_address (ip->oper2, ip2->oper2)) {
			return FALSE;
		    }
		    /*lint -fallthrough */
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
		if (is_address_changed (ip->oper1, ip2->oper1) ||
		    is_address_changed (ip->oper2, ip2->oper1)) {
		    return FALSE;
		}
		switch (ip2->oper1->mode) {
		case am_dreg:
		case am_areg:
		case am_freg:
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
		case am_ainc:
		case am_adec:
		    if (is_equal_address (ip->oper2, ip2->oper1)) {
			return FALSE;
		    }
		    /*lint -fallthrough */
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

/*
 * Determine whether a move is redundant ... this is done by looking forward
 * along the code list (following all branches) to determine
 * whether the destination is required before being overwritten.
 */
static BOOL is_move_redundant P3 (CODE *, ip, CODE *, ip2, BOOL, memory)
{
    (void) ip;			/* KEE COMPILEr HAPPY UNTIL CODE READY */
    (void) ip2;
    (void) memory;

#if 0				/* currently not fully working */
    BOOL    result, altered, overwritten;
    OPCODE  op;

    for (ip2 = ip2->fwd; ip2 != NIL_CODE; ip2 = ip2->fwd) {
	switch (ip2->opcode) {
	case op_rts:
	case op_rte:
	case op_jmp:
	    return FALSE;
	case op_nop:
	    return TRUE;
	case op_bra:
	    ip2->opcode = op_nop;	/* to prevent looping */
	    result =
		is_move_redundant (ip, find_label (ip2->oper1->u.offset->v.l),
				   memory);
	    ip2->opcode = op_bra;
	    return result;
	case op_beq:
	case op_bne:
	case op_bgt:
	case op_bge:
	case op_blt:
	case op_ble:
	case op_bls:
	case op_blo:
	case op_bhi:
	case op_bhs:
	    op = ip2->opcode;
	    ip2->opcode = op_nop;	/* to prevent looping */
	    result = is_move_redundant (ip, ip2, memory) &&
		is_move_redundant (ip, find_label (ip2->oper1->u.offset->v.l),
				   memory);
	    ip2->opcode = op;
	    return result;

	case op_jsr:
	case op_bsr:
	    return FALSE;
#ifdef ASM
	case op_asm:
	    return FALSE;
#endif /* ASM */
	case op_line:
	    break;
	default:
	    if (is_same_instruction (ip, ip2)) {
		return TRUE;
	    }
	    altered = is_altered (ip2->opcode);
	    overwritten = is_overwrite (ip2->opcode);
	    if (ip2->oper2) {
		/* two operand instruction */
		if (is_address_used (ip->oper2, ip2->oper1)) {
		    return FALSE;
		}
		if (overwritten &&
		    (ip->length <= ip2->length) &&
		    is_equal_address (ip->oper2, ip2->oper2)) {
		    return TRUE;
		}
		if (is_address_used (ip->oper2, ip2->oper2)) {
		    return FALSE;
		}
		break;
	    } else if (ip2->oper1) {
		/* one operand instruction */
		if (overwritten &&
		    (ip->length <= ip2->length) &&
		    is_equal_address (ip->oper2, ip2->oper1)) {
		    return TRUE;
		}
		if (is_address_used (ip->oper2, ip2->oper1)) {
		    return FALSE;
		}
	    }
	    break;
	}
    }
#endif
    return FALSE;
}

/*
 *	  Look forward from the instruction 'ip' and see if the flags are
 *	  used before being reset.
 */
static BOOL is_flag_used P1 (CODE *, ip)
{
    BOOL    result;

    if (ip == NIL_CODE) {
	return FALSE;
    }
    for (ip = ip->fwd; ip; ip = ip->fwd) {
	switch (ip->opcode) {
	case op_bra:
	    ip->opcode = op_nop;	/* to prevent looping */
	    result = is_flag_used (find_label (ip->oper1->u.offset->v.l));
	    ip->opcode = op_bra;
	    return result;
	case op_jsr:
	case op_bsr:
	case op_jmp:
	case op_rts:
	case op_rte:
	case op_nop:
	    return FALSE;
	default:
	    if (is_use_flags (ip->opcode)) {
		return TRUE;
	    }
	    if (is_noset_flags (ip->opcode)) {
		if (ip->oper2) {
		    if (ip->oper2->mode == am_areg) {
			break;
		    }
		} else if (ip->oper1) {
		    if (ip->oper1->mode == am_areg) {
			break;
		    }
		}
	    }
	    if (is_set_flags (ip->opcode)) {
		return FALSE;
	    }
	    break;
	}
    }
    return FALSE;
}

/* insert an instruction after the specified position in the code list */
static void insert_code P5 (CODE *, target, OPCODE, op, ILEN, len, ADDRESS *,
			    ap1, ADDRESS *, ap2)
{
    CODE   *p;

    p = code (op, len, ap1, ap2);
    p->fwd = target->fwd;
    p->back = target;
    target->fwd = p->fwd->back = p;
}

/* ensure we have a label to branch to (create if needed) */
static void check_label P2 (const CODE *, ip, CODE *, target)
{
    if (target->fwd->opcode == op_label) {
	ip->oper1->u.offset->v.l = target->fwd->oper1->u.offset->v.l;
    } else {
	insert_code (target, op_label, IL0, mk_label (nextlabel),
		     NIL_ADDRESS);
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
#ifdef PEEPFLOW
    ip->regmap = 0;
#endif /* PEEPFLOW */
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
    if (ap1 != NIL_ADDRESS && ap1->mode == am_freg &&
	(ap2 == NIL_ADDRESS || ap2->mode == am_freg)) {
	len = IL12;
    }
    add_peep (code (op, (ILEN) (len + 1), ap1, ap2));
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
    EXPR   *ep2;
    LABEL   i;

    opt3 (level);		/* do the peephole optimizations */
    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	if (ip->opcode == op_label) {
	    put_label (ip->oper1->u.offset->v.l);
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
	 * This limits the amount of code that can be generated in a
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
static void peep_delete P1 (const CODE *, ip)
{
    if (ip == NIL_CODE) {
	FATAL ((__FILE__, "peep_delete", ""));
	/* NOTREACHED */
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
 * changes LEA <ea>,An				  =>	   PEA <ea>
 *		   PEA (An)
 *	The value of An is not needed (An is scratch register)
 * CAVEAT code generator modifier!
 */
static void peep_pea P1 (const CODE *, ip)
{
    CODE   *prev;

    if (ip->oper1->mode != am_ind) {
	return;
    }
    if ((prev = ip->back) == NIL_CODE) {
	return;
    }
    if (prev->opcode == op_lea && prev->oper2->preg == ip->oper1->preg
	&& is_temporary_address_register (ip->oper1->preg)) {
	prev->opcode = op_pea;
	prev->oper2 = NIL_ADDRESS;
	peep_delete (ip);
    }
}

/*
 * peephole optimization for LEA instructions.
 */
static void peep_lea P1 (CODE *, ip)
{
    CODE   *next;

    if (ip->oper1->mode == am_ind && ip->oper1->preg == ip->oper2->preg) {
	/*
	 *       LEA (An), An    deleted
	 */
	peep_delete (ip);
	return;
    }
    if ((next = ip->fwd) == NIL_CODE) {
	return;
    }
    if (next->opcode == op_movea &&
	is_equal_address (ip->oper2, next->oper1) &&
	next->oper2->mode == am_areg &&
	is_temporary_register (next->oper1->preg)) {
	/*
	 *       LEA <ea>, An    =>      LEA <ea>, Am
	 *       MOVEA.L An, Am
	 */
	ip->oper2 = next->oper2;
	peep_delete (ip->fwd);
    }
    if (ip->oper2->preg == STACKPTR && is_dest_overwritten (ip->oper2, ip)) {
	peep_delete (ip);
	return;
    }
}

/*
 * peephole optimization for MOVE instructions.
 */
static void peep_move P1 (CODE *, ip)
{
    CODE   *ip2;
    EXPR   *ep;

    /*
     * move.w An,An changes the contents of An through sign extension
     */
    if (is_equal_address (ip->oper1, ip->oper2) &&
	(ip->oper1->mode != am_areg || ip->length != IL2)) {
	peep_delete (ip);
	return;
    }
    switch (ip->oper1->mode) {
    case am_immed:
	ep = ip->oper1->u.offset;

	if (ip->length == IL4) {
	    /*
	     * changes MOVE #n, -(A7) =>      PEA n
	     *
	     * unless n == 0
	     */
	    if (ip->oper2->mode == am_adec && ip->oper2->preg == STACKPTR &&
		(ep->nodetype != en_icon || ep->v.i != 0L)) {
		ip->opcode = op_pea;
		ip->oper2 = NIL_ADDRESS;
		ip->oper1 = copy_addr (ip->oper1, am_direct);
		ip->length = IL0;
		changes++;
		return;
	    }
	    /*
	     * changes MOVE #n, An    =>      LEA n, An
	     */
	    if (ip->oper2->mode == am_areg) {
		ip->opcode = op_lea;
		ip->oper1 = copy_addr (ip->oper1, am_direct);
		ip->length = IL0;
		changes++;
		return;
	    }
	}
	if (is_icon (ep)) {
	    /*
	     *      Replace move immediate An for the following cases
	     *                      MOVE #0, An
	     *                      MOVE #shortval, An
	     *      by the sequences
	     *                      SUB.L An,An
	     *      and     MOVE.W #shortval, An
	     */
	    if (ip->oper2->mode == am_areg) {
		if (ep->v.i == 0L) {
		    ip->length = IL4;
		    ip->opcode = op_sub;
		    ip->oper1 = ip->oper2;
		    changes++;
		} else if ((IVAL) -32768L <= ep->v.i
			   && ep->v.i <= (IVAL) 32767L) {
		    ip->length = IL2;
		}
		return;
	    }
	    /*
	     *                Replace moving small constants into a data register by
	     *                      MOVEQ   #value,Dn
	     */
	    if (ip->oper2->mode == am_dreg) {
		if ((IVAL) -128 <= ep->v.i && ep->v.i <= (IVAL) 127) {
		    ip->opcode = op_moveq;
		    ip->length = IL0;
		    changes++;
		    return;
		}
	    }
	    /*
	     *      Replace the sequence
	     *                      MOVE.W #X, -(An)
	     *                      MOVE.W #Y, -(An)
	     *      by the sequence
	     *                      MOVE.L #(X<<16|Y), -(An)
	     */
	    ip2 = ip->fwd;
	    if (ip->length == IL2 &&
		ip->oper2->mode == am_adec &&
		ip2->length == IL2 &&
		ip2->opcode == op_move &&
		ip2->oper1->mode == am_immed &&
		is_icon (ip2->oper1->u.offset) &&
		ip2->oper2->mode == am_adec &&
		ip2->oper2->preg == ip->oper2->preg) {
		ip->length = IL4;
		ep->v.u = (ep->v.u & (UVAL) 0xffffL) |
		    (ip2->oper1->u.offset->v.u << 16);
		peep_delete (ip2);
		return;
	    }
	    /*
	     * The M68000 reads the operand to be cleared.
	     * This is unsafe e.g. when manipulating interface
	     * registers, and clr is not too fast for this reason
	     * (although it is a considerable smaller instruction).
	     * We will therefore only do this optimisation on the
	     * M68000 if volatile is not set.
	     *
	     * M68010, M68020 etc. do not have this bug so always
	     * replace move immediate of zero by a CLR instruction.
	     *
	     * N. B. The special case of moving zero to a register
	     * will already have been optimised to MOVEQ or SUBA.
	     */
	    if (target_option >= target_68000 || !volatile_found) {
		if (ep->v.i == 0L) {
		    ip->opcode = op_clr;
		    ip->oper1 = ip->oper2;
		    ip->oper2 = NIL_ADDRESS;
		    changes++;
		    return;
		}
	    }
	}
	/*lint -fallthrough */
    case am_dreg:
    case am_areg:
    case am_freg:
    case am_direct:
	switch (ip->oper2->mode) {
	case am_dreg:
	    if (is_flag_used (ip)) {
		break;		/* sets the flags so cannot be deleted */
	    }
	    /*lint -fallthrough */
	case am_areg:
	case am_freg:
	    if (was_move_redundant (ip, ip, FALSE) ||
		is_move_redundant (ip, ip, FALSE)) {
		peep_delete (ip);
		return;
	    }
	    break;
	default:
	    break;
	}
	break;
    case am_ind:
    case am_indx:
	switch (ip->oper2->mode) {
	case am_areg:
	    if (ip->oper1->preg == ip->oper2->preg) {
		break;
	    }
	    if (was_move_redundant (ip, ip, TRUE) ||
		is_move_redundant (ip, ip, TRUE)) {
		peep_delete (ip);
		return;
	    }
	    break;
	case am_dreg:
	    if (is_flag_used (ip)) {
		break;		/* sets the flags so cannot be deleted */
	    }
	    /*lint -fallthrough */
	case am_freg:
	    if (was_move_redundant (ip, ip, TRUE) ||
		is_move_redundant (ip, ip, TRUE)) {
		peep_delete (ip);
		return;
	    }
	    break;
	default:
	    break;
	}
	break;
    default:
	break;
    }
}

static void peep_moveq P1 (CODE *, ip)
{
    if (was_move_redundant (ip, ip, TRUE)) {
	peep_delete (ip);
	return;
    }
}

/*
 * peephole optimization for (f)movem instructions.
 * (f)movem instructions are used to save registers on the stack.
 * if only one register is being saved we can convert the (f)movem to a
 * (f)move instruction.
 *
 */
static void peep_movem P1 (CODE *, ip)
{
    REG     reg;
    ADDRESS *ap = (ip->oper1->mode == am_smask) ? ip->oper1 : ip->oper2;

    for (reg = FP7; reg >= D0; reg--) {
	if (ap->u.mask == REGBIT (reg)) {
	    switch (reg) {
	    case D0:
	    case D1:
	    case D2:
	    case D3:
	    case D4:
	    case D5:
	    case D6:
	    case D7:
		ap->mode = am_dreg;
		ip->opcode = op_move;
		break;
	    case A0:
	    case A1:
	    case A2:
	    case A3:
	    case A4:
	    case A5:
	    case A6:
	    case A7:
		ap->mode = am_areg;
		if (ip->oper2->mode == am_rmask) {
		    ip->opcode = op_movea;
		} else {
		    ip->opcode = op_move;
		}
		break;
#ifdef FLOAT_IEEE
	    case FP0:
	    case FP1:
	    case FP2:
	    case FP3:
	    case FP4:
	    case FP5:
	    case FP6:
	    case FP7:
		ap->mode = am_freg;
		ip->opcode = op_fmove;
		break;
#endif /* FLOAT_IEEE */
	    default:
		CANNOT_REACH_HERE ();
	    }
	    ap->preg = reg;
	    changes++;
	    return;
	}
    }

    if (was_move_redundant (ip, ip, TRUE)) {
	peep_delete (ip);
    }
}

/*
 * peephole optimization for add instructions.
 * makes quick immediates out of small constants (redundant for most as's).
 * change add immediate to address registers to lea where possible
 */
static void peep_add P1 (CODE *, ip)
{
    EXPR   *ep;
    CODE   *next;

    if (ip->oper1->mode != am_immed) {
	return;
    }
    ep = ip->oper1->u.offset;
    if (ip->oper2->mode == am_areg) {
	if (ip->oper2->preg == STACKPTR
	    && is_dest_overwritten (ip->oper2, ip)) {
	    peep_delete (ip);
	    return;
	}
	/* adda.w extents the first argument automatically */
#ifdef COLDFIRE
	if (is_short (ep) && !is_coldfire ()) {
	    ip->length = IL2;
	}
#else
	if (is_short (ep)) {
	    ip->length = IL2;
	}
#endif /* COLDFIRE */
    } else {
	ip->opcode = op_addi;
    }
    if (ep->nodetype != en_icon) {
	return;
    }
    if (ep->v.i == 0L) {
	if (ip->oper2->mode == am_areg) {
	    /*
	     *               ADD #0, An      redundant
	     */
	    peep_delete (ip);
	} else {
	    /*
	     *               ADD #0, <ea>    =>      TST <ea>
	     */
	    ip->oper1 = ip->oper2;
	    ip->oper2 = NIL_ADDRESS;
	    ip->opcode = op_tst;
	    changes++;
	}
	return;
    }
    if ((IVAL) 1 <= ep->v.i && ep->v.i <= (IVAL) 8) {
	next = ip->fwd;
	if (ip->oper2->mode == am_areg &&
	    next && next->opcode == op_move &&
	    next->length == (ILEN) ep->v.i &&
	    next->oper2->mode == am_adec &&
	    next->oper2->preg == ip->oper2->preg &&
	    !is_address_used (ip->oper2, next->oper1)) {
	    /*
	     *               The sequence:
	     *                               ADD.W #4, A7
	     *                               MOVE.L <ea>, -(A7)
	     *               becomes
	     *                               MOVE.L <ea>, (A7)
	     */
	    next->oper2 = copy_addr (next->oper2, am_ind);
	    peep_delete (ip);
	} else {
	    ip->opcode = op_addq;
	}
	return;
    }
    if ((IVAL) -8 <= ep->v.i && ep->v.i <= (IVAL) -1) {
	/*
	 *       ADD #-n, <ea>   =>      SUB #n, <ea>
	 *
	 * where 1 <= n <= 8
	 */
	ep->v.i = -ep->v.i;
	ip->opcode = op_subq;
	changes++;
	return;
    }
    if (ip->oper2->mode == am_areg && is_short (ep)) {
	/*
	 *       ADDA.W #X, An   =>      LEA X(An), An
	 *
	 * they are the same size, but the lea version is quicker
	 */
	ip->oper1 = copy_addr (ip->oper1, am_indx);
	ip->oper1->preg = ip->oper2->preg;
	ip->length = IL0;
	ip->opcode = op_lea;
	changes++;
	return;
    }
}

/*
 * conversion of unsigned data types often yields statements like
 * move.b source,d0 +  andi.l #255,d0
 * which should be converted to
 * clr.l d0 + move.b source,d0
 * deletes and #-1
 */
static void peep_and P1 (CODE *, ip)
{
    CODE   *prev;
    ILEN    size;
    IVAL    val;

    if (ip->oper1->mode != am_immed
	|| ip->oper1->u.offset->nodetype != en_icon) {
	return;
    }
    val = ip->oper1->u.offset->v.i;
    if (val == -1L) {
	/*
	 *       AND #-1, <ea>
	 *
	 *      only sets flags, which the code generator does not know
	 */
	peep_delete (ip);
	return;
    }
    if ((prev = ip->back) == NIL_CODE) {
	return;
    }
    if (prev->opcode == op_and &&
	prev->oper1->mode == am_immed &&
	is_icon (prev->oper1->u.offset) &&
	is_equal_address (ip->oper2, prev->oper2)) {
	/*
	 *       AND #M, <ea>                    =>      AND #(M&N), <ea>
	 *       AND #N, <ea>
	 */
	prev->oper1->u.offset->v.i &= val;
	if (prev->length < ip->length) {
	    prev->length = ip->length;
	}
	peep_delete (ip);
	return;
    }
    /*
     *                               MOVE <ea>, <EA>                 =>      CLR <EA>
     *                               AND #N, <EA>                                    MOVE <ea>, <EA>
     *
     *               where N is 255 for byte instructions and 65535 for word
     *               instructions.   Also Dn is not used in the addressing mode
     *               for <ea>.
     */
    if (val == 255L) {
	size = IL1;
    } else if (val == 65535L) {
	size = IL2;
    } else {
	return;
    }

    if (prev->opcode != op_move || prev->length != size
	|| !is_equal_address (ip->oper2, prev->oper2)
	|| is_address_used (ip->oper2, prev->oper1)) {
	return;
    }
    prev->length = ip->length;

    ip->opcode = op_move;
    ip->oper1 = prev->oper1;
    ip->length = size;

    prev->opcode = op_clr;
    prev->oper1 = prev->oper2;
    prev->oper2 = NIL_ADDRESS;

    next_ip = prev;
}

static void peep_or P1 (const CODE *, ip)
{
    CODE   *prev;
    IVAL    val;

    if (ip->oper1->mode != am_immed) {
	return;
    }
    val = ip->oper1->u.offset->v.i;
    if (val == 0L) {
	/*
	 *       OR #0, <ea>
	 */
	peep_delete (ip);
	return;
    }
    if ((prev = ip->back) == NIL_CODE) {
	return;
    }
    if (prev->opcode == op_or &&
	prev->oper1->mode == am_immed &&
	is_icon (prev->oper1->u.offset) &&
	is_equal_address (ip->oper2, prev->oper2)) {
	/*
	 *       OR #M, <ea>                    =>              OR #(M|N), <ea>
	 *       OR #N, <ea>
	 */
	prev->oper1->u.offset->v.i |= val;
	if (prev->length < ip->length) {
	    prev->length = ip->length;
	}
	peep_delete (ip);
	return;
    }
}

/*
 * removes consecutive clr-statements
 *
 */
static void peep_clr P1 (const CODE *, ip)
{
    CODE   *prev;

    if ((prev = ip->back) == NIL_CODE || prev->opcode != op_clr ||
	!is_equal_address (ip->oper1, prev->oper1))
	return;

    if (prev->length < ip->length) {
	prev->length = ip->length;
    }
    peep_delete (ip);
}

/*
 * peephole optimization for subtract instructions.
 * makes quick immediates out of small constants (redundant for most as's).
 */
static void peep_sub P1 (CODE *, ip)
{
    EXPR   *ep;

    if (ip->oper1->mode != am_immed) {
	return;
    }
    ep = ip->oper1->u.offset;
    if (ip->oper2->mode == am_areg) {
	/*
	 *       SUBA.W extents the first argument automatically
	 */
#ifdef COLDFIRE
	if (is_short (ep) && !is_coldfire ()) {
	    ip->length = IL2;
	}
#else
	if (is_short (ep)) {
	    ip->length = IL2;
	}
#endif /* COLDFIRE */
    } else {
	ip->opcode = op_subi;
    }
    if (ep->nodetype != en_icon) {
	return;
    }
    if (ep->v.i == 0L) {
	if (ip->oper2->mode == am_areg) {
	    /*
	     *               SUB #0, An      redundant
	     */
	    peep_delete (ip);
	} else {
	    /*
	     *               SUB #0, <ea>    =>      TST <ea>
	     *
	     * provide <ea> != An
	     */
	    ip->oper1 = ip->oper2;
	    ip->oper2 = NIL_ADDRESS;
	    ip->opcode = op_tst;
	    changes++;
	    return;
	}
    }
    if ((IVAL) 1 <= ep->v.i && ep->v.i <= (IVAL) 8) {
	ip->opcode = op_subq;
	return;
    } else if ((IVAL) -8 <= ep->v.i && ep->v.i <= (IVAL) -1) {
	/*
	 *       SUB #-n, <ea>   =>      ADD #n, <ea>
	 *
	 *      where 1<= n <= 8
	 */
	ep->v.i = -ep->v.i;
	ip->opcode = op_addq;
	changes++;
	return;
    }
}

/*
 * peephole optimization for compare instructions.
 * changes compare #0 to tst
 */
static void peep_cmp P1 (CODE *, ip)
{
    EXPR   *ep;

    if (ip->oper1->mode != am_immed) {
	return;
    }
    ep = ip->oper1->u.offset;
    if (ip->oper2->mode == am_areg) {
	/*
	 *       CMPA.W extents the first argument automatically
	 */
	if (is_short (ep)) {
	    ip->length = IL2;
	}
	return;
    }
    ip->opcode = op_cmpi;
    if (is_icon (ep) && ep->v.i == 0L) {
	/*
	 *       CMP #0, <ea>    =>      TST <ea>
	 */
	ip->oper1 = ip->oper2;
	ip->oper2 = NIL_ADDRESS;
	ip->opcode = op_tst;
	next_ip = ip;
	return;
    }
}

/*
 * deletes a tst instruction if the flags are already set.
 */
static void peep_tst P1 (const CODE *, ip)
{
    CODE   *prev;

    prev = ip->back;
    if (prev == NIL_CODE) {
	return;
    }
    switch (prev->opcode) {
    case op_label:
	/*
	 * List all pseudo-instructions here. Of course, they do not do
	 * anything.
	 */
    case op_line:
#ifdef ASM
    case op_asm:
#endif /* ASM */
	return;

    case op_add:
    case op_addi:
    case op_addq:
    case op_adda:
    case op_addx:
    case op_sub:
    case op_subi:
    case op_subq:
    case op_suba:
    case op_subx:
    case op_asl:
    case op_asr:
    case op_neg:
    case op_negx:
	/*
	 * These instructions can set the carry and overflow flags which a
	 * tst instruction always clears.
	 */
	return;

    case op_mulu:
    case op_muls:
	/*
	 * These instructions set flags on full 32 bit register.
	 */
	if (prev->length < IL4) {
	    return;
	}
	break;

    case op_movea:
	/*
	 * A move TO an address register does not set the flags
	 */
	return;
    case op_move:
    case op_moveq:
    case op_clr:
	/*
	 * All other MOVE and CLR instructions set the flags according to
	 * the moved operand, which is prev->oper1
	 */
	if (is_equal_address (prev->oper1, ip->oper1)) {
	    break;
	}
	/*lint -fallthrough */
    default:
	/*
	 * If instructions have different length then the flags will not be
	 * set correctly.
	 */
	if (prev->length != ip->length) {
	    return;
	}
	/*
	 * All instructions that have a target set the flags according to the
	 * target (prev->oper2)
	 * Note that is_equal_address may be called with a NIL pointer
	 */
	if (is_equal_address
	    (prev->oper2 ? prev->oper2 : prev->oper1, ip->oper1)) {break;
	}
	return;
    }
    /*
     * We come here if the flags are already set, thus the tst
     * instruction can be deleted.
     */
    peep_delete (ip);
}

/*
 * peephole optimization for unconditional transfers. deletes instructions
 * which have no path. applies to bra, jmp, and rts instructions.
 */
static void peep_uctran P1 (const CODE *, ip)
{
    if (ip) {
	while (ip->fwd != NIL_CODE && ip->fwd->opcode != op_label) {
	    peep_delete (ip->fwd);
	}
    }
}

/*
 * optimizes conditional branch over a bra.
 */
static void peep_bxx P1 (const CODE *, ip)
{
    CODE   *p;
    LABEL   label = ip->oper1->u.offset->v.l;
#if 0
    CODE   *target;
    LABEL   label = ip->oper1->u.offset->v.l;
#endif
    static OPCODE revcond[] = {
	op_bne, op_beq,
	op_bge, op_bgt,
	op_ble, op_blt,
	op_bls, op_blo,
	op_bhs, op_bhi
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
	    label == next->oper1->u.offset->v.l) {
	    ip->fwd->opcode = revcond[(int) ip->opcode - (int) op_beq];
	    peep_delete (ip);
	}
    }
#if 0
    /* Space optimisation:
     * if the code after the branch instruction is the same as the
     * instruction after the label the branch can be moved forward an
     * instruction provided it didn't affect the conditions on which the
     * branch is taken.
     */

    target = find_label (label);
    /* now skip forward over identical instruction sequences */
    while (is_same_instruction (ip->fwd, target->fwd)
	   && is_flags_unchanged (ip, ip->fwd)) {
	/* switch order of instructions */
	p = ip->fwd;

	ip->fwd = p->fwd;
	p->fwd->back = ip;
	p->fwd = ip;

	p->back = ip->back;
	p->back->fwd = p;
	ip->back = p;

	/* and delete redundant instruction */
	peep_delete (target->fwd);
    }
    check_label (ip, target->back);
#endif

    /* delete branches to the following statement */
    p = ip->fwd;
    while (p != NIL_CODE && p->opcode == op_label) {
	if (p->oper1->u.offset->v.l == label) {
	    peep_delete (ip);
	    return;
	}
	p = p->fwd;
    }

}

static void peep_ext P1 (const CODE *, ip)
{
    CODE   *next = ip->fwd;

    if (next == NIL_CODE) {
	return;
    }
    if (next->oper1 &&
	next->oper1->mode == am_indx2 &&
	next->oper1->sreg == ip->oper1->preg) {
	/*
	 * convert       EXT.L Dn                =>      ??? 0(Am.Dn.W)
	 *                       ??? 0(Am,Dn.L)
	 */
	next->oper1->mode = am_indx4;
	peep_delete (ip);
	return;
    }
    switch (next->opcode) {
    case op_movea:
    case op_adda:
    case op_suba:
	if ((ip->length == IL4) &&
	    ((next->oper2->mode == am_areg) &&
	     is_equal_oper (next->oper1, ip->oper1))) {
	    /*
	     * convert                EXT.L Dn                =>      MOVE.W Dn,An
	     *                      MOVE.L Dn, An
	     */
	    peep_delete (ip);
	    next->length = IL2;
	}
	break;
    case op_asl:
    case op_asr:
    case op_rol:
    case op_ror:
	if (next->oper1->mode == am_dreg &&
	    is_equal_oper (next->oper1, ip->oper1)) peep_delete (ip);
	break;
    case op_clr:
	if (next->opcode == op_clr &&
	    next->fwd != NIL_CODE &&
	    next->fwd->opcode == op_tst &&
	    is_equal_oper (next->fwd->oper1, ip->oper1) &&
	    next->fwd->length == ip->length) {
	    /*
	     * convert              EXT.L Dn                =>              CLR.L Dm
	     *                              CLR.L Dm                                EXT.L Dn
	     *                              TST.L Dn
	     */
	    next->fwd->opcode = op_ext;
	    peep_delete (ip);
	}
	break;
    default:
	break;
    }
}

/*
 * For small shifts left, it can be more efficient to generate ADD
 * instructions.  Although there is a small speed gain when the shift
 * is by 2 there is a space penalty ... we go for space not speed.
 */
static void peep_shift P1 (CODE *, ip)
{
    if (ip->oper1->mode != am_immed) {
	return;
    }
    if (ip->oper1->u.offset->v.i == (IVAL) 1) {
	/*
	 *       SHL #1, Dn      =>      ADD Dn, Dn
	 */
	ip->opcode = op_add;
	ip->oper1 = copy_addr (ip->oper2, ip->oper2->mode);
	changes++;
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

    case op_bra:
	prev = ip->back;
	/*
	 * To make this fast, assume that the label number is really
	 * next->oper1->u.offset->v.l
	 */
	label = next->oper1->u.offset->v.l;
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
	    switch (prev->opcode) {
	    case op_bra:
	    case op_jmp:
	    case op_rts:
	    case op_rte:
		peep_uctran (prev);
		break;
	    default:
		break;
	    }
	}
	break;
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
	/* NOTREACHED */
    }
    /* Space optimisation:
     * if the code before the target of the branch is itself a branch
     * then we can move the destination block of code to eliminate the branch
     */
    p = target->back;
    if (p != NIL_CODE && ((p->opcode == op_bra)
			  || (p->opcode == op_jmp)
			  || (p->opcode == op_rts)
			  || (p->opcode == op_rte))) {
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

static void peep_jmp P1 (CODE *, ip)
{
    CODE   *target;

    if (!optimize_option) {
	return;
    }
    /*
     * see if we can find a jmp just like this one - if so we shall branch
     * to it.  This allows the tail end of the code for switch jump tables
     * to be commoned up.  This is space optimisation!
     */
    for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	if ((target != ip) && (is_same_instruction (ip, target))) {
	    ip->opcode = op_bra;
	    ip->oper1 = mk_label ((LABEL) 0);	/* label filled in by next line */
	    check_label (ip, target->back);
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
 * delete unnecessary stack frame creation
 */
static void peep_link P1 (const CODE *, ip)
{
    if (ip->fwd == NIL_CODE) {
	return;
    }
    if (ip->fwd->opcode == op_unlk &&
	is_equal_oper (ip->oper1, ip->fwd->oper1)) {
	peep_delete (ip->fwd);
	peep_delete (ip);
    }
}

/*
 * peephole optimizer. This routine calls the instruction specific
 * optimization routines above for each instruction in the peep list.
 */
static void opt3 P1 (unsigned, level)
{
    CODE   *ip;

    do {
	changes = 0;
	if (is_peep_phase (level, PEEP_INSTRUCTION)) {
	    /*
	     *       Instruction specific optisations
	     */
	    for (next_ip = peep_head; (ip = next_ip) != NIL_CODE;
		 next_ip = ip->fwd) {
		switch (ip->opcode) {
		case op_move:
		case op_movea:
#ifdef FLOAT_IEEE
		case op_fmove:
#endif /* FLOAT_IEEE */
		    peep_move (ip);
		    break;
		case op_movem:
#ifdef FLOAT_IEEE
		case op_fmovem:
#endif /* FLOAT_IEEE */
		    peep_movem (ip);
		    break;
		case op_moveq:
		    peep_moveq (ip);
		    break;
		case op_pea:
		    peep_pea (ip);
		    break;
		case op_lea:
		    peep_lea (ip);
		    break;
		case op_add:
		case op_addq:
		case op_adda:
		    peep_add (ip);
		    break;
		case op_and:
		    peep_and (ip);
		    break;
		case op_or:
		    peep_or (ip);
		    break;
		case op_clr:
		    peep_clr (ip);
		    break;
		case op_sub:
		case op_suba:
		    peep_sub (ip);
		    break;
		case op_cmp:
		case op_cmpa:
		    peep_cmp (ip);
		    break;
		case op_tst:
		    peep_tst (ip);
		    break;
		case op_ext:
		    peep_ext (ip);
		    break;
		case op_asl:
		    peep_shift (ip);
		    break;
		case op_line:
		    peep_line (ip);
		    break;
		case op_link:
		    peep_link (ip);
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
		case op_beq:
		case op_bne:
		case op_bgt:
		case op_bge:
		case op_blt:
		case op_bls:
		case op_blo:
		case op_bhi:
		case op_bhs:
		    peep_bxx (ip);
		    break;
		case op_rts:
		case op_rte:
		    peep_uctran (ip);
		    break;
		case op_label:
		    peep_label (ip);
		    break;
		case op_bra:
		    peep_bra (ip);
		    break;
		case op_jmp:
		    peep_jmp (ip);
		    break;
		default:
		    break;
		}
	    }
	}
#ifdef PEEPFLOW
	if (is_peep_phase (level, PEEP_FLOW)) {
	    changes += flow_dataflow (peep_head);
	}
#endif /* PEEPFLOW */
#ifdef VERBOSE
	if (verbose_option && changes) {
	    message (MSG_PEEPCHANGES, changes);
	}
#endif /* VERBOSE */
    } while (changes);
}

#ifdef PEEPFLOW
/*
 * builds the peepinfo of this instruction.
 */
REGMAP *build_regmap P1 (CODE *, ip)
{
    ADDRESS *src;
    ADDRESS *dst;
    REGMAP *map;

    if (ip->regmap == NULL) {
	ip->regmap = (REGMAP *) xalloc (sizeof (REGMAP));
    }
    map = ip->regmap;

    map->write = 0;
    map->modified = 0;
    map->read = 0;
    map->used = 0;
    map->updated = 0;

    dst = ip->oper2;
    if (dst == NIL_ADDRESS) {
	/* one operand instruction */
	dst = ip->oper1;
	src = NIL_ADDRESS;
    } else {
	/* two operand instruction */
	src = ip->oper2;
    }

    if (src != NIL_ADDRESS) {
	switch (src->mode) {
	case am_dreg:
	case am_areg:
	case am_freg:
	    map->read |= (1UL << (int) src->preg);
	    break;

	case am_ainc:
	case am_adec:
	    map->updated |= (1UL << (int) src->preg);
	    map->read |= (1UL << REG_MEMORY);
	    break;

	case am_indx2:
	    map->used |= (1UL << (int) src->sreg);
	    /*lint -fallthrough */
	case am_ind:
	case am_indx:
	    map->used |= (1UL << (int) src->preg);
	    /*lint -fallthrough */
	case am_direct:
	    /*lint -fallthrough */
	    map->read |= (1UL << REG_MEMORY);
	    break;

	default:
	    break;
	}
    }
    if (dst != NIL_ADDRESS) {
	switch (dst->mode) {
	case am_dreg:
	case am_areg:
	case am_freg:
	    map->modified |= (1UL << (int) dst->preg);
	    break;

	case am_ainc:
	case am_adec:
	    map->updated |= (1UL << (int) dst->preg);
	    map->modified |= (1UL << REG_MEMORY);
	    break;

	case am_indx2:
	    map->used |= (1UL << (int) dst->sreg);
	    /*lint -fallthrough */
	case am_ind:
	case am_indx:
	    map->used |= (1UL << (int) dst->preg);
	    /*lint -fallthrough */
	case am_direct:
	    /*lint -fallthrough */
	    map->modified |= (1UL << REG_MEMORY);
	    break;

	default:
	    break;
	}
    }
    if (is_modify (ip->opcode)) {
	map->write = map->modified;
	map->modified = 0;
    }
    if (is_uses_sp (ip->opcode)) {
	map->updated |= (1UL << A7);
    }
    return map;
}
#endif /* PEEP_FLOW */

#endif /* MC680X0 */
