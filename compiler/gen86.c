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

#include "config.h"

#ifdef INTEL_86
/******************************************************************************
 *
 * this module contains all of the code generation routines for evaluating
 * expressions and conditions for the INTEL 8086 processor.
 *
 *****************************************************************************/

#define GEN_MODULE

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "gen86.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define	AL_DEFAULT	(g_alignments[bt_ellipsis])

/********************************************************** Static Variables */

static unsigned peep_option = PEEP_ALL;	/* peephole optimisation */
static int target_option = target_8086;	/* code generator target */
static int stackopt_option = OPT_MINIMUM;	/* Use lazy stack optimisation */
static int regs_used = 0;	/* number of register variable allocated */
static REGMASK restore_mask;	/* register restore mask */
static SIZE max_stack_adjust = 0L;	/* largest amount stack is altered */
static REG regframe = FRAMEPTR;
static SETVAL regsave_option = EMPTY_SET;	/* registers to save on call */

static ADDRESS eax_reg = {
    am_dreg, EAX, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};
static ADDRESS ecx_reg = {
    am_dreg, ECX, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};
static ADDRESS edx_reg = {
    am_dreg, EDX, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};
static ADDRESS esp_reg = {
    am_dreg, ESP, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};
static ADDRESS edi_reg = {
    am_dreg, EDI, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};
static ADDRESS esi_reg = {
    am_dreg, ESI, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};
static ADDRESS ah_reg = {
    am_dreg, EAX+4, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};

static REGTYPE reg_type[] = {
    (REGTYPE) (D_REG | T_REG | X_REG | N_REG),	/* EAX */
    (REGTYPE) (D_REG | T_REG | X_REG | N_REG),	/* EDX */
    (REGTYPE) (D_REG | T_REG | Y_REG | C_REG),	/* ECX */
    (REGTYPE) (D_REG | T_REG),	/* EBX */
    (REGTYPE) (A_REG | T_REG),	/* ESI */
    (REGTYPE) (A_REG | T_REG),	/* EDI */
    0,				/* ESP */
    0,				/* EBP */
    0,				/* AX */
    0,				/* DX */
    0,				/* CX */
    0,				/* BX */
    0,				/* SI */
    0,				/* DI */
    0,				/* SP */
    0,				/* BP */
    0,				/* AL */
    0,				/* DL */
    0,				/* CL */
    0,				/* BL */
    0,				/* AH */
    0,				/* DH */
    0,				/* BH */
    0,				/* CH */
    (REGTYPE) (F_REG | T_REG),	/* ST0 */
    (REGTYPE) (F_REG | T_REG),	/* ST1 */
    (REGTYPE) (F_REG | T_REG),	/* ST2 */
    (REGTYPE) (F_REG | T_REG),	/* ST3 */
    (REGTYPE) (F_REG | T_REG),	/* ST4 */
    (REGTYPE) (F_REG | T_REG),	/* ST5 */
    (REGTYPE) (F_REG | T_REG),	/* ST6 */
    (REGTYPE) (F_REG | T_REG)	/* ST7 */
};

/*
 *  The following tables specify the alignment requirements of the
 *  basic types depending on the processor type.
 */
static SIZE alignments_8086[] = {
    1L,				/* bt_void      */
    1L,				/* bt_bool      */
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
    2L,				/* bt_longlong  */
    2L,				/* bt_ulonglong */
    2L,				/* bt_float     */
    2L,				/* bt_double    */
    2L,				/* bt_longdouble */
    2L,				/* bt_floatcomplex */
    2L,				/* bt_doublecomplex */
    2L,				/* bt_longdoublecomplex */
    2L,				/* bt_floatimaginary */
    2L,				/* bt_doubleimaginary */
    2L,				/* bt_longdoubleimaginary */
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
PRIVATE SIZE *g_alignments = &alignments_8086[0];

#endif /* MULTIPLE_PROCESSORS */

/*********************************************** Static Function Definitions */

static ADDRESS *func_result P_ ((FLAGS, SIZE, TYP *));
static ADDRESS *g_asbin P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asbitfield P_ ((const EXPR *, FLAGS, OPCODE, BOOL));
static ADDRESS *g_expr P_ ((const EXPR *, FLAGS));
static ADDRESS *g_extend P_ ((ADDRESS *, TYP *, TYP *));
static ADDRESS *g_extend2 P_ ((ADDRESS *, TYP *));
static ADDRESS *mk_amode P_ ((AMODE));
static ADDRESS *mk_expr P_ ((AMODE, EXPR *));
static ADDRESS *mk_indirect P_ ((REG, EXPR *));
static ADDRESS *mk_low P_ ((ADDRESS *));
static ADDRESS *mk_high P_ ((ADDRESS *));
static void g_compare P_ ((const EXPR *, OPCODE, OPCODE, LABEL));
static void g_falsejp P_ ((const EXPR *, LABEL));
static void g_test P_ ((const EXPR *));
static void g_truejp P_ ((const EXPR *, LABEL));

#ifdef STACK_CHECK
static ADDRESS *mk_strlab P_ ((const CHAR *));

#endif /* STACK_CHECK */

/*********************************************** Global Function Definitions */

#ifdef MULTIPLE_PROCESSORS
PRIVATE BOOL g_is_bigendian P_ ((void));
PRIVATE BOOL g_is_ascending_stack P_ ((void));
PRIVATE void g_auto_align P_ ((void));
PRIVATE void g_flush P_ ((SYM *));
PRIVATE void g_allocate P_ ((CSE *));
PRIVATE void g_branch P_ ((LABEL));
PRIVATE void g_entry P_ ((SIZE));
PRIVATE void g_epilogue P_ ((void));
PRIVATE void g_expression P_ ((const EXPR *));
PRIVATE void g_jfalse P_ ((const EXPR *, LABEL));
PRIVATE void g_jtrue P_ ((const EXPR *, LABEL));
PRIVATE void g_label P_ ((LABEL));
PRIVATE void g_return P_ ((const EXPR *, TYP *));
PRIVATE void g_stack P_ ((SIZE));
PRIVATE void g_switch_compare P_ ((const EXPR *, STMT *));
PRIVATE void g_switch_table P_ ((const EXPR *, const SWITCH *, UVAL, UVAL));
PRIVATE void g_initialize P_ ((void));

#endif /* MULTIPLE_PROCESSORS */

/*****************************************************************************/

/*
 * copy an address mode structure.
 */
static ADDRESS *copy_addr P2 (ADDRESS *, ap, AMODE, mode)
{
    ADDRESS *newap;

    if (ap == NIL_ADDRESS) {
	FATAL ((__FILE__, "copy_addr", "ap == 0"));
    }
    newap = (ADDRESS *) xalloc (sizeof (ADDRESS));

    *newap = *ap;
    newap->mode = mode;
    return newap;
}

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

#ifndef INTEL_386
/*
 * make a node to reference an immediate value i.
 */
ADDRESS *mk_immed P1 (IVAL, i)
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
#endif /* INTEL_386 */

#ifdef DEBUGOPT
/*
 * make a node to reference a line number.
 */
static ADDRESS *mk_line P1 (LINE, i)
{
    return mk_expr (am_line, mk_const ((IVAL) i));
}

#endif /* DEBUGOPT */

#ifdef DEBUGOPT
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
#endif /* DEBUGOPT */

/*
 * make a direct reference to a node.
 */
static ADDRESS *mk_direct P1 (EXPR *, ep)
{
    return mk_expr (am_direct, ep);
}

/*
   * make an indirect reference to a node.
 */
static ADDRESS *mk_indirect P2 (REG, reg, EXPR *, ep)
{
    ADDRESS *ap;

    ap = mk_expr (am_indx, ep);
    ap->preg = reg;
    return ap;
}

#ifdef STACK_CHECK
/*
 * generate a direct reference to a string label.
 */
static ADDRESS *mk_strlab P1 (const CHAR *, s)
{
    EXPR   *ep;

    ep = mk_node (en_nacon, NIL_EXPR, NIL_EXPR, tp_void);
    ep->v.str = s;
    return mk_expr (am_direct, ep);
}
#endif /* STACK_CHECK */

#ifndef INTEL_386
/*
 * make an address reference to a register.
 */
ADDRESS *mk_reg P1 (REG, reg)
{
    ADDRESS *ap;

    switch (reg) {
    case EAX:
    case EBX:
    case ECX:
    case EDX:
    case ESI:
    case EDI:
    case ESP:
    case EBP:
	ap = mk_amode (am_dreg);
	break;
#ifdef FLOAT_IEEE
    case ST0:
    case ST1:
    case ST2:
    case ST3:
    case ST4:
    case ST5:
    case ST6:
    case ST7:
	ap = mk_amode (am_freg);
	break;
#endif /* FLOAT_IEEE */
    default:
	CANNOT_REACH_HERE ();
    }
    ap->preg = reg;
    return ap;
}

ADDRESS *mk_mreg P2 (REG, r1, REG, r2)
{
    ADDRESS *ap;

    ap = mk_amode (am_mreg);
    ap->preg = r1;
    ap->sreg = r2;
    return ap;
}
#endif /* INTEL_386 */

static ADDRESS *mk_offset P2 (ADDRESS *, ap, SIZE, off)
{
    switch (ap->mode) {
    case am_ind:
	ap = mk_indirect (ap->preg, mk_const (off));
	return ap;
    case am_indx:
    case am_indx2:
	ap = copy_addr (ap, ap->mode);
	ap->u.offset = mk_const (ap->u.offset->v.i + off);
	return ap;
    case am_direct:
	ap = copy_addr (ap, ap->mode);
	ap->u.offset = mk_add (ap->u.offset, mk_const (off));
	return ap;
    default:
	/* address must be done "by hand" */
	return NIL_ADDRESS;
    }
}

/*
 *   Get the low address/reference for a 4 byte item
 */
static ADDRESS *mk_low P1 (ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_dreg:
    case am_areg:
	return ap;
    case am_mreg:
	return mk_reg (ap->preg);
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_direct:
	return mk_offset (ap, 0L);
    case am_immed:
	return mk_immed ((IVAL) (ap->u.offset->v.u & 0x0000ffffL));
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return NIL_ADDRESS;
}

/*
 *   Get the high address/reference for a 4 byte item
 */
static ADDRESS *mk_high P1 (ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_dreg:
    case am_areg:
	/* Note - this should only be used for byte sized registers */
        return mk_reg ((REG)(ap->preg+4));
    case am_mreg:
	return mk_reg (ap->sreg);
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_direct:
	return mk_offset (ap, 2L);
    case am_immed:
	return mk_immed ((IVAL) ((ap->u.offset->v.u >> 16) & 0x0000ffffL));
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return NIL_ADDRESS;
}

/*
 * returns addressing mode of form offset(regframe)
 * size is rounded up to AL_DEFAULT
 */
static ADDRESS *mk_scratch P1 (SIZE, size)
{
    ADDRESS *ap;

    /* round up the request */
    if (size % AL_DEFAULT) {
	size += AL_DEFAULT - (size % AL_DEFAULT);
    }
    /* allocate the storage */
    act_scratch += size;

    /*
     * The next statement could be deferred and put into the
     * routine checkstack(), but this is just safer.
     */
    if (act_scratch > max_scratch) {
	max_scratch = act_scratch;
    }
    ap = mk_indirect (regframe, mk_const (-(lc_auto_max + act_scratch)));
    return ap;
}

/*
 * add a label node to the peep list
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
 *    Add a conditional branch instruction to the peep list
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
    switch (bytes) {
    case 0l:
	return;
    default:
	g_code (op_add, IL2, mk_immed (bytes), &esp_reg);
    }
    stack_offset -= bytes;
    if (max_stack_adjust < bytes) {
	max_stack_adjust = bytes;
    }
}

/*
 * mk_legal will coerce the addressing mode in ap1 into a mode that is
 * satisfactory for the flag word.
 */
static ADDRESS *mk_legal P3 (ADDRESS *, ap, FLAGS, flags, TYP *, tp)
{
    ADDRESS *ap1, *ap2;
    SIZE    size = tp->size;

    if (flags & F_NOVALUE) {
	if (ap) {
#ifdef FLOAT_IEEE
	    if (ap->mode == am_freg) {
		g_fcode (op_fstp, IL10, mk_reg (ST0), NIL_ADDRESS);
	    }
#endif /* FLOAT_IEEE */
	    freeop (ap);
	}
	return NIL_ADDRESS;
    }
    if (ap == NIL_ADDRESS) {
	FATAL ((__FILE__, "mk_legal", "ap == 0"));
    }
    switch (ap->mode) {
    case am_immed:
	if (flags & F_IMMED) {
	    return ap;		/* mode ok */
	}
	break;
    case am_areg:
	if (flags & F_AREG) {
	    if ((flags & F_VOL) && !is_temporary_register (ap->preg)) {
		break;
	    }
	    if ((flags & F_NOEDI) && (ap->preg == EDI || ap->preg == ESI)) {
		break;
	    }
	    return ap;
	}
	break;
    case am_dreg:
	if (flags & F_DREG) {
	    if ((flags & F_VOL) && !is_temporary_register (ap->preg)) {
		break;
	    }
	    if ((flags & F_NOEDI) && (ap->preg == EDI || ap->preg == ESI)) {
		break;
	    }
	    if ((flags & F_NOECX) && (ap->preg == ECX)) {
		break;
	    }
	    if (flags & F_EAXEDX) {
		break;
	    }
	    if ((flags & F_ECX) && (ap->preg != ECX)) {
		break;
	    }
	    return ap;
	}
#ifdef FLOAT_IEEE
	if (flags & F_FREG) {
	    ap1 = mk_indirect (ESP, mk_const (-2l));
	    g_code (op_mov, IL2, ap, ap1);
	    g_fcode (op_fild, IL2, ap1, NIL_ADDRESS);
	    freeop (ap);
	    ap = float_register ();
	    return ap;
	}
	break;
    case am_freg:
	if (flags & F_FREG) {
	    return ap;
	}
#endif /* FLOAT_IEEE */
	break;
    case am_mreg:
	if (flags & F_DREG) {
	    if ((flags & F_NOEDI) && (ap->preg == EDI || ap->preg == ESI ||
				      ap->sreg == EDI || ap->sreg == ESI))
		break;
	    if ((flags & F_VOL) && !is_temporary_register (ap->preg) &&
		!is_temporary_register (ap->sreg))
		break;
	    return ap;
	}
	if (flags & F_EAXEDX)
	    return ap;
	break;
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_direct:
	if (flags & F_MEM) {
	    return ap;
	}
	break;
    default:
	FATAL (
	       (__FILE__, "mk_legal", "mode = %d, flags = 0x%x", ap->mode,
		flags));
    }
    if (flags & F_DREG) {
	freeop (ap);		/* maybe we can reuse it */
	if (flags & F_ECX) {
	    ap2 = cx_register ();
	    g_code (op_mov, IL2, ap, ap2);
	    return ap2;
	}
	if (flags & F_EAXEDX) {
	    switch (size) {
	    case IL1:
		ap2 = ax_register ();
		g_code (op_mov, IL1, ap, mk_low (ap2));
		break;
	    default:
		ap2 = axdx_register ();
		g_code (op_mov, IL2, ap, mk_low (ap2));
		break;
	    }
	    return ap2;
	}
	switch (size) {
	case 1L:
	    /*
	     *       byte transfers from %edi/%esi to a scratch register come
	     *        up here
	     */
	    if (ap->mode == am_dreg && (ap->preg == ESI || ap->preg == EDI)) {
		size = 2L;
	    }
	    /*lint -fallthrough */
	case 2L:
	    if (flags & F_NOECX) {
		ap2 = data_register_no_cx ();
	    } else {
		ap2 = data_register ();
	    }
	    g_code (op_mov, (ILEN) size, ap, ap2);
	    return ap2;
	case 4L:
	    ap2 = mdata_register ();
	    g_code (op_mov, IL2, mk_low (ap), mk_low (ap2));
	    g_code (op_mov, IL2, mk_high (ap), mk_high (ap2));
	    return ap2;
	default:
	    break;
	}
    }
    if (flags & F_AREG) {
	freeop (ap);		/* maybe we can reuse it */
	switch (size) {
	case 1L:
	    ap2 = address_register ();
	    ap1 = data_register ();
	    g_code (op_mov, (ILEN) size, ap, ap1);
	    ap1 = g_extend (ap1, tp, tp_short);
	    g_code (op_mov, 2L, ap1, ap2);
	    freeop (ap1);
	    return ap2;
	case 2L:
	    ap2 = address_register ();
	    g_code (op_mov, (ILEN) size, ap, ap2);
	    return ap2;
	default:
	    break;
	}
    }
#ifdef FLOAT_IEEE
    if (flags & F_FREG) {
	freeop (ap);
	switch (tp->type) {
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap2 = float_register ();
	    g_fcode (op_fld, (ILEN) size, ap, NIL_ADDRESS);
	    return ap2;
	case bt_char:
	case bt_schar:
/*KDW */
	case bt_short:
	case bt_int16:
	case bt_long:
	case bt_int32:
	    ap2 = float_register ();
	    g_fcode (op_fild, (ILEN) size, ap, NIL_ADDRESS);
	    return ap2;
	default:
	    break;
	}
    }
#endif /* FLOAT_IEEE */
    if (flags & F_MEM) {
	freeop (ap);
	ap2 = mk_scratch (tp->size);
	switch (tp->size) {
	case 1L:
	case 2L:
	    switch (ap->mode) {
#ifdef FLOAT_IEEE
	    case am_freg:
		g_fcode (op_fstp, (ILEN) size, ap2, NIL_ADDRESS);
		break;
#endif /* FLOAT_IEEE */
	    default:
		g_code (op_mov, (ILEN) size, ap, ap2);
		break;
	    }
	    break;
	case 4L:
	    switch (ap->mode) {
#ifdef FLOAT_IEEE
	    case am_freg:
		g_fcode (op_fstp, (ILEN) size, ap2, NIL_ADDRESS);
		break;
#endif /* FLOAT_IEEE */
	    case am_mreg:
		g_code (op_mov, IL2, mk_high (ap), mk_high (ap2));
		g_code (op_mov, IL2, mk_low (ap), mk_low (ap2));
		break;
	    default:
		g_code (op_mov, (ILEN) size, ap, ap2);
	    }
	    break;
	case 8L:
	case 10L:
	    switch (ap->mode) {
#ifdef FLOAT_IEEE
	    case am_freg:
		g_fcode (op_fstp, (ILEN) size, ap2, NIL_ADDRESS);
		break;
#endif /* FLOAT_IEEE */
	    default:
		break;
	    }
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	return ap2;
    }
    FATAL (
	   (__FILE__, "mk_legal", "mode = %d, flags = 0x%x", ap->mode,
	    flags));
    return NIL_ADDRESS;
}

static ADDRESS *g_index P1 (const EXPR *, ep)
{
    ADDRESS *ap1, *ap2;
    EXPR   *ep0 = ep->v.p[0];
    EXPR   *ep1 = ep->v.p[1];

    ap1 = g_expr (ep0, (FLAGS) (F_AREG | F_IMMED | F_VOL));
    switch (ap1->mode) {
    case am_areg:
	ap2 = g_expr (ep1, F_ALL);
	validate (ap1);
	break;
    case am_immed:
	ap2 = ap1;
	ap1 = g_expr (ep1, (FLAGS) (F_AREG | F_IMMED));
	validate (ap2);
	break;
    default:
	CANNOT_REACH_HERE ();
    }

    switch (ap1->mode) {
    case am_areg:
	if (!is_temporary_register (ap1->preg)) {
	    ap1 = copy_addr (ap1, ap1->mode);
	    switch (ap2->mode) {
	    case am_dreg:
		/* 0(Rn,Rm) */
		ap1->mode = am_indx2;
		ap1->sreg = ap2->preg;
		ap1->deep = ap2->deep;
		ap1->u.offset = mk_const (0L);
		return ap1;
	    default:
		break;
	    }
	}
	break;
    case am_immed:
	switch (ap2->mode) {
	case am_immed:
	    ap1 = copy_addr (ap1, am_direct);
	    ap1->u.offset = mk_add (ap1->u.offset, ap2->u.offset);
	    return ap1;
	case am_areg:
	    g_code (op_add, IL2, ap1, ap2);
	    ap2 = copy_addr (ap2, am_ind);
	    return ap2;
	default:
	    CANNOT_REACH_HERE ();
	}
	break;
    default:
	CANNOT_REACH_HERE ();
    }
    freeop (ap2);
    if (!is_temporary_register (ap1->preg)) {
	ap1 = mk_legal (ap1, (FLAGS) (F_AREG | F_VOL), tp_pointer);
    }
    g_code (op_add, IL2, ap2, ap1);
    ap1 = copy_addr (ap1, am_ind);
    return ap1;
}


/*
 * rotate a bitfield into the required position (assumes ap is a register)
 */
static void g_rotate P5 (ADDRESS *, ap, ILEN, ilen, int, offset, TYP *, tp,
			 int, width)
{
    OPCODE  op;
    int ls, rs;
    ADDRESS *ap2;

    switch (tp->type) {
    case bt_int16:
	/* sign bitfield */
	ls = 16 - offset - width;
	rs = 16 - width;
	if (ls > 1) {
	    ap2 = cx_register ();
	    g_code (op_mov, IL2, mk_immed ((IVAL) ls), ap2);
	    g_code (op_asl, IL2, ap2, ap);
	    freeop (ap);
	} else if (ls > 0) {
	    g_code (op_asl, IL2, mk_immed (1L), ap);
	}
	if (rs > 1) {
	    g_code (op_mov, IL2, mk_immed ((IVAL)rs), ap2);
	    g_code (op_asr, IL2, ap2, ap);
	    freeop (ap);
	} else if (ls > 0) {
	    g_code (op_asr, IL2, mk_immed (1L), ap);
	}
    default:
	/* offset is in range -15 .. 15 */
	if (offset < 0) {
	    offset += ilen * 8;
	}
	/* offset in range 0..15 */
	if (offset != 0) {
	    if (offset > 7) {
		op = op_rol;
		offset = 16 - offset;
	    } else {
		op = op_ror;
	    }
	    g_code (op, ilen, mk_immed ((IVAL) offset), ap);
	}
	if (tp->type != bt_void) {
	    g_code (op_and, ilen, mk_immed ((IVAL) bitmask ((BITSIZE) width)),
		    ap);
	}
    }
}

/*
 *   'ap' which has the size of type 'tp1' is to be extended to
 *   be the size of 'tp2'.
 */
static ADDRESS *g_extend P3 (ADDRESS *, ap, TYP *, tp1, TYP *, tp2)
{
    ap = mk_legal (ap, (FLAGS) (F_DREG | F_VOL), tp1);
    switch (tp2->type) {
    case bt_int16:
    case bt_uint16:
    case bt_short:
    case bt_ushort:
	switch (tp1->type) {
	case bt_char:
	case bt_schar:
	    if (ap->preg == EAX) {
		g_code (op_cbw, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    } else {
		if (target_option >= target_80186) {
		    /* 80186 code */
		    g_code (op_asl, IL2, mk_immed (8L), mk_low (ap));
		    g_code (op_asr, IL2, mk_immed (8L), mk_low (ap));
		} else {
		    g_code (op_xchg, IL2, mk_low (ap), mk_reg (EAX));
		    g_code (op_cbw, IL0, NIL_ADDRESS, NIL_ADDRESS);
		    g_code (op_xchg, IL2, mk_reg (EAX), mk_low (ap));
		}
	    }
	    break;
	case bt_uchar:
	case bt_charu:
	    g_code (op_xor, IL1, mk_high (ap), mk_high (ap));
	    break;
	case bt_int16:
	case bt_uint16:
	case bt_short:
	case bt_ushort:
	    break;
	case bt_int32:
	case bt_long:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	break;
    case bt_int32:
    case bt_long:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	switch (ap->mode)
	{
	case am_mreg:
	    break;
	default:
	    freeop (ap);
	    ap = mdata_register();
	    break;
	}
	switch (tp1->type) {
	case bt_char:
	case bt_schar:
	    if (ap->preg == EAX) {
		g_code (op_cbw, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    } else {
		if (target_option >= target_80186) {
		    /* 80186 code */
		    g_code (op_asl, IL2, mk_immed (8L), mk_low (ap));
		    g_code (op_asr, IL2, mk_immed (8L), mk_low (ap));
		} else {
		    g_code (op_xchg, IL2, mk_low (ap), mk_reg (EAX));
		    g_code (op_cbw, IL0, NIL_ADDRESS, NIL_ADDRESS);
		    g_code (op_xchg, IL2, mk_reg (EAX), mk_low (ap));
		}
	    }
	    /*lint -fallthrough */
	case bt_int16:
	case bt_short:
	    if (ap->preg == EAX) {
		g_code (op_cwd, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    } else {
		g_code (op_mov, IL2, mk_immed (0L), mk_high (ap));
		g_code (op_or, IL2, mk_low (ap), mk_low (ap));
		g_code (op_jge, IL0, mk_label (nextlabel), NIL_ADDRESS);
		g_code (op_not, IL2, mk_high (ap), NIL_ADDRESS);
		g_label (nextlabel++);
	    }
	    break;
	case bt_charu:
	case bt_uchar:
	    if (target_option >= target_80186) {
		/* 80186 code */
		g_code (op_shl, IL2, mk_immed (8L), mk_low (ap));
		g_code (op_shr, IL2, mk_immed (8L), mk_low (ap));
	    } else {
		g_code (op_and, IL2, mk_immed (0xFF00L), mk_low (ap));
	    }
	    /*lint -fallthrough */
	case bt_uint16:
	case bt_ushort:
	case bt_pointer16:
	    g_code (op_mov, IL2, mk_immed (0L), mk_high (ap));
	    break;
	case bt_int32:
	case bt_long:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	break;
    default:
	FATAL (
	       (__FILE__, "g_extend", "type1=%d, type2=%d", tp1->type,
		tp2->type));
	break;
    }
    return ap;
}

/*
 * extend 'ap' from 1 byte to 2 bytes
 */
static ADDRESS *g_extend2 P2 (ADDRESS *, ap, TYP *, tp)
{
    switch (tp->type)
    {
    case bt_char:
    case bt_schar:
	ap = g_extend (ap, tp, tp_short);
	break;
    case bt_uchar:
    case bt_charu:
	ap = g_extend (ap, tp, tp_ushort);
	break;
    case bt_short:
    case bt_int16:
	ap = g_extend (ap, tp, tp_long);
	break;
    case bt_ushort:
    case bt_uint16:
    case bt_pointer16:
	ap = g_extend (ap, tp, tp_ulong);
	break;
    default:
	FATAL((__FILE__, "g_extend2", "type=%d", tp->type));
	break;
    }
    return ap;
}

/*
 * return the addressing mode of a dereferenced node.
 */
static ADDRESS *g_deref P2 (const EXPR *, ep, TYP *, tp)
{
    ADDRESS *ap1;

    /*
     *       If a reference to a structure/union is required, return a
     *       pointer to the struct instead.
     */
    if (is_structure_type (tp) || is_array_assignment (tp)) {
	return g_expr (ep, F_ALL);
    }
    switch (ep->nodetype) {
    case en_add:
	return g_index (ep);
    case en_autocon:
	return mk_indirect (regframe, mk_const (ep->v.i));
    default:
	ap1 = g_expr (ep, (FLAGS) (F_AREG | F_IMMED));
	if (ap1->mode == am_immed) {
	    return copy_addr (ap1, am_direct);
	} else {
	    return copy_addr (ap1, am_ind);
	}
    }
}

/*
 * get a bitfield value
 */
static ADDRESS *g_fderef P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap;

    ap = g_deref (ep->v.p[0], ep->etp);
    ap = mk_legal (ap, (FLAGS) (F_DREG | F_VOL), ep->etp);
    g_rotate (ap, (ILEN) ep->etp->size, (int) ep->v.bit.offset, ep->etp,
	      (int) ep->v.bit.width);
    return mk_legal (ap, flags, ep->etp);
}

/*
 * generate code to evaluate a unary minus or complement.
 */
static ADDRESS *g_unary P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap;

    switch (ep->etp->type) {
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
	ap = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	g_code (op, (ILEN) ep->etp->size, ap, NIL_ADDRESS);
	return mk_legal (ap, flags, ep->etp);
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	switch (op) {
	case op_neg:
	    g_code (op, IL2, mk_low (ap), NIL_ADDRESS);
	    g_code (op_adc, IL2, mk_immed (0L), mk_high (ap));
	    g_code (op, IL2, mk_high (ap), NIL_ADDRESS);
	    break;
	case op_not:
	    g_code (op, IL2, mk_low (ap), NIL_ADDRESS);
	    g_code (op, IL2, mk_high (ap), NIL_ADDRESS);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	return mk_legal (ap, flags, ep->etp);
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap = g_expr (ep->v.p[0], F_FREG);
	g_fcode (op_fchs, IL0, NIL_ADDRESS, NIL_ADDRESS);
	return mk_legal (ap, flags, ep->etp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_unary", "illegal type or operation"));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate code to evaluate a autoincrement/autodecrement node
 */
static ADDRESS *g_aincdec P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;

#ifdef FLOAT_IEEE
    ILEN    ilen;

#endif /* FLOAT_IEEE */
    switch (ep->etp->type) {
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_pointer16:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}
	if (flags & F_NOVALUE) {
	    ap2 = g_expr (ep->v.p[0], F_ALL);
	    ap1 = NIL_ADDRESS;
	} else {
	    ap1 = data_register ();
	    ap2 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG));
	    validate (ap1);
	    g_code (op_mov, (ILEN) ep->etp->size, ap2, ap1);
	}
	g_code (op, (ILEN) ep->etp->size, mk_immed (ep->v.p[1]->v.i), ap2);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	if (flags & F_NOVALUE) {
	    ap1 = g_expr (ep->v.p[0], F_ALL);
	    g_code (op, IL2, mk_immed (1L), mk_low (ap1));
	    op = ((op == op_add) ? op_adc : op_sbb);
	    g_code (op, IL2, mk_immed (0L), mk_high (ap1));
	} else {
	    ap1 = mdata_register ();
	    ap2 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG));
	    validate (ap1);
	    g_code (op_mov, IL2, mk_low (ap2), mk_low (ap1));
	    g_code (op_mov, IL2, mk_high (ap2), mk_high (ap1));
	    g_code (op, IL2, mk_immed (1L), mk_low (ap2));
	    op = ((op == op_add) ? op_adc : op_sbb);
	    g_code (op, IL2, mk_immed (0L), mk_high (ap2));
	    freeop (ap2);
	}
	return mk_legal (ap1, flags, ep->etp);
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	if (flags & F_NOVALUE) {
	    return g_asbin (ep, flags, op);
	}
	ap1 = g_expr (ep->v.p[0], F_MEM);
	validate (ap1);
	ilen = (ILEN) ep->etp->size;
	g_fcode (op_fld, ilen, ap1, NIL_ADDRESS);
	ap2 = g_expr (ep->v.p[1], F_FREG);
	validate (ap1);
	switch (op) {
	case op_add:
	    g_fcode (op_fadd, ilen, ap1, NIL_ADDRESS);
	    break;
	case op_sub:
	    g_fcode (op_fsubr, ilen, ap1, NIL_ADDRESS);
	    break;
	default:
	    FATAL ((__FILE__, "g_aincdec", "illegal op %d", op));
	}
	freeop (ap2);
	freeop (ap1);
	g_fcode (op_fstp, ilen, ap1, NIL_ADDRESS);
	ap1 = float_register ();
	return mk_legal (ap1, flags, ep->etp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_aincdec", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}


/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */
static ADDRESS *g_bin P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;
    TYP    *tp = ep->etp;
    OPCODE  op2;
    FLAGS   f = F_ALL;

    switch (tp->type) {
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
	f = (FLAGS) (f | F_NOEDI);
	/*lint -fallthrough */
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_pointer16:
	if (op == op_imul) {
	    f &= ~F_IMMED;
	}
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	ap2 = g_expr (ep->v.p[1], f);
	validate (ap1);
	g_code (op, (ILEN) tp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    case bt_int32:
    case bt_long:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	switch (op) {
	case op_add:
	    op2 = op_adc;
	    goto common;
	case op_sub:
	    op2 = op_sbb;
	    goto common;
	case op_and:
	case op_or:
	case op_xor:
	    op2 = op;
	  common:
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	    ap2 = g_expr (ep->v.p[1], f);
	    validate (ap1);
	    g_code (op, IL2, mk_low (ap2), mk_low (ap1));
	    g_code (op2, IL2, mk_high (ap2), mk_high (ap1));
	    freeop (ap2);
	    return mk_legal (ap1, flags, tp);
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	break;
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap1 = g_expr (ep->v.p[0], F_MEM);
	ap2 = g_expr (ep->v.p[1], F_FREG);
	validate (ap1);
	switch (op) {
	case op_add:
	    op = op_fadd;
	    break;
	case op_sub:
	    op = op_fsubr;
	    break;
	case op_imul:
	    op = op_fmul;
	    break;
	case op_idiv:
	    op = op_fdivr;
	    break;
	default:
	    FATAL ((__FILE__, "g_bin", "illegal op %d", op));
	}
	g_fcode (op, (ILEN) tp->size, ap1, NIL_ADDRESS);
	freeop (ap2);
	freeop (ap1);
	ap1 = float_register ();
	return mk_legal (ap1, flags, ep->etp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_bin", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate code to evaluate a binary as-node
 * the result.
 */
static ADDRESS *g_asbin P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2, *ap3;
    FLAGS   flagx = F_NONE;

#ifdef FLOAT_IEEE
    ILEN    ilen;

#endif /* FLOAT_IEEE */
    switch (ep->etp->type) {
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
	flagx = (FLAGS) (flagx | F_NOEDI);
	/*lint -fallthrough */
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_pointer16:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}
	if (flags & F_NOVALUE) {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (flagx | F_ALL));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (flagx | F_DREG | F_IMMED));
	    validate (ap1);
	    g_code (op, (ILEN) ep->etp->size, ap2, ap1);
	    freeop (ap2);
	    freeop (ap1);
	    /* void result */
	    ap1 = NIL_ADDRESS;
	} else {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (flagx | F_MEM | F_DREG));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (flagx | F_ALL));
	    validate (ap1);
	    ap3 = data_register ();
	    g_code (op_mov, (ILEN) ep->etp->size, ap1, ap3);
	    g_code (op, (ILEN) ep->etp->size, ap2, ap3);
	    g_code (op_mov, (ILEN) ep->etp->size, ap3, ap1);
	    freeop (ap3);
	    freeop (ap2);
	    freeop (ap1);
	    /* need result */
	    ap1 = data_register ();
	    g_code (op_mov, IL2, ap3, ap1);
	    ap1 = mk_legal (ap1, flags, ep->etp);
	}
	return ap1;
    case bt_int32:
    case bt_long:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	if (flags & F_NOVALUE) {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (flagx | F_ALL));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (flagx | F_DREG | F_IMMED));
	    validate (ap1);
	    switch (op) {
	    case op_add:
		g_code (op, IL2, mk_low (ap2), mk_low (ap1));
		g_code (op_adc, IL2, mk_high (ap2), mk_high (ap1));
		break;
	    case op_sub:
		g_code (op, IL2, mk_low (ap2), mk_low (ap1));
		g_code (op_sbb, IL2, mk_high (ap2), mk_high (ap1));
		break;
	    case op_or:
	    case op_xor:
	    case op_and:
		g_code (op, IL2, mk_low (ap2), mk_low (ap1));
		g_code (op, IL2, mk_high (ap2), mk_high (ap1));
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	    freeop (ap2);
	    freeop (ap1);
	    /* void result */
	    ap1 = NIL_ADDRESS;
	} else {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (flagx | F_DREG));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (flagx | F_ALL));
	    validate (ap1);
	    switch (op) {
	    case op_add:
		g_code (op, IL2, mk_low (ap2), mk_low (ap1));
		g_code (op_adc, IL2, mk_high (ap2), mk_high (ap1));
		break;
	    case op_sub:
		g_code (op, IL2, mk_low (ap2), mk_low (ap1));
		g_code (op_sbb, IL2, mk_high (ap2), mk_high (ap1));
		break;
	    case op_or:
	    case op_xor:
	    case op_and:
		g_code (op, IL2, mk_low (ap2), mk_low (ap1));
		g_code (op, IL2, mk_high (ap2), mk_high (ap1));
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	    freeop (ap2);
	    ap1 = mk_legal (ap1, flags, ep->etp);
	}
	return ap1;
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap1 = g_expr (ep->v.p[0], F_MEM);
	ap2 = g_expr (ep->v.p[1], F_FREG);
	validate (ap1);
	ilen = (ILEN) ep->etp->size;
	switch (op) {
	case op_add:
	    op = op_fadd;
	    break;
	case op_sub:
	    op = op_fsubr;
	    break;
	case op_imul:
	    op = op_fmul;
	    break;
	case op_idiv:
	    op = op_fdivr;
	    break;
	default:
	    FATAL ((__FILE__, "g_asbin", "illegal op %d", op));
	}
	g_fcode (op, ilen, ap1, NIL_ADDRESS);
	freeop (ap2);
	freeop (ap1);
	if (flags & F_NOVALUE) {
	    g_fcode (op_fstp, ilen, ap1, NIL_ADDRESS);
	    ap1 = NIL_ADDRESS;
	} else {
	    g_fcode (op_fst, ilen, ap1, NIL_ADDRESS);
	    ap1 = float_register ();
	}
	return mk_legal (ap1, flags, ep->etp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_asbin", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*---------------------------------------------------------------------------*/

/*
 * generate code to evaluate a shift node
 */
static ADDRESS *g_shift P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    EXPR   *ep1;
    ADDRESS *ap1, *ap2;
    FLAGS   flgs;

    switch (ep->etp->type) {
    case bt_int16:
    case bt_char:
    case bt_schar:
    case bt_short:
	if (op == op_shr) {
	    op = op_asr;
	}
	/*lint -fallthrough */
    case bt_pointer16:
    case bt_uint16:
    case bt_uchar:
    case bt_charu:
    case bt_ushort:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL | F_NOECX));
	ep1 = ep->v.p[1];
	if (target_option == target_8086 &&
	    (ep1->nodetype == en_icon && ep1->v.i == 2)) {
	    ap2 = mk_immed (1L);
	    g_code (op, (ILEN) ep->etp->size, ap2, ap1);
	} else {
	    if (target_option >= target_80186 ||
		(ep1->nodetype == en_icon && ep1->v.i == 1)) {
		flgs = (FLAGS) (F_DREG | F_IMMED | F_ECX);
	    } else {
		flgs = (FLAGS) (F_DREG | F_ECX);
	    }
	    ap2 = g_expr (ep->v.p[1], flgs);
	}
	validate (ap1);
	g_code (op, (ILEN) ep->etp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    default:
	FATAL ((__FILE__, "g_shift", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate code to evaluate an assign shift node
 */
static ADDRESS *g_asshift P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;
    EXPR   *ep1;    FLAGS   flgs;   // Added for new code below - WSF


    switch (ep->etp->type) {
    case bt_int16:
    case bt_short:
    case bt_char:
    case bt_schar:
	if (op == op_shr) {
	    op = op_asr;
	}
	/*lint -fallthrough */
    case bt_pointer16:
    case bt_uint16:
    case bt_uchar:
    case bt_charu:
    case bt_ushort:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}

	// The following code replaces the above code, which generated invalid
	// code for the 8086 (i.e., uses an immediate operand). The code was 
	// taken from the g_shift() function. - WSF
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL | F_NOECX));
	ep1 = ep->v.p[1];
		flgs = (FLAGS) (F_DREG | F_ECX);
	    ap2 = g_expr (ep->v.p[1], flgs);
	validate (ap1);
	g_code (op, (ILEN) ep->etp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    //

/*
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG | F_NOECX));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED | F_ECX));
	validate (ap1);
	g_code (op, (ILEN) ep->etp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
*/
    default:
	FATAL ((__FILE__, "g_asshift", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;


}

/*
 * generate code to evaluate a divide node (mod==0) or mod node (mod==1)
 */
static ADDRESS *g_div P3 (const EXPR *, ep, FLAGS, flags, BOOL, mod)
{
    ADDRESS *ap1, *ap2;
    OPCODE  op = op_div;

    switch (ep->etp->type) {
    case bt_char:
    case bt_schar:
	op = op_idiv;
	/*lint -fallthrough */
    case bt_uchar:
    case bt_charu:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_EAXEDX | F_VOL));
	ap1 = g_extend2 (ap1, ep->etp); 
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_MEM));
	validate (ap1);
	g_code (op, IL1, ap2, NIL_ADDRESS);
	freeop (ap2);
	freeop (ap1);
	ap1 = data_register ();
	if (mod) {
	    g_code (op_mov, IL1, &ah_reg, ap1);
	} else {
            g_code (op_mov, IL1, &eax_reg, ap1);
	}
	return mk_legal (ap1, flags, ep->etp);
    case bt_short:
    case bt_int16:
	op = op_idiv;
	/*lint -fallthrough */
    case bt_uint16:
    case bt_ushort:
    case bt_pointer16:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_EAXEDX | F_VOL));
	ap1 = g_extend2 (ap1, ep->etp);
	ap2 = g_expr (ep->v.p[1], F_DREG | F_MEM);
	validate (ap1);
	g_code (op, IL2, ap2, NIL_ADDRESS);
	freeop (ap2);
	freeop (ap1);
	ap1 = data_register ();
	if (mod) {
	    g_code (op_mov, IL2, &edx_reg, ap1);
	} else {
	    g_code (op_mov, IL2, &eax_reg, ap1);
	}
	return mk_legal (ap1, flags, ep->etp);
    default:
	return g_bin (ep, flags, (is_signed_type (ep->etp) ? op_idiv : op_div));
    }
}

/*
 * generate code for /= and %= node
 */
static ADDRESS *g_asdiv P3 (const EXPR *, ep, FLAGS, flags, BOOL, mod)
{
    ADDRESS *ap1, *ap2, *ap3;
    OPCODE  op = op_idiv;

    switch (ep->etp->type) {
    case bt_charu:
    case bt_uchar:
	op = op_div;
    case bt_char:
    case bt_schar:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, mod);
	}

	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG | F_EAXEDX));
	if (ap1->mode == am_dreg)
	{
	    ap1 = g_extend2 (ap1, ep->etp);
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_MEM));
	    validate (ap1);
	    g_code (op, IL1, ap2, NIL_ADDRESS);
	    if (mod) {
		g_code (op_mov, IL1, &ah_reg, ap1);
	    } else {
		g_code (op_mov, IL1, &eax_reg, ap1);
	    }
	    freeop (ap2);
	} else {
	    ap3 = ax_register ();
	    g_code (op_mov, IL1, ap1, ap3);
	    ap3 = g_extend2 (ap3, ep->etp);
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_MEM));
	    validate (ap3);
	    validate (ap1);
	    g_code (op, IL1, ap2, NIL_ADDRESS);
	    freeop (ap2);
	    if (mod) {
		g_code (op_mov, IL1, &ah_reg, &eax_reg);
	    }
	    g_code (op_mov, IL1, &eax_reg, ap1);
	    freeop (ap3);
	}
	return mk_legal (ap1, flags, ep->etp);
    case bt_ushort:
    case bt_uint16:
    case bt_pointer16:
	op = op_div;
	/*lint -fallthrough */
    case bt_int16:
    case bt_short:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, mod);
	}
	
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG | F_EAXEDX));
	if (ap1->mode == am_mreg) {
	    ap1 = g_extend2 (ap1, ep->etp);
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_MEM));
	    validate (ap1);
	    g_code (op, (ILEN) ep->etp->size, ap2, NIL_ADDRESS);
	    if (mod) {
		g_code (op_mov, IL2, mk_high(ap1), mk_low (ap1));
	    }
	    freeop (ap2);
	} else {
	    ap3 = axdx_register ();
	    g_code (op_mov, (ILEN) ep->etp->size, ap1, mk_low (ap3));
	    ap3 = g_extend2 (ap3, ep->etp);
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_MEM));
	    validate (ap3);
	    g_code (op, (ILEN) ep->etp->size, ap2, NIL_ADDRESS);
	    freeop (ap2);
	    validate (ap1);
	    if (mod) {
		g_code (op_mov, IL2, mk_high(ap3), mk_low (ap1));
	    } else {
    		g_code (op_mov, IL2, mk_low(ap3), mk_low (ap1));
	    }
	    freeop (ap3);
	}
	return mk_legal (ap1, flags, ep->etp);
    default:
	return g_asbin (ep, flags, op_idiv);
    }
}

/*
 * generate code to evaluate a multiply node
 */
static ADDRESS *g_mul P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;
    OPCODE  op = op_imul;

    switch (ep->etp->type) {
    case bt_uchar:
    case bt_charu:
    case bt_char:
    case bt_schar:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	ap1 = g_extend2 (ap1, ep->etp);
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_MEM | F_DREG | F_NOEDI));
	validate (ap1);	/* in case push occurred */
	if ((ap2->mode == am_dreg) && (ap2->preg == EAX)) {
	    g_code (op, IL2, ap1, NIL_ADDRESS);
	    g_code (op_mov, IL1, ap1, &eax_reg);
	} else if ((ap1->mode == am_dreg) && (ap1->preg == EAX)) {
	    g_code (op, IL2, ap2, NIL_ADDRESS);
	} else {
	    ap1 = mk_legal (ap1, flags | F_EAXEDX, ep->etp);
	    g_code (op, IL2, ap2, NIL_ADDRESS);
	}
	freeop (ap2);
	freeop (ap1);
	ap2 = data_register ();
	g_code (op_mov, IL1, mk_low (ap1), ap2);
	return mk_legal (ap2, flags, ep->etp);
    case bt_uint16:
    case bt_ushort:
    case bt_pointer16:
	/*
	 * unless the result is extended to 32 bit, there should
	 * be no difference between imul and mul.
	 * NB. There is no mul instruction unless 32-bit extension
	 * is desired.
	 */
	/* op = op_mul; >>>This instruction does not exist<<< */
	/*lint -fallthrough */
    case bt_int16:
    case bt_short:
	if (target_option >= target_80186 && is_icon (ep->v.p[1])) {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	    ap2 = g_expr (ep->v.p[1], F_IMMED);
	    g_code (op, (ILEN) ep->etp->size, ap2, mk_low (ap1));
	    freeop (ap2);
	    return mk_legal (ap1, flags, ep->etp);
	} else {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_EAXEDX | F_VOL));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_MEM));
	    validate (ap1);	/* in case push occurred */
	    g_code (op, (ILEN) ep->etp->size, ap2, NIL_ADDRESS);
	    freeop (ap2);
	    freeop (ap1);
	    ap2 = data_register ();
	    g_code (op_mov, IL2, mk_low (ap1), ap2);
	    return mk_legal (ap2, flags, ep->etp);
	}
    default:
	return g_bin (ep, flags, op_imul);
    }
}

/*
 * generate code for *= node
 */
static ADDRESS *g_asmul P2 (const EXPR *, ep, FLAGS, flags)
{
#if 1
    ADDRESS *ap1, *ap2, *ap3;

    /* Note: the results of imul should be the same as mul for the needed cases. */

    switch (ep->etp->type) {
    case bt_charu:
    case bt_uchar:
    case bt_char:
    case bt_schar:
    case bt_ushort:
    case bt_uint16:
    case bt_pointer16:
    case bt_short:
    case bt_int16:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op_imul, FALSE);
	}
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_VOL));
	validate (ap1);
	if ((ap2->mode == am_dreg) && (ap2->preg == EAX)) {
	    g_code (op_imul, (ILEN) ep->etp->size, ap1, NIL_ADDRESS);
	    g_code (op_mov, (ILEN) ep->etp->size, &eax_reg, ap1);
	} else if ((ap1->mode == am_dreg) && (ap1->preg == EAX)) {
	    g_code (op_imul, (ILEN) ep->etp->size, ap2, NIL_ADDRESS);
	    g_code (op_mov, (ILEN) ep->etp->size, ap1, ap2);
	} else {
	    ap3 = data_register ();
	    g_code (op_mov, (ILEN) ep->etp->size, ap3, ap1);
	    g_code (op_imul, (ILEN) ep->etp->size, ap2, NIL_ADDRESS);
	    g_code (op_mov, (ILEN) ep->etp->size, ap1, ap3);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    default:
	return g_asbin (ep, flags, op_imul);
    }
#else
    OPCODE  op;
    ILEN    size = IL0;
    FLAGS   f = (FLAGS) (F_MEM | F_DREG);
    ADDRESS *ap1, *ap2, *ap3;

    switch (ep->etp->type) {
    case bt_charu:
    case bt_uchar:
	op = op_movzbl;
	f = (FLAGS) (f | F_NOEDI);
	goto common;
    case bt_char:
    case bt_schar:
	op = op_movsbl;
	f = (FLAGS) (f | F_NOEDI);
	goto common;
    case bt_ushort:
    case bt_uint16:
    case bt_pointer16:
    case bt_short:
    case bt_int16:
	op = op_mov;
	size = IL2;
      common:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op_imul, FALSE);
	}
	ap1 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_VOL));
	if (size == IL0) {
	    g_code (op, size, ap1, ap1);
	}
	ap2 = g_expr (ep->v.p[0], f);
	ap3 = data_register ();
	validate (ap1);
	g_code (op, size, ap2, ap3);
	g_code (op_imul, IL2, ap3, ap1);
	g_code (op_mov, (ILEN) ep->etp->size, ap1, ap2);
	freeop (ap3);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    default:
	return g_asbin (ep, flags, op_imul);
    }
#endif
}

/*
 * generate code to evaluate a condition operator node (?:)
 */
static ADDRESS *g_hook P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;
    LABEL   false_label, end_label;
    TYP    *tp = ep->etp;
    SIZE    offset;
    FLAGS   flagx;

#ifndef NDEBUG
    BOOL    result_is_void = FALSE;

#endif /* NDEBUG */

    switch (ep->etp->type) {
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	flagx = F_FREG;
	break;
#endif /* FLOAT_IEEE */
    case bt_void:
#ifndef NDEBUG
	result_is_void = TRUE;
#endif /* NDEBUG */
	flagx = (FLAGS) (F_ALL | F_NOVALUE);
	break;
    case bt_struct:
    case bt_union:
	tp = tp_pointer;
	/*FALLTHROUGH */
    default:
	flagx = (FLAGS) (F_DREG | F_VOL);
    }

    false_label = nextlabel++;
    end_label = nextlabel++;

    temp_inv ();		/* I do not think I can avoid that */
    offset = stack_offset;
    stack_offset = 0L;

    /* all registers are void */

    g_falsejp (ep->v.p[0], false_label);
    ep = ep->v.p[1];

    /* all registers are void */

    ap1 = g_expr (ep->v.p[0], flagx);
#ifdef FLOAT_IEEE
    if (flagx == F_MEM) {
	ADDRESS *ap;

	freeop (ap1);
	ap = address_register ();
	g_code (op_lea, IL2, ap1, ap);
	ap1 = copy_addr (ap, am_ind);
    }
#endif /* FLOAT_IEEE */
    freeop (ap1);

    /* all registers are void */

    g_branch (end_label);
    g_label (false_label);

    ap2 = g_expr (ep->v.p[1], flagx);
#ifdef FLOAT_IEEE
    if (flagx == F_MEM) {
	ADDRESS *ap;

	freeop (ap2);
	ap = address_register ();
	g_code (op_lea, IL2, ap2, ap);
	ap1 = copy_addr (ap, am_ind);
    }
#endif /* FLOAT_IEEE */

    assert (result_is_void || is_equal_address (ap1, ap2));

    g_label (end_label);

    g_stack (stack_offset);
    stack_offset = offset;
    return mk_legal (ap2, flags, tp);
}

/*
 * Generate the code for assign operators in bitfields
 */
static ADDRESS *g_asbitfield P4 (const EXPR *, ep, FLAGS, flags, OPCODE, op,
				 BOOL, mod)
{
    ADDRESS *ap1, *ap2, *ap3;
    EXPR   *lnode = ep->v.p[0];
    int     width = (int) lnode->v.bit.width;
    int     offset = (int) lnode->v.bit.offset;
    ILEN    ilen = (ILEN) ep->etp->size;
    UVAL    mask;

    /* Evaluate the address of the LHS */
    ap2 = g_expr (lnode->v.bit.p, (FLAGS) (F_AREG));
    ap2 = copy_addr (ap2, am_ind);

    /* Now get the value of the LHS, rotate and mask out unwanted bits */
    ap1 = data_register ();
    g_code (op_mov, ilen, ap2, ap1);
    g_rotate (ap1, ilen, offset, lnode->etp, width);

    /* now do the operation, masking the result back into the required size */
    switch (op) {
    case op_div:
    case op_idiv:
	/* evaluate the RHS */
	ap3 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED | F_ECX));
	validate (ap1);

	if (uses_temp (ap3) || ap3->mode == am_immed) {
/*KDW */
	    g_code (op_mov, IL2, ap3, &ecx_reg);
	    freeop (ap3);
	    ap3 = &ecx_reg;
	}
	g_code (op_mov, IL2, ap1, &eax_reg);
	g_extend2 (ap1, ep->etp);
	g_code (op, IL2, ap3, NIL_ADDRESS);
	if (mod) {
	    g_code (op_mov, IL2, &edx_reg, ap1);
	} else {
	    g_code (op_mov, IL2, &eax_reg, ap1);
	}
	break;
    case op_asr:
    case op_shr:
    case op_shl:
	/* evaluate the RHS */
	ap3 = g_expr (ep->v.p[1], (FLAGS) (F_ALL | F_ECX));
	validate (ap1);

	if (ap3->mode != am_immed) {
	    g_code (op_mov, ilen, ap3, &ecx_reg);
	    g_code (op, ilen, &ecx_reg, ap1);
	    freeop (ap3);
	    break;
	}
	/*lint -fallthrough */
    default:
	/* evaluate the RHS */
	ap3 = g_expr (ep->v.p[1], F_ALL);
	validate (ap1);

	g_code (op, ilen, ap3, ap1);
	freeop (ap3);
    }
    mask = (UVAL) bitmask ((BITSIZE) width);
    g_code (op_and, ilen, mk_immed ((IVAL) mask), ap1);

    /* rotate result back into position, and store */
    g_rotate (ap1, ilen, -offset, tp_void, 0);
    validate (ap2);
    g_code (op_and, ilen, mk_immed ((IVAL) ~(mask << offset)), ap2);
    g_code (op_or, ilen, ap1, ap2);
    freeop (ap1);
    freeop (ap2);

    /* return a result */
    ap2 = data_register ();
    g_code (op_mov, ilen, ap1, ap2);
    ap1 = ap2;

    if (!(flags & F_NOVALUE)) {
	/* result value needed */
	g_rotate (ap1, ilen, offset, tp_void, 0);
	if (mod) {
	    /* post increment/decrement restore original value */
	    switch (op) {
	    case op_add:
		g_code (op_sub, ilen, mk_immed (1L), ap1);
		g_code (op_and, ilen, mk_immed ((IVAL) mask), ap1);
		break;
	    case op_sub:
		g_code (op_add, ilen, mk_immed (1L), ap1);
		g_code (op_and, ilen, mk_immed ((IVAL) mask), ap1);
		break;
	    default:
		break;
	    }
	}
    }
    return mk_legal (ap1, flags, ep->etp);
}

/*
 * assign structures: ap1=dest, ap2=source
 */
static void structassign P3 (ADDRESS *, ap1, ADDRESS *, ap2, SIZE, size)
{
    ADDRESS *ap3;

    if (size == 2l) {
	if (ap1->mode == am_dreg) {
	    ap1 = mk_indirect (ap1->preg, NIL_EXPR);
	} else {
	    g_code (op_mov, IL2, ap1, &edi_reg);
	    ap1 = mk_indirect (EDI, NIL_EXPR);
	}
	if (ap2->mode == am_dreg) {
	    ap1 = mk_indirect (ap2->preg, NIL_EXPR);
	} else {
	    g_code (op_mov, IL2, ap2, &esi_reg);
	    ap2 = mk_indirect (ESI, NIL_EXPR);
	}
	ap3 = data_register ();
	g_code (op_mov, IL2, ap2, ap3);
	g_code (op_mov, IL2, ap3, ap1);
	freeop (ap3);
    } else {
	g_code (op_mov, IL2, ap1, &edi_reg);
	g_code (op_mov, IL2, ap2, &esi_reg);
	if (size % 2l) {
	    ap3 = mk_immed (size);
	    ap3 = mk_legal (ap3, (FLAGS) (F_DREG | F_ECX), tp_char);
	    g_code (op_rep, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_smov, IL1, NIL_ADDRESS, NIL_ADDRESS);
	} else {
	    ap3 = mk_immed ((IVAL) ((USIZE) size >> 1));
	    ap3 = mk_legal (ap3, (FLAGS) (F_DREG | F_ECX), tp_int);
	    g_code (op_rep, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_smov, IL2, NIL_ADDRESS, NIL_ADDRESS);
	}
	freeop (ap3);
    }
}

/*
 * generate code for an assignment node.
 */
static ADDRESS *g_assign P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2, *ap3;
    TYP    *tp = ep->etp;
    SIZE    size = tp->size;
    UVAL    mask;
    FLAGS   flagx = (flags & F_NOVALUE) ? F_ALL : F_ALL;

    switch (tp->type) {
    case bt_float:
    case bt_double:
    case bt_longdouble:
#ifdef FLOAT_IEEE
	if (fpu_option) {
	    ap1 = g_expr (ep->v.p[0], F_MEM);
	    ap2 = g_expr (ep->v.p[1], F_FREG);
	    validate (ap1);
	    g_fcode ((flags & F_NOVALUE ? op_fstp : op_fst), (ILEN) size, ap1,
		     NIL_ADDRESS);
	    freeop (ap2);
	    freeop (ap1);
	    ap1 = flags & F_NOVALUE ? NIL_ADDRESS : float_register ();
	    return mk_legal (ap1, flags, tp);
	}
	/*lint -fallthrough */
#endif /* FLOAT_IEEE */
	ap2 = g_expr (ep->v.p[1], F_MEM);
	ap1 = g_expr (ep->v.p[0], F_MEM);
	validate (ap2);
	structassign (ap1, ap2, size);
	freeop (ap1);
	return mk_legal (ap2, flags, tp_pointer);
    case bt_pointer16:
    case bt_pointer32:
	if (!is_array_type (tp) && !is_array_assignment (tp)) {
	    goto common;	    /*lint !e801*/  /* use of goto is deprecated*/
	}
	/*lint -fallthrough */
    case bt_floatcomplex:
    case bt_doublecomplex:
    case bt_longdoublecomplex:
    case bt_struct:
    case bt_union:
	ap2 = g_expr (ep->v.p[1], F_MEM);
	ap1 = g_expr (ep->v.p[0], F_MEM);
	validate (ap2);
	structassign (ap1, ap2, size);
	freeop (ap1);
	return mk_legal (ap2, flags, tp_pointer);
#if 0
    case bt_int32:
    case bt_long:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	ap1 = g_expr (ep->v.p[1], flagx);
	ap2 = g_expr (ep->v.p[0], F_ALL);
	validate (ap1);
	g_code (op_mov, IL2, mk_low (ap1), mk_low (ap2));
	g_code (op_mov, IL2, mk_high (ap1), mk_high (ap2));
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#endif
    default:
common:
	switch (ep->v.p[0]->nodetype) {
	case en_fieldref:
	    /*
	     * Field assignment
	     */
	    /* get the value */
	    mask = bitmask (ep->v.p[0]->v.bit.width);
	    ap1 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_VOL));
	    if (ap1->mode == am_immed) {
		ap1->u.offset->v.i &= (IVAL) mask;
		ap3 = mk_immed ((IVAL)
				(ap1->u.offset->v.u << (int) ep->v.p[0]->v.bit.offset));
	    } else {
		if (flags & F_NOVALUE) {
		    ap3 = ap1;
		    g_code (op_and, (ILEN) size, mk_immed ((IVAL) mask), ap3);
		} else {
		    if (is_signed_type (tp)) {
			SIZE    i = tp_int->size * 8L -
			    (SIZE) ep->v.p[0]->v.bit.width -

			    (SIZE) ep->v.p[0]->v.bit.offset;

			g_code (op_asl, (ILEN) size, mk_immed (i), ap1);
			g_code (op_asr, (ILEN) size, mk_immed (i), ap1);
			ap3 = data_register ();
			g_code (op_mov, IL4, ap1, ap3);
			g_code (op_and, (ILEN) size, mk_immed ((IVAL) mask),
				ap3);
		    } else {
			g_code (op_and, (ILEN) size, mk_immed ((IVAL) mask),
				ap1);
			ap3 = data_register ();
			g_code (op_mov, IL4, ap1, ap3);
		    }
		}
		g_rotate (ap3, (ILEN) size, -(int) ep->v.p[0]->v.bit.offset,
			  tp_void, 0);
	    }
	    mask <<= (int) ep->v.p[0]->v.bit.offset;
	    ap2 = g_deref (ep->v.p[0]->v.bit.p, ep->v.p[0]->etp);
	    validate (ap3);
	    g_code (op_and, (ILEN) size, mk_immed ((IVAL) ~mask), ap2);
	    g_code (op_or, (ILEN) size, ap3, ap2);
	    freeop (ap2);
	    if (!(flags & F_NOVALUE)) {
		freeop (ap3);
		validate (ap1);
	    }
	    break;
	    /*
	     * (uns.) char, (uns.) short, (uns.) long, float
	     *
	     * we want to pass the right hand side as the expression value.
	     * This can''t be done if the left side is a register variable on
	     * which the right hand side addressing mode depends. But if the
	     * left side IS a register variable, it is desirable to pass the
	     * left side, so no problem.
	     */
	case en_register:
	    /* pass the left side as expr. value */
	    ap1 = g_expr (ep->v.p[0], F_ALL);
	    ap2 = g_expr (ep->v.p[1], F_ALL);
	    validate (ap1);
	    g_code (op_mov, (ILEN) size, ap2, ap1);
	    freeop (ap2);
	    break;
	default:
	    /* pass the right side as expr. value */
	    /* normally, this is more efficient */
	    ap1 = g_expr (ep->v.p[1], flagx);
	    ap2 = g_expr (ep->v.p[0], F_ALL);
	    validate (ap1);
	    if (is_register_mode (ap1->mode) || is_register_mode (ap2->mode)
		|| (ap1->mode == am_immed)) {
		if (size == 4L) {
		    g_code (op_mov, IL2, mk_low (ap1), mk_low (ap2));
		    g_code (op_mov, IL2, mk_high (ap1), mk_high (ap2));
		} else {
		    g_code (op_mov, (ILEN) size, ap1, ap2);
		}
		freeop (ap2);
	    } else {
		if (flags & F_NOVALUE) {
		    ap3 = data_register ();
		    if (size == 4L) {
			g_code (op_mov, IL2, mk_low (ap1), ap3);
			g_code (op_mov, IL2, ap3, mk_low (ap2));
			g_code (op_mov, IL2, mk_high (ap1), ap3);
			g_code (op_mov, IL2, ap3, mk_high (ap2));
		    } else {
			g_code (op_mov, size, ap1, ap3);
			g_code (op_mov, size, ap3, ap2);
		    }
		    freeop (ap3);
		    freeop (ap2);
		    freeop (ap1);
		    ap1 = NIL_ADDRESS;
		} else {
		    if (size == 4L) {
			ap3 = mdata_register ();
			g_code (op_mov, IL2, mk_low (ap1), mk_low (ap3));
			g_code (op_mov, IL2, mk_low (ap3), mk_low (ap2));
			g_code (op_mov, IL2, mk_high (ap1), mk_high (ap3));
			g_code (op_mov, IL2, mk_high (ap3), mk_high (ap2));
			freeop (ap3);
			freeop (ap2);
			freeop (ap1);
			ap1 = mdata_register ();
			g_code (op_mov, (ILEN) size, mk_low (ap3),
				mk_low (ap1));
			g_code (op_mov, (ILEN) size, mk_high (ap3),
				mk_high (ap1));
		    } else {
			ap3 = data_register ();
			g_code (op_mov, IL2, ap1, ap3);
			g_code (op_mov, IL2, ap3, ap2);
			freeop (ap3);
			freeop (ap2);
			freeop (ap1);
			ap1 = data_register ();
			g_code (op_mov, (ILEN) size, ap3, ap1);
		    }
		}
	    }
	    break;
	}
	return mk_legal (ap1, flags, tp);
    }
}

/*
 * push the operand expression onto the stack. return the number of bytes
 * pushed
 */
static SIZE push_param P1 (const EXPR *, ep)
{
    ADDRESS *ap;
    FLAGS   flagx;

#ifdef FLOAT_IEEE
    ADDRESS *ap1;

#endif /* FLOAT_IEEE */
    SIZE    size = ep->etp->size;

    switch (ep->etp->type) {
    case bt_float:
    case bt_double:
    case bt_longdouble:
#ifdef FLOAT_IEEE
	ap = g_expr (ep, F_FREG);
	g_code (op_sub, IL2, mk_immed (size), &esp_reg);
	ap1 = mk_indirect (ESP, NIL_EXPR);
	g_fcode (op_fstp, (ILEN) size, ap1, NIL_ADDRESS);
	freeop (ap);
	break;
#endif /* FLOAT_IEEE */
    case bt_struct:
    case bt_union:
	/* pushing of structures and unions */
	ap = g_expr (ep, F_ALL);
	g_code (op_sub, IL2, mk_immed (size), &esp_reg);
	structassign (&esp_reg, ap, size);
	freeop (ap);
	break;
    default:
	flagx =
	    (target_option <
	     target_80186) ? (FLAGS) (F_DREG | F_AREG | F_MEM) : F_ALL;
	switch (size) {
	case 1L:
	case 2L:
	    ap = g_expr (ep, flagx);
	    g_code (op_push, IL2, ap, NIL_ADDRESS);
	    freeop (ap);
	    size = 2;
	    break;
	case 4L:
	    ap = g_expr (ep, flagx);
	    g_code (op_push, IL2, mk_high (ap), NIL_ADDRESS);
	    g_code (op_push, IL2, mk_low (ap), NIL_ADDRESS);
	    freeop (ap);
	    break;
	default:
	    FATAL (
		   (__FILE__, "push_param",
		    "size(%ld) != 2, ep->etp->type=%d", size, ep->etp->type));
	    break;
	}
	break;
    }
    return size;
}

/*
 * push a list of parameters onto the stack and return the number of
 * bytes that the parameters occupy.
 */
static SIZE g_parms P1 (const EXPR *, ep)
{
    SIZE    size;

    is_parameter++;
    for (size = 0L; ep != NIL_EXPR; ep = ep->v.p[1]) {
	size += push_param (ep->v.p[0]);
    }
    is_parameter--;
    return size;
}

static ADDRESS *func_result P3 (FLAGS, flags, SIZE, bytes, TYP *, tp)
{
    ADDRESS *ap;

    stack_offset += bytes;
    if (is_parameter) {
	g_stack (bytes);
    }
    if (flags & F_NOVALUE) {
	return NIL_ADDRESS;
    }
    switch (tp->type) {
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap = float_register ();
	break;
#endif /* FLOAT_IEEE */
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	if (!(flags & F_DREG)) {
	    FATAL ((__FILE__, "func_result", "not F_DREG"));
	}
	ap = mdata_register ();
	g_code (op_mov, IL2, mk_reg (reg_usage->result->reg[0]), mk_low (ap));
	g_code (op_mov, IL2, mk_reg (reg_usage->result->reg[1]),
		mk_high (ap));
	break;
    default:
	if (flags & F_DREG) {
	    ap = data_register ();
	} else if (flags & F_AREG) {
	    ap = address_register ();
	} else {
	    FATAL (
		   (__FILE__, "func_result", "not F_DREG or F_AREG (%d)",
		    flags));
	}
	if (ap->preg != reg_usage->result->reg[0]) {
	    g_code (op_mov, IL2, &eax_reg, ap);
	}
	break;
    }
    return ap;
}

/*
 * generate a function call node and return the address mode of the result.
 */
static ADDRESS *g_fcall P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap, *ap1;
    SIZE    size;
    EXPR   *ep0 = ep->v.p[0];
    REG     reg;

    if (!is_parameter && ep->nodetype != en_call) {
	switch (stackopt_option) {

	case OPT_SAFE:
	    /*
	     *       no stack optimisation
	     */
	    g_stack (stack_offset);
	    break;

	case OPT_MINIMUM:
	    /*
	     *       "Minimum" stack optimisation.  Perform a stack optimisation
	     *       unless:
	     *       1.  The alloca() routine is called
	     *       2.  The function call is via a variable
	     *       3.  The function starts with an underscore character
	     */
	    if ((ep0->nodetype != en_nacon) ||
		(ep0->v.str[0] == (CHAR) '_') || (ep0->v.str == alloca_name)) {
		g_stack (stack_offset);
	    }
	    break;

	case OPT_AVERAGE:
	    /*
	     *       "Average" stack optimisation.   This will not suppress
	     *       the optimisation on encountering calls to functions
	     *       whose names begin with underscore.
	     */
	    if ((ep0->nodetype != en_nacon) || (ep0->v.str == alloca_name)) {
		g_stack (stack_offset);
	    }
	    break;

	case OPT_MAXIMUM:
	    /*
	     *       "Maximum" stack optimisation.   This will not suppress
	     *       the optimisation on encountering calls to functions
	     *       whose names begin with underscore or via a function
	     *       variable.
	     */
	    if ((ep0->nodetype == en_nacon) && (ep0->v.str == alloca_name)) {
		g_stack (stack_offset);
	    }
	    break;

	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
    }
    /* push any used registers */
    temp_inv ();
    for (reg = EAX; reg <= EBP; reg++) {
	if (regsave_option & MEMBER (reg)) {
	    g_code (op_push, IL2, mk_reg (reg), NIL_ADDRESS);
	}
    }
    size = g_parms (ep->v.p[1]);	/* generate parameters */
    /*
     * for functions returning a structure or a union, push a pointer to the
     * return value as additional argument The scratch space will be
     * allocated in the stack frame of the calling function.
     */
    if (is_structure_type (ep->etp)) {
	ap = mk_scratch (ep->etp->size);
	ap1 = address_register ();
	g_code (op_lea, IL2, ap, ap1);
	g_code (op_push, IL2, ap1, NIL_ADDRESS);
	freeop (ap1);
	freeop (ap);
	size += tp_pointer->size;
    }
    if (ep->nodetype == en_call) {
	size = 0L;
    }
    /* call the function */
    switch (ep0->nodetype) {
    case en_nacon:
    case en_labcon:
	ap = mk_direct (ep0);
	break;
    default:
	ap = g_expr (ep0, F_AREG);
	freeop (ap);
	ap = copy_addr (ap, ap->mode);
	ap->preg = REG16 (ap->preg);
	break;
    }
    g_code (op_call, IL0, ap, NIL_ADDRESS);
    for (reg = EBP + 1; reg > EAX; reg--) {
	if (regsave_option & MEMBER ((int) reg - 1)) {
	    g_stack (size);
	    size = 0L;
	    g_code (op_pop, IL2, mk_reg ((REG) ((int) reg - 1)), NIL_ADDRESS);
	}
    }
    ap = func_result (flags, size, ep->etp);
    g_stack(stack_offset);	// Added by WSF; Makes sure stack is restored after function call
							// This prevents problems when using the stack with inline assemby
    return mk_legal (ap, flags, ep->etp);
}

/*
 * generates code for a en_cast node
 */
static ADDRESS *g_cast P4 (ADDRESS *, ap, TYP *, tp1, TYP *, tp2, FLAGS,
			   flags)
{
    ADDRESS *ap1;

    if (flags & F_NOVALUE) {
	freeop (ap);
	return NIL_ADDRESS;
    }
    /*
     *       Casts to a narrower integer type are no-ops since the 8086 is
     *       low-endian to avoid code duplication, float/double is shared
     */
    switch (tp2->type) {
	/* type to cast to */
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	switch (tp1->type) {
	case bt_float:
	    if (tp2->type != bt_float && ap->mode == am_freg) {
		/*
		 *   A double or long double value which is in the FP
		 *   registers must be stored and then reloaded in order
		 *   to correctly round the bits of a float.  Although
		 *   this looks like a NULL operation is isn't!
		 */
		ap1 = mk_scratch (tp_float->size);
		g_fcode (op_fstp, IL4, ap1, NIL_ADDRESS);
		g_fcode (op_fld, IL4, ap1, NIL_ADDRESS);
		freeop (ap1);
	    }
	    /*lint -fallthrough */
	case bt_double:
	    ap = mk_legal (ap, F_FREG, tp1);
	    return mk_legal (ap, flags, tp2);
	case bt_uchar:
	case bt_schar:
	case bt_char:
	case bt_charu:
	    ap = g_cast (ap, tp1, tp_short, F_ALL);
	    /*lint -fallthrough */
	case bt_short:
	case bt_int16:
	    /*
	     * For the conversion signed short --> float/double, there is
	     * a 8087 instruction
	     */
	    switch (ap->mode) {
	    default:
		/*
		 *   FPU-code for a signed short that is in a register:
		 *   the value is written to the stack and loaded from
		 *   there since there is no direct path between CPU
		 *   and FPU registers.
		 */
		ap1 = mk_scratch (2L);
		g_code (op_mov, IL2, ap, ap1);
		g_fcode (op_fild, IL2, ap1, NIL_ADDRESS);
		freeop (ap1);
		break;
	    case am_direct:
	    case am_indx:
	    case am_indx2:
		g_fcode (op_fild, IL2, ap, NIL_ADDRESS);
		break;
	    }
	    freeop (ap);
	    ap = float_register ();
	    return mk_legal (ap, flags, tp2);
	case bt_pointer16:
	case bt_ushort:
	case bt_uint16:
	    ap = g_cast (ap, tp1, tp_long, F_ALL);
	    /*lint -fallthrough */
	case bt_int32:
	case bt_long:
	    /*
	     *       For the conversion signed long --> float/double, there is
	     *       a 8087 instruction
	     */
	    switch (ap->mode) {
	    default:
		/*
		 *   FPU-code for a signed short that is in a register:
		 *   the value is written to and stack and loaded from
		 *   there since there is no direct path between CPU
		 *   and FPU registers.
		 */
		ap1 = mk_scratch (4L);
		g_code (op_mov, IL2, mk_low (ap), mk_low (ap1));
		g_code (op_mov, IL2, mk_high (ap), mk_high (ap1));
		g_fcode (op_fild, IL4, ap1, NIL_ADDRESS);
		freeop (ap1);
		break;
	    case am_direct:
	    case am_indx:
	    case am_indx2:
		g_fcode (op_fild, IL4, ap, NIL_ADDRESS);
		break;
	    }
	    freeop (ap);
	    ap = float_register ();
	    return mk_legal (ap, flags, tp2);
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    ap = mk_legal (ap, F_DREG, tp1);
	    ap1 = mk_scratch (8L);
	    g_code (op_mov, IL2, mk_offset (ap1, 6L), mk_immed (0L));
	    g_code (op_mov, IL2, mk_offset (ap1, 4L), mk_immed (0L));
	    g_code (op_mov, IL2, mk_high (ap1), mk_high (ap));
	    g_code (op_mov, IL2, mk_low (ap1), mk_low (ap));
	    g_fcode (op_fild, IL4, ap1, NIL_ADDRESS);
	    freeop (ap1);
	    freeop (ap);
	    ap = float_register ();
	    return mk_legal (ap, flags, tp2);
	default:
	    break;
	}
	break;
#endif /* FLOAT_IEEE */
    case bt_uchar:
    case bt_schar:
    case bt_char:
    case bt_charu:
	flags = (FLAGS) (flags | F_NOEDI);
	switch (tp1->type) {
	case bt_uchar:
	case bt_schar:
	case bt_char:
	case bt_charu:
	case bt_ushort:
	case bt_short:
	case bt_int16:
	case bt_uint16:
	case bt_pointer16:
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_long:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    if (ap->mode == am_mreg) {
		freeop (ap);
		ap1 = data_register ();
		g_code (op_mov, IL1, mk_low (ap), ap1);
		ap = ap1;
	    }
	    return mk_legal (ap, flags, tp2);
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    freeop (ap);
	    ap = data_register ();
	    ap1 = mk_scratch (2L);
	    g_fcode (op_fistp, IL2, ap1, NIL_ADDRESS);
	    g_fcode (op_fwait, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_mov, IL2, ap1, ap);
	    freeop (ap1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
    case bt_short:
    case bt_int16:
	switch (tp1->type) {
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	case bt_pointer16:
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_long:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    ap1 = mk_legal (ap, flags, tp2);
	    freeop (ap1);
	    if ((ap1->mode == am_mreg) && (ap1->preg == EAX)) {
		ap = ax_register ();
	    } else {
		ap = data_register ();
		g_code (op_mov, IL2, mk_low (ap1), ap);
	    }
	    return mk_legal (ap, flags, tp2);
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    freeop (ap);
	    ap = data_register ();
	    ap1 = mk_scratch (2L);
	    g_fcode (op_fistp, IL2, ap1, NIL_ADDRESS);
	    g_fcode (op_fwait, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_mov, IL2, ap1, ap);
	    freeop (ap1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
    case bt_ushort:
    case bt_uint16:
    case bt_pointer16:
	switch (tp1->type) {
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	case bt_pointer16:
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_long:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    ap1 = mk_legal (ap, flags, tp2);
	    freeop (ap1);
	    if ((ap1->mode == am_mreg) && (ap1->preg == EAX)) {
		ap = ax_register ();
	    } else {
		ap = data_register ();
	        g_code (op_mov, IL2, mk_low (ap1), ap);
	    }
	    return mk_legal (ap, flags, tp2);
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    freeop (ap);
	    ap = data_register ();
	    ap1 = mk_scratch (2L);
	    g_fcode (op_fistp, IL2, ap1, NIL_ADDRESS);
	    g_fcode (op_fwait, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_mov, IL2, ap1, ap);
	    freeop (ap1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
    case bt_int32:
    case bt_long:
	switch (tp1->type) {
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	case bt_pointer16:
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_long:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    return mk_legal (ap, flags, tp2);
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    freeop (ap);
	    ap = mdata_register ();
	    ap1 = mk_scratch (4L);
	    g_fcode (op_fistp, IL4, ap1, NIL_ADDRESS);
	    g_fcode (op_fwait, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_mov, IL2, mk_low (ap1), mk_low (ap));
	    g_code (op_mov, IL2, mk_high (ap1), mk_high (ap));
	    freeop (ap1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	switch (tp1->type) {
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	case bt_pointer16:
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_long:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    return mk_legal (ap, flags, tp2);
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    freeop (ap);
	    ap = mdata_register ();
	    ap1 = mk_scratch (8L);
	    g_fcode (op_fistp, IL8, ap1, NIL_ADDRESS);
	    g_fcode (op_fwait, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_mov, IL2, mk_low (ap1), mk_low (ap));
	    g_code (op_mov, IL2, mk_high (ap1), mk_high (ap));
	    freeop (ap1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
    default:
	break;
    }
    FATAL ((__FILE__, "g_cast", "type1=%d, type2=%d", tp1->type, tp2->type));
    return NIL_ADDRESS;
}

/*
 * generate code to do a comparison of the two operands of node.
 */
static void g_compare P4 (const EXPR *, ep, OPCODE, opu, OPCODE, ops, LABEL, label1)
{
    EXPR   *ep0 = ep->v.p[0];
    EXPR   *ep1 = ep->v.p[1];
    ADDRESS *ap1, *ap2;
    FLAGS   flagx;
    LABEL   label2;
    OPCODE  op1, op2, op3, op4;
    TYP    *tp = ep0->etp;

    if (is_unsigned_type (tp)) {
	op1 = opu;
	switch (op1) {
	case op_je:
	    op2 = op_jne;
	    op3 = op_je;
	    op4 = op_jne;
	    break;
	case op_jne:
	    op2 = op_jne;
	    op3 = op_jne;
	    op4 = op_jne;
	    break;
	case op_jb:
	    op2 = op_jne;
	    op3 = op_jb;
	    op4 = op_jb;
	    break;
	case op_jbe:
	    op2 = op_ja;
	    op3 = op_jbe;
	    op4 = op_jb;
	    break;
	case op_ja:
	    op2 = op_jne;
	    op3 = op_ja;
	    op4 = op_ja;
	    break;
	case op_jae:
	    op2 = op_jb;
	    op3 = op_jae;
	    op4 = op_ja;
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
    } else {
	op1 = ops;
	switch (op1) {
	case op_je:
	    op2 = op_jne;
	    op3 = op_je;
	    op4 = op_jne;
	    break;
	case op_jne:
	    op2 = op_jne;
	    op3 = op_jne;
	    op4 = op_jne;
	    break;
	case op_jl:
	    op2 = op_jne;
	    op3 = op_jb;
	    op4 = op_jl;
	    break;
	case op_jle:
	    op2 = op_jg;
	    op3 = op_jbe;
	    op4 = op_jl;
	    break;
	case op_jg:
	    op2 = op_jne;
	    op3 = op_ja;
	    op4 = op_jg;
	    break;
	case op_jge:
	    op2 = op_jl;
	    op3 = op_jae;
	    op4 = op_jg;
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
    }
    switch (tp->type) {
    case bt_uchar:
    case bt_schar:
    case bt_char:
    case bt_charu:
    case bt_ushort:
    case bt_short:
    case bt_int16:
    case bt_uint16:
    case bt_pointer16:
	ap2 = g_expr (ep0, F_ALL);
	flagx = (ap2->mode == am_immed) ? (FLAGS) (F_MEM | F_DREG) : F_DREG;
	ap1 = g_expr (ep1, flagx);
	validate (ap2);
	sync_stack ();
	g_code (op_cmp, (ILEN) tp->size, ap2, ap1);
	g_cbranch (op1, label1);
	freeop (ap1);
	freeop (ap2);
	return;
    case bt_int32:
    case bt_long:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	label2 = nextlabel++;
	ap2 = g_expr (ep0, F_ALL);
	flagx = (ap2->mode == am_immed) ? (FLAGS) (F_MEM | F_DREG) : F_DREG;
	ap1 = g_expr (ep1, flagx);
	validate (ap2);
	sync_stack ();
	g_code (op_cmp, IL2, mk_high (ap2), mk_high (ap1));
	if (op1 == op_je) {
	    g_cbranch (op4, label2);
	} else {
	    g_cbranch (op4, label1);
	    if (op4 != op2) {
		g_cbranch (op2, label2);
	    }
	}
	g_code (op_cmp, IL2, mk_low (ap2), mk_low (ap1));
	g_cbranch (op3, label1);
	g_label (label2);
	freeop (ap1);
	freeop (ap2);
	return;
#ifdef FLOAT_IEEE
    case bt_longdouble:
    case bt_double:
    case bt_float:
	ap1 = g_expr (ep0, F_MEM);
	ap2 = g_expr (ep1, F_FREG);
	validate (ap1);
	g_fcode (op_fcomp, (ILEN) tp->size, ap1, NIL_ADDRESS);
	if (is_register_used (EAX)) {
	    g_code (op_push, IL2, &eax_reg, NIL_ADDRESS);
	    g_fcode (op_fnstsw, IL0, &eax_reg, NIL_ADDRESS);
	    g_code (op_sahf, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_pop, IL2, &eax_reg, NIL_ADDRESS);
	} else {
	    g_fcode (op_fnstsw, IL0, &eax_reg, NIL_ADDRESS);
	    g_code (op_sahf, IL0, NIL_ADDRESS, NIL_ADDRESS);
	}
	freeop (ap2);
	freeop (ap1);
	g_cbranch (op1, label1);
	return;
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_compare", "illegal type %d", tp->type));
	break;
    }
    return;
}

static void g_test P1 (const EXPR *, ep)
{
    ADDRESS *ap;

    switch (ep->etp->type) {
    case bt_uchar:
    case bt_char:
    case bt_schar:
    case bt_charu:
    case bt_ushort:
    case bt_short:
    case bt_int16:
    case bt_uint16:
    case bt_pointer16:
	ap = g_expr (ep, F_DREG);
	sync_stack ();
	g_code (op_test, (ILEN) ep->etp->size, ap, ap);
	freeop (ap);
	break;
    case bt_int32:
    case bt_long:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	ap = g_expr (ep, (FLAGS) (F_VOL | F_DREG));
	sync_stack ();
	g_code (op_or, IL2, mk_low (ap), mk_high (ap));
	freeop (ap);
	break;
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap = g_expr (ep, F_FREG);
	g_fcode (op_ftst, IL0, NIL_ADDRESS, NIL_ADDRESS);
	g_fcode (op_fstp, IL10, mk_reg (ST0), NIL_ADDRESS);
	if (is_register_used (EAX)) {
	    g_code (op_push, IL2, &eax_reg, NIL_ADDRESS);
	    g_fcode (op_fnstsw, IL0, &eax_reg, NIL_ADDRESS);
	    g_code (op_sahf, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_pop, IL2, &eax_reg, NIL_ADDRESS);
	} else {
	    g_fcode (op_fnstsw, IL0, &eax_reg, NIL_ADDRESS);
	    g_code (op_sahf, IL0, NIL_ADDRESS, NIL_ADDRESS);
	}
	freeop (ap);
	break;
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_test", "type = %d", ep->etp->type));
    }
}

#ifdef ASM
static ADDRESS *g_asm P1 (const EXPR *, ep)
{
    ADDRESS *ap = mk_expr (am_str, copynode (ep));

    g_code (op_asm, IL0, ap, NIL_ADDRESS);
    return NIL_ADDRESS;
}
#endif /* ASM */

/*
 * general expression evaluation. returns the addressing mode of the result.
 */
static ADDRESS *g_expr P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;
    LABEL   lab0, lab1;

    if (ep == NIL_EXPR) {
	FATAL ((__FILE__, "g_expr", "ep == 0"));
    }
    if (tst_const (ep)) {
	ap1 = mk_expr (am_immed, copynode (ep));
	return mk_legal (ap1, flags, ep->etp);
    }
    switch (ep->nodetype) {
    case en_autocon:
	ap1 = data_register ();
	ap2 = mk_indirect (regframe, copynode (ep));
	g_code (op_lea, IL2, ap2, ap1);
	return mk_legal (ap1, flags, ep->etp);
    case en_register:
	ap1 = mk_reg (ep->v.r);
	return mk_legal (ap1, flags, ep->etp);
    case en_ref:
	ap1 = g_deref (ep->v.p[0], ep->etp);
	if (is_structure_type (ep->etp) ||
	    is_array_type (ep->etp) ||
	    is_array_assignment (ep->etp)) {
	    return mk_legal (ap1, flags, tp_pointer);
	} else {
	    return mk_legal (ap1, flags, ep->etp);
	}
    case en_fieldref:
	return g_fderef (ep, flags);
    case en_uminus:
	return g_unary (ep, flags, op_neg);
    case en_compl:
	return g_unary (ep, flags, op_not);
    case en_add:
	return g_bin (ep, flags, op_add);
    case en_sub:
	return g_bin (ep, flags, op_sub);
    case en_and:
	return g_bin (ep, flags, op_and);
    case en_or:
	return g_bin (ep, flags, op_or);
    case en_xor:
	return g_bin (ep, flags, op_xor);
    case en_lsh:
	return g_shift (ep, flags, op_shl);
    case en_rsh:
	return g_shift (ep, flags, op_shr);
    case en_assign:
	return g_assign (ep, flags);
    case en_asadd:
	return g_asbin (ep, flags, op_add);
    case en_assub:
	return g_asbin (ep, flags, op_sub);
    case en_asand:
	return g_asbin (ep, flags, op_and);
    case en_asor:
	return g_asbin (ep, flags, op_or);
    case en_asxor:
	return g_asbin (ep, flags, op_xor);
    case en_asmul:
	return g_asmul (ep, flags);
    case en_asdiv:
	return g_asdiv (ep, flags, FALSE);
    case en_asmod:
	return g_asdiv (ep, flags, TRUE);
    case en_aslsh:
	return g_asshift (ep, flags, op_shl);
    case en_asrsh:
	return g_asshift (ep, flags, op_shr);
    case en_ainc:
	return g_aincdec (ep, flags, op_add);
    case en_adec:
	return g_aincdec (ep, flags, op_sub);
    case en_mul:
	return g_mul (ep, flags);
    case en_div:
	return g_div (ep, flags, FALSE);
    case en_mod:
	return g_div (ep, flags, TRUE);
    case en_cond:
	return g_hook (ep, flags);
    case en_eq:
    case en_ne:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
    case en_not:
    case en_test:
    case en_land:
    case en_lor:
	lab0 = nextlabel++;
	lab1 = nextlabel++;
	g_truejp (ep, lab0);
	ap1 = data_register ();
	g_code (op_mov, IL2, mk_immed (0L), ap1);
	g_branch (lab1);
	g_label (lab0);
	g_code (op_mov, IL2, mk_immed (1L), ap1);
	g_label (lab1);
	return mk_legal (ap1, flags, ep->etp);
    case en_comma:
	freeop (g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_NOVALUE)));
	return g_expr (ep->v.p[1], flags);
    case en_fcall:
    case en_call:
    case en_usercall:
	return g_fcall (ep, flags);
    case en_cast:
	return g_cast (g_expr (ep->v.p[0], F_ALL),
		       ep->v.p[0]->etp, ep->etp, flags);
    case en_deref:
	/*
	 * The cases where this node occurs are handled automatically:
	 * g_assign and g_fcall return a pointer to a structure rather than a
	 * structure.
	 */
	return g_expr (ep->v.p[0], flags);
#ifdef ASM
    case en_str:
	return g_asm (ep);
#endif /* ASM */
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
    case en_fcon:
	if ((FLAGS) (flags & (F_MEM | F_FREG)) == F_NONE) {
	    FATAL ((__FILE__, "g_expr", "EN_FCON"));
	}
	ap1 = mk_label (mk_flabel (&ep->v.f, ep->etp));
	return mk_legal (ap1, flags, ep->etp);
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */
    default:
	FATAL ((__FILE__, "g_expr", "uncoded nodetype %d", ep->nodetype));
    }
    return NIL_ADDRESS;
}


/*
 * generate a jump to label if the node passed evaluates to a true condition.
 */
static void g_truejp P2 (const EXPR *, ep, LABEL, label)
{
    LABEL   lab0;

    if (ep == NIL_EXPR) {
	FATAL ((__FILE__, "g_truejp", "ep == 0"));
    }
    if (is_icon (ep)) {
	if (ep->v.i) {
	    g_branch (label);
	}
	return;
    }
    switch (ep->nodetype) {
    case en_eq:
	g_compare (ep, op_je, op_je, label);
	break;
    case en_ne:
	g_compare (ep, op_jne, op_jne, label);
	break;
    case en_lt:
	g_compare (ep, op_ja, op_jg, label);
	break;
    case en_le:
	g_compare (ep, op_jae, op_jge, label);
	break;
    case en_gt:
	g_compare (ep, op_jb, op_jl, label);
	break;
    case en_ge:
	g_compare (ep, op_jbe, op_jle, label);
	break;
    case en_land:
	lab0 = nextlabel++;
	g_falsejp (ep->v.p[0], lab0);
	g_truejp (ep->v.p[1], label);
	g_label (lab0);
	break;
    case en_lor:
	g_truejp (ep->v.p[0], label);
	g_truejp (ep->v.p[1], label);
	break;
    case en_test:
	g_truejp (ep->v.p[0], label);
	break;
    case en_not:
	g_falsejp (ep->v.p[0], label);
	break;
    default:
	g_test (ep);
	g_cbranch (op_jne, label);
	break;
    }
}

/*
 * generate code to execute a jump to label if the expression passed is
 * false.
 */
static void g_falsejp P2 (const EXPR *, ep, LABEL, label)
{
    LABEL   lab0;

    if (ep == NIL_EXPR) {
	FATAL ((__FILE__, "g_falsejp", "ep == 0"));
    }
    if (is_icon (ep)) {
	if (!ep->v.i) {
	    g_branch (label);
	}
	return;
    }
    switch (ep->nodetype) {
    case en_eq:
	g_compare (ep, op_jne, op_jne, label);
	break;
    case en_ne:
	g_compare (ep, op_je, op_je, label);
	break;
    case en_lt:
	g_compare (ep, op_jbe, op_jle, label);
	break;
    case en_le:
	g_compare (ep, op_jb, op_jl, label);
	break;
    case en_gt:
	g_compare (ep, op_jae, op_jge, label);
	break;
    case en_ge:
	g_compare (ep, op_ja, op_jg, label);
	break;
    case en_land:
	g_falsejp (ep->v.p[0], label);
	g_falsejp (ep->v.p[1], label);
	break;
    case en_lor:
	lab0 = nextlabel++;
	g_truejp (ep->v.p[0], lab0);
	g_falsejp (ep->v.p[1], label);
	g_label (lab0);
	break;
    case en_test:
	g_falsejp (ep->v.p[0], label);
	break;
    case en_not:
	g_truejp (ep->v.p[0], label);
	break;
    default:
	g_test (ep);
	g_cbranch (op_je, label);
	break;
    }
}

/*---------------------------------------------------------------------------*/
/*
 * evaluate an expression node
 */
PRIVATE void g_expression P1 (const EXPR *, ep)
{
    initstack ();
    if (ep != NIL_EXPR) {
	VOIDCAST g_expr (ep, (FLAGS) (F_ALL | F_NOVALUE));
    }
    checkstack ();
}

/*
 * generate the code for the expression node and the code to jump to the label
 * if it is a non-zero result.
 */
PRIVATE void g_jtrue P2 (const EXPR *, ep, LABEL, label)
{
    initstack ();
    g_truejp (ep, label);
    checkstack ();
}

/*
 * generate the code for the expression node and the code to jump to the label
 * if it is a zero result.
 */
PRIVATE void g_jfalse P2 (const EXPR *, ep, LABEL, label)
{
    initstack ();
    g_falsejp (ep, label);
    checkstack ();
}

/*
 * generate the code for a switch table
 */
PRIVATE void g_switch_table P4 (const EXPR *, ep, const SWITCH *, sw, UVAL,
				min_caselabel, UVAL, max_caselabel)
{
    ADDRESS *ap, *ap1;

    initstack ();
    ap = g_expr (ep, (FLAGS) (F_AREG | F_VOL));
#if 0
    ap = g_extend (ap, ep->etp, tp_short);	/* Needs changing!!! */
#endif
    /*
     * move the interval
     */
    max_caselabel -= min_caselabel;
    if (min_caselabel != 0) {
	g_code (op_sub, IL2, mk_immed ((IVAL) min_caselabel), ap);
    }
    /*
     *       The jump table had better be small enough to fit into 2 byte
     *       offset ... so only use the bottom 16 bits.
     */
    g_code (op_cmp, IL2, mk_immed ((IVAL) max_caselabel), ap);
    g_cbranch (op_ja, sw->deflab);
    g_code (op_shl, IL2, mk_immed (1l), ap);
    freeop (ap);
    ap1 = mk_indirect (ap->preg, mk_lcon (sw->tablab));
    sync_stack ();
    /*
     * DO NOT USE OP_BRA here....
     * op_bra is reserved for jumps to internal labels.
     * This keeps things easy in the peephole optimizer
     * While producing assembler output, op_bra and op_jmp yield
     * the same
     */
    g_code (op_jmp, IL2, ap1, NIL_ADDRESS);
    checkstack ();
}

/*
 * Generate the body of a switch statement by comparing each case value
 * in turn.   The comparison is infact done by using subtraction as this
 * actually generates more efficient code (and would work best if the
 * labels were sorted!)
 */
PRIVATE void g_switch_compare P2 (const EXPR *, ep, STMT *, stmt)
{
    ADDRESS *ap;
    UVAL    min_value;

    initstack ();
    ap = g_expr (ep, (FLAGS) (F_DREG | F_VOL));
    sync_stack ();
    for (min_value = 0; stmt != NIL_STMT; stmt = stmt->s1) {
	if (stmt->stype != st_default) {
	    ADDRESS *ap2 = mk_immed ((IVAL) ((UVAL) stmt->v2.i - min_value));

	    switch (ep->etp->size) {
	    case 1L:
	    case 2L:
		g_code (op_sub, (ILEN) ep->etp->size, ap2, ap);
		break;
	    case 4L:
		g_code (op_sub, IL2, mk_low (ap2), mk_low (ap));
		g_code (op_sbb, IL2, mk_high (ap2), mk_high (ap));
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	    min_value = (UVAL) stmt->v2.i;
	    stmt->v2.l = nextlabel++;
	    g_cbranch (op_je, stmt->v2.l);
	}
    }
    freeop (ap);
    checkstack ();
}

/*
 * Generate the code at the start of a procedure which sets up the
 * stack frame
 */
PRIVATE void g_entry P1 (SIZE, frame_size)
{
#ifdef STACK_CHECK
    if (stackcheck_option) {
	SYM    *sp;
	ADDRESS *ap;

	sp = internal_symbol (SUP_STACKCHECK, NIL_TYP);
	ap = mk_immed (frame_size + max_stack_adjust);
	if (target_option < target_80186) {
	    ADDRESS *ap1 = data_register ();

	    g_code (op_mov, IL2, ap, ap1);
	    g_code (op_push, IL2, ap1, NIL_ADDRESS);
	    freeop (ap1);
	} else {
	    g_code (op_push, IL2, ap, NIL_ADDRESS);
	}
	g_code (op_call, IL0, mk_strlab (nameof (sp)), NIL_ADDRESS);
    }
#endif /* STACK_CHECK */
    if (target_option >= target_80186) {
	g_code (op_enter, IL2, mk_immed (frame_size), mk_immed (0));
    } else {
	g_code (op_push, IL2, mk_reg (regframe), NIL_ADDRESS);
	g_code (op_mov, IL2, mk_reg (STACKPTR), mk_reg (regframe));
	if (frame_size != 0L) {
	    g_code (op_sub, IL2, mk_immed (frame_size), mk_reg (STACKPTR));
	}
    }
    max_stack_adjust = 0L;
}

/*
 * Generate the code for evaluating the expression returned by a routine
 */
PRIVATE void g_return P2 (const EXPR *, stmtexp, TYP *, tp)
{
    EXPR   *ep, *ep1;
    ADDRESS *ap;

    initstack ();
    switch (tp->type) {
    case bt_struct:
    case bt_union:
	/* assign structure */
	ep = mk_autocon ((SIZE) 4);
	ep = mk_ref (ep, tp_pointer);
	ep1 = mk_ref (ep, tp);
	ep1 = mk_node (en_assign, ep1, copynode (stmtexp), tp);
	VOIDCAST g_expr (ep1, (FLAGS) (F_ALL | F_NOVALUE));

	ap = g_expr (ep, F_ALL);
	g_code (op_mov, IL2, ap, mk_reg (reg_usage->result->reg[0]));
	freeop (ap);
	break;
#ifdef FLOAT_SUPPORT
    case bt_float:
    case bt_longdouble:
    case bt_double:
	/*
	 *   return floating point value on top of fpu stack
	 */
	ap = g_expr (stmtexp, F_FREG);
	freeop (ap);
	break;
#endif /* FLOAT_SUPPORT */

    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_pointer16:
	ap = g_expr (stmtexp, F_ALL);
	g_code (op_mov, (ILEN) stmtexp->etp->size, ap,
		mk_reg (reg_usage->result->reg[0]));
	freeop (ap);
	break;
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap = g_expr (stmtexp, F_ALL);
	g_code (op_mov, IL2, mk_low (ap), mk_reg (reg_usage->result->reg[0]));
	g_code (op_mov, IL2, mk_high (ap),
		mk_reg (reg_usage->result->reg[1]));
	freeop (ap);
	break;
    default:
	FATAL ((__FILE__, "g_return", "illegal type %d", tp->type));
    }
    checkstack ();
}

/*
 * Collapse the stack frame at the end of a function
 */
PRIVATE void g_epilogue P0 (void)
{
    ADDRESS *ap;
    SIZE    stackoffset;
    REG     reg;

    /*
     * Adjust stack pointer to initial value
     */
    if (regs_used > 0 && !is_leaf_function) {
	if (lc_auto > lc_auto_max) {
	    lc_auto_max = lc_auto;
	}
	stackoffset = lc_auto_max + max_scratch + (2L * (SIZE) regs_used);
	ap = mk_indirect (EBP, mk_const (-stackoffset));
	g_code (op_lea, IL2, ap, mk_reg (ESP));
    }
    /*
     * pop clobbered register variables
     */
    for (reg = EAX; reg <= ESI; reg++) {
	if (restore_mask & (REGBIT (reg))) {
	    g_code (op_pop, IL2, mk_reg (reg), NIL_ADDRESS);
	}
    }
    if (target_option >= target_80186) {
	g_code (op_leave, IL0, NIL_ADDRESS, NIL_ADDRESS);
    } else {
	g_code (op_mov, IL2, mk_reg (EBP), mk_reg (ESP));
	g_code (op_pop, IL2, mk_reg (EBP), NIL_ADDRESS);
    }
    g_code (op_ret, IL0, NIL_ADDRESS, NIL_ADDRESS);
}

/*
 * allocate will allocate registers for the expressions that have a high
 * enough desirability.
 */
PRIVATE void g_allocate P1 (CSE *, olist)
{
    REGMASK mask = (REGMASK) 0;
    REG     reg;

    (void) olist;

    /*
     *       Now take into account which registers must be saved by the
     *       function.
     */
    mask &= reglist_to_mask (reg_usage->save);

    for (reg = ESI; reg >= EAX; reg--) {
	if (mask & (REGBIT (reg))) {
	    g_code (op_push, IL2, mk_reg (reg), NIL_ADDRESS);
	}
    }
    restore_mask = mask;
}

/*
 *   Go through the common sub-expression tree and check to see if
 *   any registers must be loaded with a value.
 */

PRIVATE void g_preload P1 (CSE *, olist)
{
    CSE    *csp;
    EXPR   *ep;
    ADDRESS *ap, *ap2;

    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	if (csp->reg != NO_REG) {	/* see if preload needed */
	    ep = csp->exp;
	    if ((!is_lvalue (ep)) || (ep->v.p[0]->v.i > 0L)) {
		initstack ();
		ap = g_expr (ep, F_ALL);
		ap2 = mk_reg (csp->reg);
		g_code (op_mov, (ILEN) ep->etp->size, ap, ap2);
		freeop (ap);
	    }
	}
    }
}

/*
 *   Write out the completed code list
 */
PRIVATE void g_flush P1 (SYM *, sp)
{
    put_literals ();
    if (sp) {
	put_cseg (alignment_of_type (typeof (sp)));
	put_name (sp);
    }
    flush_peep (peep_option);
    put_epilogue (sp, nextlabel++);
}


/*
 *   Ensure that the stack is rounded to a suitable value.
 */
PRIVATE void g_auto_align P0 (void)
{
    if (lc_auto_max % AL_DEFAULT != 0L) {
	lc_auto_max += AL_DEFAULT - (lc_auto_max % AL_DEFAULT);
    }
}

/*
 *   return TRUE if this is a big-endian processor ...
 *   otherwise return false.
 */
PRIVATE BOOL g_is_bigendian P0 (void)
{
    return FALSE;
}

PRIVATE BOOL g_is_ascending_stack P0 (void)
{
    return FALSE;
}

/*
 *   The routine g_transform() is called to do any code generator
 *   specific transformations on the expression tree before being
 *   passed to the global optimizer.
 *
 *   The expression tree is examined and expression nodes which
 *   are performed by compiler support routines are replaced by
 *   the equivalent function call.   Previously these calls were
 *   performed in the code generation routines themselves.  However
 *   this meant that these routines were unnecessarily complicated
 *   and also that the resulting expressions didn't benefit from
 *   optimisations that could be detected by the global optimiser.
 */
PRIVATE EXPR *g_transform P1 (EXPR *, ep)
{
    if (ep == NIL_EXPR) {
	return ep;
    }
    switch (ep->nodetype) {
    case en_icon:
    case en_fcon:
    case en_nacon:
    case en_labcon:
    case en_autocon:
    case en_sym:
    case en_register:
	break;
    case en_eq:
    case en_ne:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
    case en_add:
    case en_sub:
    case en_and:
    case en_or:
    case en_xor:
    case en_land:
    case en_lor:
    case en_cond:
    case en_comma:
    case en_list:
    case en_asadd:
    case en_assub:
    case en_asor:
    case en_asxor:
    case en_asand:
    case en_fcall:
    case en_call:
    case en_usercall:
    case en_assign:
    case en_ainc:
    case en_adec:
    case en_uminus:
    case en_test:
    case en_not:
    case en_compl:
    case en_deref:
    case en_cast:
    case en_ref:
    case en_fieldref:
	return ep;
    case en_aslsh:
    case en_asrsh:
    case en_asmul:
    case en_asdiv:
    case en_asmod:
	switch (ep->etp->type) {
	case bt_int16:
	case bt_short:
	    if (ep->etp->size == 2)
		break;
	    /*lint -fallthrough */
	case bt_int32:
	case bt_long:
	    switch (ep->nodetype) {
	    case en_aslsh:
		ep = transform_assign (ep, SUP_ASLSHL, SUP_LSHL, SUP_ASOPL);
		return ep;
	    case en_asrsh:
		ep = transform_assign (ep, SUP_ASLSHR, SUP_LSHR, SUP_ASOPL);
		return ep;
	    case en_asmul:
		ep = transform_assign (ep, SUP_ASLMUL, SUP_LMUL, SUP_ASOPL);
		return ep;
	    case en_asmod:
		ep = transform_assign (ep, SUP_ASLMOD, SUP_LMOD, SUP_ASOPL);
		return ep;
	    case en_asdiv:
		ep = transform_assign (ep, SUP_ASLDIV, SUP_LDIV, SUP_ASOPL);
		return ep;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	    break;
	case bt_uint16:
	case bt_ushort:
	case bt_pointer16:
	    if (ep->etp->size == 2)
		break;
	    /*lint -fallthrough */
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    switch (ep->nodetype) {
	    case en_aslsh:
		ep = transform_assign (ep, SUP_ASULSHL, SUP_ULSHL, SUP_ASOPL);
		return ep;
	    case en_asrsh:
		ep = transform_assign (ep, SUP_ASULSHR, SUP_ULSHR, SUP_ASOPL);
		return ep;
	    case en_asmul:
		ep = transform_assign (ep, SUP_ASULMUL, SUP_ULMUL, SUP_ASOPL);
		return ep;
	    case en_asmod:
		ep = transform_assign (ep, SUP_ASULMOD, SUP_ULMOD, SUP_ASOPL);
		return ep;
	    case en_asdiv:
		ep = transform_assign (ep, SUP_ASULDIV, SUP_ULDIV, SUP_ASOPL);
		return ep;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	    break;
	default:
	    break;
	}
	return ep;
    case en_mul:
    case en_div:
    case en_mod:
    case en_lsh:
    case en_rsh:
	switch (ep->etp->type) {
	case bt_int32:
	case bt_long:
	    switch (ep->nodetype) {
	    case en_lsh:
		ep = transform_binary (ep, SUP_LSHL);
		break;
	    case en_rsh:
		ep = transform_binary (ep, SUP_LSHR);
		break;
	    case en_mul:
		ep = transform_binary (ep, SUP_LMUL);
		break;
	    case en_mod:
		ep = transform_binary (ep, SUP_LMOD);
		break;
	    case en_div:
		ep = transform_binary (ep, SUP_LDIV);
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	    return ep;
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    switch (ep->nodetype) {
	    case en_lsh:
		ep = transform_binary (ep, SUP_ULSHL);
		break;
	    case en_rsh:
		ep = transform_binary (ep, SUP_ULSHR);
		break;
	    case en_mul:
		ep = transform_binary (ep, SUP_ULMUL);
		break;
	    case en_mod:
		ep = transform_binary (ep, SUP_ULMOD);
		break;
	    case en_div:
		ep = transform_binary (ep, SUP_ULDIV);
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	    return ep;
	default:
	    break;
	}
	return ep;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return ep;
}

/*
 *   This routine is called after the global optimizer has done it's
 *   work re-organising the expression tree.  This allows a code
 *   generator to make code generator specific changes to the expression
 *   tree which will result in better code generation.
 */

PRIVATE EXPR *g_order P1 (EXPR *, ep)
{
    switch (ep->nodetype) {
    case en_lt:
    case en_gt:
    case en_le:
    case en_ge:
    case en_eq:
    case en_ne:
	switch (ep->v.p[0]->nodetype) {
	case en_icon:
	    swap_nodes (ep);
	    break;
	default:
	    break;
	}
	switch (ep->v.p[1]->nodetype) {
	case en_icon:
	    swap_nodes (ep);
	    break;
	default:
	    break;
	}
	break;
    default:
	break;
    }
    return ep;
}

/*
 *   This routine is called when the compiler is initialising, i.e.
 *   before it even starts scanning tokens.
 */
PRIVATE void g_initialize P0 (void)
{
    regtypes = &reg_type[0];
    if (!optimize_option) {
	stackopt_option = 0;
    }
    if (stackopt_option) {
	is_parameter = 0;
    }
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
    UNUSEDARG (tp);
    return reg_usage;
}

#ifdef MULTIPLE_PROCESSORS
#define	MC86_FUNCS	(void *)&mc86_funcs
#else
#define	MC86_FUNCS	(void *)NULL
#endif /* MULTIPLE_PROCESSORS */

static OPTENUM yesnoopts[] = {
    {(const CHAR *) "yes", 1},
    {(const CHAR *) "no", 0},
    {(const CHAR *) NULL, 0}
};

static OPTSET peepset[] = {
    {(const CHAR *) "none", PEEP_NONE},
    {(const CHAR *) "flow", PEEP_FLOW},
    {(const CHAR *) "all", PEEP_ALL},
    {(const CHAR *) NULL, 0}
};

static OPTENUM stackoptions[] = {
    {(const CHAR *) "safest", OPT_SAFE},
    {(const CHAR *) "minimum", OPT_MINIMUM},
    {(const CHAR *) "average", OPT_AVERAGE},
    {(const CHAR *) "maximum", OPT_MAXIMUM},
    {(const CHAR *) NULL, 0}
};

static OPTENUM target_opts[] = {
    {(const CHAR *) "8086", (int) target_8086},
    {(const CHAR *) "80186", (int) target_80186},
    {(const CHAR *) "80286", (int) target_80286},
    {(const CHAR *) NULL, 0}
};

static OPTSET reg86set[] = {
    {(const CHAR *) "ax", MEMBER (EAX)},
    {(const CHAR *) "bx", MEMBER (EBX)},
    {(const CHAR *) "cx", MEMBER (ECX)},
    {(const CHAR *) "dx", MEMBER (EDX)},
    {(const CHAR *) "di", MEMBER (EDI)},
    {(const CHAR *) "si", MEMBER (ESI)},
    {(const CHAR *) NULL, EMPTY_SET},
};

static OPTION opts[] = {
    {
     (const CHAR *) "target=", enumeration_option,
     {&target_option},
     {&target_opts[0]}
     },
    {
     (const CHAR *) "fpu=", enumeration_option,
     {&fpu_option},
     {&yesnoopts[0]}
     },
    {
     (const CHAR *) "peep=", set_option,
     {&peep_option},
     {(struct optenum*)&peepset[0]}
     },
    {
     (const CHAR *) "prefix=", string_option,
     {(void *) &external_prefix},
     {NULL}
     },
    {
     (const CHAR *) "regsave=", set_option,
     {&regsave_option},
     {(struct optenum*)&reg86set[0]}
     },
#ifdef STACK_CHECK
    {
     (const CHAR *) "stackcheck=", enumeration_option,
     {&stackcheck_option},
     {&yesnoopts[0]}
     },
#endif /* STACK_CHECK */
    {
     (const CHAR *) "stackopt=", enumeration_option,
     {&stackopt_option},
     {&stackoptions[0]}
     },
#ifdef TRANSLATE
    {
     (const CHAR *) "trans=", enumeration_option,
     {&trans_option},
     {&yesnoopts[0]}
     },
#endif /* TRANSLATE */
#ifdef MULTIPLE_ASSEMBLERS
#ifdef TARGET_NASM
    {
     (const CHAR *) "nasm86", chip_option,
     {&nasmx86_func},
     {MC86_FUNCS}
     },
#endif /* TARGET_MASM */
#ifdef TARGET_MASM
    {
     (const CHAR *) "masm86", chip_option,
     {&masmx86_func},
     {MC86_FUNCS}
     },
#endif /* TARGET_MASM */
#ifdef TARGET_BAS
    {
     (const CHAR *) "bas86", chip_option,
     {&basx86_func},
     {MC86_FUNCS}
     },
#endif /* TARGET_BAS */
#ifdef TARGET_GAS
    {
     (const CHAR *) "gas86", chip_option,
     {&gasx86_func},
     {MC86_FUNCS}
     },
#endif /* TARGET_GAS */
#ifdef TARGET_SYSV
    {
     (const CHAR *) "sysv86", chip_option,
     {&sysvx86_func},
     {MC86_FUNCS}
     },
#endif /* TARGET_SYSV */
#else
#ifdef TARGET_MASM
    {
     (const CHAR *) "masm86", chip_option,
     {NULL},
     {MC86_FUNCS}
     },
#endif /* TARGET_MASM */
#ifdef TARGET_BAS
    {
     (const CHAR *) "bas86", chip_option,
     {NULL},
     {MC86_FUNCS}
     },
#endif /* TARGET_BAS */
#ifdef TARGET_GAS
    {
     (const CHAR *) "gas86", chip_option,
     {NULL},
     {MC86_FUNCS}
     },
#endif /* TARGET_GAS */
#ifdef TARGET_SYSV
    {
     (const CHAR *) "sysv86", chip_option,
     {NULL},
     {MC86_FUNCS}
     },
#endif /* TARGET_SYSV */
#endif /* MULTIPLE_ASSEMBLERS */
    {
     (const CHAR *) NULL, NULL,
     {NULL},
     {NULL}
     }
};

OPTIONS opts86 = {
    (const CHAR *) "Intel 8086 ",
    opts
};

#ifdef MULTIPLE_PROCESSORS
struct genfuncs mc86_funcs = {
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
    &alignments_8086[0]
};

#undef GEN_MODULE
#endif /* MULTIPLE_PROCESSORS */
#endif /* INTEL_86 */
