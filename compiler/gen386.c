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

#ifdef INTEL_386
/******************************************************************************
 *
 *  This module contains all the code generation routines for
 *  evaluating expressions and conditions for the INTEL 80386
 *  processor.
 *
 *****************************************************************************/

#define GEN_MODULE

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "gen386.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define	AL_DEFAULT	(g_alignments[bt_ellipsis])

/********************************************************** Static Variables */

/*
 *   Command line options
 */
static unsigned peep_option = PEEP_ALL;	/* peephole optimisations */
static int stackopt_option = OPT_MINIMUM;	/* Use lazy stack optimisation */
static int regs_used = 0;	/* number of register variable allocated */

static REGMASK restore_mask;	/* register restore mask */
static SIZE max_stack_adjust = 0L;	/* largest amount stack is altered */
static REG regframe = FRAMEPTR;


#ifdef FLOAT_IEEE
static ADDRESS ax_reg = {
    am_dreg, AX, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};

#endif /* FLOAT_IEEE */

static ADDRESS ecx_reg = {
    am_dreg, ECX, (REG) 0, (DEEP) 0,
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
static ADDRESS esp_reg = {
    am_dreg, ESP, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};
static ADDRESS eax_reg = {
    am_dreg, EAX, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};
static ADDRESS edx_reg = {
    am_dreg, EDX, (REG) 0, (DEEP) 0,
    {NIL_EXPR}
};

static REGTYPE reg_type[] = {
    (REGTYPE) (D_REG | A_REG | X_REG | T_REG),	/* EAX */
    (REGTYPE) (D_REG | A_REG | X_REG | T_REG),	/* EDX */
    (REGTYPE) (D_REG | A_REG | Y_REG | T_REG | C_REG),	/* ECX */
    (REGTYPE) 0,		/* EBX */
    (REGTYPE) 0,		/* ESI */
    (REGTYPE) 0,		/* EDI */
    (REGTYPE) 0,		/* ESP */
    (REGTYPE) 0,		/* EBP */
    (REGTYPE) 0,		/* AX */
    (REGTYPE) 0,		/* DX */
    (REGTYPE) 0,		/* CX */
    (REGTYPE) 0,		/* BX */
    (REGTYPE) 0,		/* SI */
    (REGTYPE) 0,		/* DI */
    (REGTYPE) 0,		/* SP */
    (REGTYPE) 0,		/* BP */
    (REGTYPE) 0,		/* AL */
    (REGTYPE) 0,		/* DL */
    (REGTYPE) 0,		/* CL */
    (REGTYPE) 0,		/* BL */
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
 *   The following tables specify the alignment requirements of the
 *   basic types depending on the processor type.
 */
static SIZE alignments_386[] = {
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
    4L,				/* bt_int32     */
    4L,				/* bt_uint32    */
    4L,				/* bt_long      */
    4L,				/* bt_ulong     */
    4L,				/* bt_longlong  */
    4L,				/* bt_ulonglong */
    4L,				/* bt_float     */
    4L,				/* bt_double    */
    4L,				/* bt_longdouble */
    4L,				/* bt_floatcomplex */
    4L,				/* bt_doublecomplex */
    4L,				/* bt_longdoublecomplex */
    4L,				/* bt_floatimaginary */
    4L,				/* bt_doubleimaginary */
    4L,				/* bt_longdoubleimaginary */
    4L,				/* bt_pointer16 */
    4L,				/* bt_pointer32 */
    4L,				/* bt_struct    */
    4L,				/* bt_union     */
    4L,				/* bt_func      */
    4L,				/* bt_bitfield  */
    4L,				/* bt_ubitfield */
    1L,				/* bt_bbitfield */
    4L				/* bt_ellipsis - used for alignment suitable for all types */
};

#ifndef MULTIPLE_PROCESSORS
PRIVATE SIZE *g_alignments = &alignments_386[0];

#endif /* MULTIPLE_PROCESSORS */

/*********************************************** Static Function Definitions */

static ADDRESS *func_result P_ ((FLAGS, SIZE, TYP *));
static ADDRESS *g_aincdec P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asbin P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asbitfield P_ ((const EXPR *, FLAGS, OPCODE, BOOL));
static ADDRESS *g_asdiv P_ ((const EXPR *, FLAGS, BOOL));
static ADDRESS *g_asmul P_ ((const EXPR *, FLAGS));
static ADDRESS *g_asshift P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_assign P_ ((const EXPR *, FLAGS));
static ADDRESS *g_bin P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_cast P_ ((ADDRESS *, TYP *, TYP *, FLAGS));
static ADDRESS *g_deref P_ ((const EXPR *, TYP *));
static ADDRESS *g_div P_ ((const EXPR *, FLAGS, BOOL));
static ADDRESS *g_expr P_ ((const EXPR *, FLAGS));
static ADDRESS *g_fcall P_ ((const EXPR *, FLAGS));
static ADDRESS *g_fderef P_ ((const EXPR *, FLAGS));
static ADDRESS *g_hook P_ ((const EXPR *, FLAGS));
static ADDRESS *g_mul P_ ((const EXPR *, FLAGS));
static ADDRESS *g_shift P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_unary P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *mk_amode P_ ((AMODE));
static ADDRESS *mk_expr P_ ((AMODE, EXPR *));
static ADDRESS *mk_legal P_ ((ADDRESS *, FLAGS, TYP *));
static ADDRESS *mk_direct P_ ((EXPR *));
static ADDRESS *mk_indirect P_ ((REG, EXPR *));
static ADDRESS *mk_strlab P_ ((const CHAR *));
static OPCODE g_compare P_ ((const EXPR *, OPCODE, OPCODE));
static OPCODE g_test P_ ((const EXPR *, OPCODE));
static SIZE g_parms P_ ((const EXPR *));
static SIZE push_param P_ ((const EXPR *));
static void g_falsejp P_ ((const EXPR *, LABEL));
static void g_rotate P_ ((ADDRESS *, ILEN, int, TYP *, int));
static void g_truejp P_ ((const EXPR *, LABEL));
static void structassign P_ ((ADDRESS *, ADDRESS *, SIZE));

#ifdef FLOAT_IEEE
static void call_library P_ ((const CHAR *));

#endif /* FLOAT_IEEE */

/*********************************************** Global Function Definitions */

#ifdef MULTIPLE_PROCESSORS
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

#endif /* MULTIPLE_PROCESSORS */

/*****************************************************************************/

#ifdef FLOAT_IEEE
/*
 * generate a call to a library routine.
 * it is assumed that lib_name won''t be clobbered
 */
static void call_library P1 (const CHAR *, lib_name)
{
    SYM    *sp;

    sp = internal_symbol (lib_name, NIL_TYP);
    g_code (op_call, IL0, mk_strlab (nameof (sp)), NIL_ADDRESS);
}

#endif /* FLOAT_IEEE */

static ADDRESS *mk_amode P1 (AMODE, mode)
{
    ADDRESS *ap;

    ap = (ADDRESS *) xalloc (sizeof (ADDRESS));

    ap->mode = mode;
    return ap;
}

/*
 * copy an address mode structure.
 */
static ADDRESS *copy_addr P2 (ADDRESS *, ap, AMODE, mode)
{
    ADDRESS *newap;

    assert (ap != NIL_ADDRESS);

    newap = (ADDRESS *) xalloc (sizeof (ADDRESS));

    *newap = *ap;
    newap->mode = mode;
    return newap;
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
	ap->preg = reg;
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
	ap->preg = reg;
	break;
#endif /* FLOAT_IEEE */
    default:
	CANNOT_REACH_HERE ();
	ap = NIL_ADDRESS;
    }
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

static ADDRESS *mk_offset P2 (ADDRESS *, ap, SIZE, off)
{
    switch (ap->mode) {
    case am_ind:
	ap = copy_addr (ap, am_indx);
	ap->u.offset = mk_const (off);
	return ap;
    case am_indx:
    case am_indx2:
	ap = copy_addr (ap, am_indx);
	ap->u.offset = mk_const (ap->u.offset->v.i + off);
	return ap;
    case am_direct:
	ap = copy_addr (ap, ap->mode);
	ap->u.offset = mk_add (ap->u.offset, mk_const (off));
	return ap;
    default:
	return NIL_ADDRESS;
    }
}

static ADDRESS *mk_low P1 (ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_dreg:
    case am_areg:
    case am_freg:
    case am_immed:
	return ap;
    case am_mreg:
	return mk_reg (ap->preg);
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_direct:
	return mk_offset (ap, 0L);
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return NIL_ADDRESS;
}

static ADDRESS *mk_high P1 (ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_mreg:
	return mk_reg (ap->sreg);
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_direct:
	return mk_offset (ap, 4L);
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
	if (bytes == 4L && !is_register_used (ECX)) {
	    g_code (op_pop, IL4, &ecx_reg, NIL_ADDRESS);
	} else {
	    g_code (op_add, IL4, mk_immed (bytes), &esp_reg);
	}
	stack_offset -= bytes;
	if (max_stack_adjust < bytes) {
	    max_stack_adjust = bytes;
	}
    }
}

/*
 * mk_legal will coerce the addressing mode in ap1 into a mode that is
 * satisfactory for the flag word.
 */
static ADDRESS *mk_legal P3 (ADDRESS *, ap, FLAGS, flags, TYP *, tp)
{
    ADDRESS *ap2;
    ILEN    ilen = (ILEN) tp->size;

    if (flags & F_NOVALUE) {
	if (ap != NIL_ADDRESS) {
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
	    return ap;
	}
#ifdef FLOAT_IEEE
	if (flags & F_FREG) {
	    ADDRESS *ap1;

	    ap1 = mk_indirect (ESP, mk_const (-4L));
	    g_code (op_mov, IL4, ap, ap1);
	    g_code (op_fild, IL4, ap1, NIL_ADDRESS);
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
	    if ((flags & F_NOEDI) && ((ap->preg == EDI || ap->preg == ESI) ||
				      (ap->sreg == EDI || ap->sreg == EDI)))
		break;
	    if ((flags & F_VOL) && !is_temporary_register (ap->preg) &&
		!is_temporary_register (ap->sreg))
		break;
	    if ((flags & F_EAXEDX)
		&& ((ap->preg != EAX) || (ap->sreg != EDX))) {
		break;
	    }
	    return ap;
	}
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
	       (__FILE__, "mk_legal", "mode == %d, flags = 0x%x", ap->mode,
		flags));
    }
    if (flags & F_DREG) {
	freeop (ap);		/* maybe we can use it */
	if (flags & F_ECX) {
	    ap2 = cx_register ();
	    g_code (op_mov, IL2, ap, ap2);
	    return ap2;
	}
	if (flags & F_EAXEDX) {
	    ap2 = axdx_register ();
	    g_code (op_mov, IL4, ap, mk_low (ap2));
	} else {
	    ap2 = data_register ();
	    /*
	     * byte transfers from %edi/%esi to a scratch register come up here
	     */
	    if (ap->mode == am_dreg && (ap->preg == ESI || ap->preg == EDI)
		&& ilen == IL1) {
		ilen = IL2;
	    }
	    g_code (op_mov, ilen, ap, ap2);
	}
	return ap2;
    }
#ifdef FLOAT_IEEE
    if (flags & F_FREG) {
	freeop (ap);
	switch (tp->type) {
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap2 = float_register ();
	    g_fcode (op_fld, ilen, ap, NIL_ADDRESS);
	    return ap2;
	case bt_char:
	case bt_schar:
/*KDW */
	case bt_short:
	case bt_int16:
	case bt_long:
	case bt_int32:
	    ap2 = float_register ();
	    g_code (op_fild, ilen, ap, NIL_ADDRESS);
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
	case 4L:
	    switch (ap->mode) {
#ifdef FLOAT_IEEE
	    case am_freg:
		g_fcode (op_fstp, ilen, ap2, NIL_ADDRESS);
		break;
#endif /* FLOAT_IEEE */
	    default:
		g_code (op_mov, ilen, ap, ap2);
		break;
	    }
	    break;
	case 8L:
	    switch (ap->mode) {
#ifdef FLOAT_IEEE
	    case am_freg:
		g_fcode (op_fstp, ilen, ap2, NIL_ADDRESS);
		break;
#endif /* FLOAT_IEEE */
	    case am_mreg:
		g_code (op_mov, IL4, mk_high (ap), mk_high (ap2));
		g_code (op_mov, IL4, mk_low (ap), mk_low (ap2));
		break;
	    default:
		g_code (op_mov, ilen, ap, ap2);
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

/*
 * generate code to evaluate an index node and return the addressing
 * mode of the result.
 */
static ADDRESS *g_index P1 (const EXPR *, ep)
{
    ADDRESS *ap1, *ap2;

    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_IMMED | F_VOL));
    switch (ap1->mode) {
    case am_dreg:
	ap2 = g_expr (ep->v.p[1], F_ALL);
	validate (ap1);
	break;
    case am_immed:
	ap2 = ap1;
	ap1 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED | F_VOL));
	break;
    default:
	CANNOT_REACH_HERE ();
    }

    switch (ap1->mode) {
    case am_dreg:
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
	case am_dreg:
	    g_code (op_add, IL4, ap1, ap2);
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
	ap1 = mk_legal (ap1, (FLAGS) (F_DREG | F_VOL), tp_pointer);
    }
    g_code (op_add, IL4, ap2, ap1);
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

    switch (tp->type) {
    case bt_int16:
    case bt_int32:
	/* sign bitfield */
	g_code (op_asl, ilen, mk_immed (32L - (IVAL) offset - (IVAL) width),
		ap);
	g_code (op_asr, ilen, mk_immed (32L - (IVAL) width), ap);
	break;
    default:
	/* offset is in range -31 .. 31 */
	if (offset < 0) {
	    offset += ilen * 8;
	}
	/* offset in range 0..31 */
	if (offset != 0) {
	    if (offset > 15) {
		op = op_rol;
		offset = 32 - offset;
	    } else {
		op = op_ror;
	    }
	    g_code (op, ilen, mk_immed ((IVAL) offset), ap);
	}
	if (!is_void (tp)) {
	    g_code (op_and, ilen, mk_immed ((IVAL) bitmask ((BITSIZE) width)),
		    ap);
	}
    }
}


static ADDRESS *g_extend P3 (ADDRESS *, ap, TYP *, tp1, TYP *, tp2)
{
    OPCODE  op;

    ap = mk_legal (ap, (FLAGS) (F_DREG | F_VOL), tp1);
    switch (tp2->type) {
    case bt_int16:
    case bt_uint16:
    case bt_short:
    case bt_ushort:
	switch (tp1->type) {
	case bt_char:
	case bt_schar:
	    op = op_movsbw;
	    g_code (op, IL0, ap, ap);
	    break;
	case bt_uchar:
	case bt_charu:
	    op = op_movzbw;
	    g_code (op, IL0, ap, ap);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	break;
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	switch (tp1->type) {
	case bt_char:
	case bt_schar:
	    op = op_movsbl;
	    g_code (op, IL0, ap, ap);
	    break;
	case bt_int16:
	case bt_short:
	    op = op_movswl;
	    g_code (op, IL0, ap, ap);
	    break;
	case bt_uchar:
	case bt_charu:
	    op = op_movzbl;
	    g_code (op, IL0, ap, ap);
	    break;
	case bt_uint16:
	case bt_ushort:
	    op = op_movzwl;
	    g_code (op, IL0, ap, ap);
	    break;
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
    default:
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

    if (is_structure_type (tp) || is_array_assignment (tp)) {
	return g_expr (ep, F_ALL);
    }
    switch (ep->nodetype) {
    case en_add:
	return g_index (ep);
    case en_autocon:
	ap1 = mk_indirect (regframe, mk_const (ep->v.i));
	return ap1;
    default:
	ap1 = g_expr (ep, (FLAGS) (F_DREG | F_IMMED));
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

/*============================================================================*/
#ifdef FLOAT_IEEE
static void push_rtl_params P2 (const EXPR *, ep1, const EXPR *, ep2)
{
    ADDRESS *ap, *ap2;

    is_parameter++;
    temp_inv ();
    ap = g_expr (ep1, F_MEM);
    ap2 = data_register ();
    g_code (op_lea, IL4, ap, ap2);
    g_code (op_push, IL4, ap2, NIL_ADDRESS);
    freeop (ap2);
    freeop (ap);
    if (ep2) {
	ap = g_expr (ep2, F_MEM);
	ap2 = data_register ();
	g_code (op_lea, IL0, ap, ap2);
	g_code (op_push, IL4, ap2, NIL_ADDRESS);
	freeop (ap2);
	freeop (ap);
    }
}
#endif /* FLOAT_IEEE */
/*============================================================================*/

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
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	g_code (op, (ILEN) ep->etp->size, ap, NIL_ADDRESS);
	return mk_legal (ap, flags, ep->etp);
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap = g_expr (ep->v.p[0], F_FREG);
	g_code (op_fchs, IL0, NIL_ADDRESS, NIL_ADDRESS);
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
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}
	if (flags & F_NOVALUE) {
	    ap1 = NIL_ADDRESS;
	    ap2 = g_expr (ep->v.p[0], F_ALL);
	} else {
	    ap1 = data_register ();
	    ap2 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG));
	    validate (ap1);
	    g_code (op_mov, (ILEN) ep->etp->size, ap2, ap1);
	}
	g_code (op, (ILEN) ep->etp->size, mk_immed (ep->v.p[1]->v.i), ap2);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	if (flags & F_NOVALUE) {
	    return g_asbin (ep, flags, op);
	}
	ilen = (ILEN) ep->etp->size;
	ap1 = g_expr (ep->v.p[0], F_MEM);
	g_fcode (op_fld, ilen, ap1, NIL_ADDRESS);	/* save result */
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

/*============================================================================*/
/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */
static ADDRESS *g_bin P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;
    TYP    *tp = ep->etp;
    FLAGS   f = F_ALL;

    switch (tp->type) {
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
	f = (FLAGS) (f | F_NOEDI);
	/*FALLTHRU */
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	ap2 = g_expr (ep->v.p[1], f);
	validate (ap1);
	g_code (op, (ILEN) tp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
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
	/*FALLTHRU */
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
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
	    return NIL_ADDRESS;
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
	    g_code (op_mov, IL4, ap3, ap1);
	    return mk_legal (ap1, flags, ep->etp);
	}
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

/*============================================================================*/

/*
 * generate code to evaluate a shift node
 */
static ADDRESS *g_shift P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;

    switch (ep->etp->type) {
    case bt_long:
    case bt_int32:
    case bt_int16:
    case bt_char:
    case bt_schar:
    case bt_short:
	if (op == op_shr) {
	    op = op_asr;
	}
	/*FALLTHRU */
    case bt_pointer32:
    case bt_ulong:
    case bt_uint32:
    case bt_uint16:
    case bt_charu:
    case bt_uchar:
    case bt_ushort:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL | F_NOECX));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED | F_ECX));
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

    switch (ep->etp->type) {
    case bt_long:
    case bt_int32:
    case bt_int16:
    case bt_short:
    case bt_char:
    case bt_schar:
	if (op == op_shr) {
	    op = op_asr;
	}
	/*FALLTHRU */
    case bt_pointer32:
    case bt_ulong:
    case bt_uint32:
    case bt_uint16:
    case bt_uchar:
    case bt_charu:
    case bt_ushort:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG | F_NOECX));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED | F_ECX));
	validate (ap1);
	g_code (op, (ILEN) ep->etp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    default:
	FATAL ((__FILE__, "g_asshift", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*============================================================================*/

/*
 * generate code to evaluate a divide node (mod==0) or mod node (mod==1)
 */
static ADDRESS *g_div P3 (const EXPR *, ep, FLAGS, flags, BOOL, mod)
{
    ADDRESS *ap1, *ap2;
    OPCODE  op = op_idiv;

    switch (ep->etp->type) {
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	op = op_div;
	/*FALLTHRU */
    case bt_long:
    case bt_int32:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_EAXEDX));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_MEM));
	validate (ap1);
	if (op == op_idiv) {
	    g_code (op_cdq, IL0, NIL_ADDRESS, NIL_ADDRESS);
	} else {
	    g_code (op_xor, IL4, &edx_reg, &edx_reg);
	}
	g_code (op, IL4, ap2, NIL_ADDRESS);
	freeop (ap2);
	freeop (ap1);
	ap1 = data_register ();
	if (mod) {
	    g_code (op_mov, IL4, &edx_reg, ap1);
	} else {
	    g_code (op_mov, IL4, &eax_reg, ap1);
	}
	return mk_legal (ap1, flags, ep->etp);
    default:
	return g_bin (ep, flags, op_idiv);
    }
}

/*
 * generate code for /= node
 */
static ADDRESS *g_asdiv P3 (const EXPR *, ep, FLAGS, flags, BOOL, mod)
{
    ADDRESS *ap1, *ap2;
    OPCODE  op = op_idiv;

    switch (ep->etp->type) {
    case bt_charu:
    case bt_uchar:
    case bt_ushort:
    case bt_uint16:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	op = op_div;
	/*FALLTHRU */
    case bt_char:
    case bt_schar:
    case bt_short:
    case bt_int16:
    case bt_int32:
    case bt_long:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, mod);
	}
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_MEM | F_DREG | F_EAXEDX));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_MEM));
	validate (ap1);
	if (op == op_idiv) {
	    g_code (op_cdq, IL0, NIL_ADDRESS, NIL_ADDRESS);
	} else {
	    g_code (op_xor, IL4, mk_high (ap1), mk_high (ap1));
	}
	g_code (op, IL4, ap2, NIL_ADDRESS);
	freeop (ap2);
	freeop (ap1);
	ap2 = data_register ();
	if (mod) {
	    g_code (op_mov, (ILEN) ep->etp->size, mk_high (ap1), ap2);
	} else {
	    g_code (op_mov, (ILEN) ep->etp->size, mk_low (ap1), ap2);
	}
	return mk_legal (ap2, flags, ep->etp);
    default:
	return g_asbin (ep, flags, op_idiv);
    }
}

/*============================================================================*/

/*
 * generate code to evaluate a multiply node
 */
static ADDRESS *g_mul P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;
    OPCODE  op = op_imul;

    switch (ep->etp->type) {
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	/*
	 * unless the result is extended to 64 bit, there should
	 * be no difference between imul and mul.
	 * NB. There is no mul instruction unless 64-bit extension
	 * is desired.
	 */
	/* op = op_mul; >>>This instruction does not exist<<< */
	/*FALLTHRU */
    case bt_int32:
    case bt_long:
	ap1 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_VOL));
	ap2 = g_expr (ep->v.p[0], F_ALL);
	validate (ap1);		/* in case push occurred */
	g_code (op, (ILEN) ep->etp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    default:
	return g_bin (ep, flags, op_imul);
    }
}

/*
 * generate code for *= node
 */
static ADDRESS *g_asmul P2 (const EXPR *, ep, FLAGS, flags)
{
    OPCODE  op;
    ILEN    ilen = IL0;
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
	op = op_movzwl;
	goto common;
    case bt_short:
    case bt_int16:
	op = op_movswl;
	goto common;
    case bt_uint32:
    case bt_int32:
    case bt_ulong:
    case bt_long:
    case bt_pointer32:
	op = op_mov;
	ilen = IL4;
      common:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op_imul, FALSE);
	}
	ap1 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_VOL));
	if (ilen == IL0) {
	    g_code (op, ilen, ap1, ap1);
	}
	ap2 = g_expr (ep->v.p[0], f);
	ap3 = data_register ();
	validate (ap1);
	g_code (op, ilen, ap2, ap3);
	g_code (op_imul, IL4, ap3, ap1);
	g_code (op_mov, (ILEN) ep->etp->size, ap1, ap2);
	freeop (ap3);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    default:
	return g_asbin (ep, flags, op_imul);
    }
}

/*============================================================================*/

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
    BOOL    result_is_void = FALSE;

    switch (ep->etp->type) {
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	flagx = F_MEM;
	break;
#endif /* FLOAT_IEEE */
    case bt_void:
	result_is_void = TRUE;
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

    /* all scratch registers are void */

    g_falsejp (ep->v.p[0], false_label);
    ep = ep->v.p[1];

    /* all registers are void */

    ap1 = g_expr (ep->v.p[0], flagx);
#ifdef FLOAT_IEEE
    if (flagx == F_MEM) {
	ADDRESS *ap;

	freeop (ap1);
	ap = data_register ();
	g_code (op_lea, IL4, ap1, ap);
	ap1 = copy_addr (ap, am_ind);
    }
#endif /* FLOAT_IEEE */
    freeop (ap1);

    /* all scratch registers are void */

    g_branch (end_label);
    g_label (false_label);

    ap2 = g_expr (ep->v.p[1], flagx);
#ifdef FLOAT_IEEE
    if (flagx == F_MEM) {
	ADDRESS *ap;

	freeop (ap2);
	ap = data_register ();
	g_code (op_lea, IL4, ap2, ap);
	ap2 = copy_addr (ap, am_ind);
    }
#endif /* FLOAT_IEEE */

    if (!result_is_void && !is_equal_address (ap1, ap2)) {
	FATAL ((__FILE__, "g_hook", "INCONSISTENCY"));
    }
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
    EXPR   *lnode = ep->v.bit.p;
    int     width = (int) lnode->v.bit.width;
    int     offset = (int) lnode->v.bit.offset;
    ILEN    ilen = (ILEN) ep->etp->size;
    UVAL    mask;

    /* Evaluate the address of the LHS */
    ap2 = g_expr (lnode->v.bit.p, F_DREG);
    ap2 = copy_addr (ap2, am_indx);

    /* Now get the value of the LHS, rotate and mask out unwanted bits */
    ap1 = data_register ();
    g_code (op_mov, ilen, ap2, ap1);
    g_rotate (ap1, ilen, offset, lnode->etp, width);

    /* now do the operation, masking the result back into the required size */
    switch (op) {
    case op_div:
    case op_idiv:
	/* evaluate the RHS */
	ap3 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED));
	validate (ap1);

	if (uses_temp (ap3) || ap3->mode == am_immed) {
/*KDW */
	    g_code (op_mov, IL4, ap3, &ecx_reg);
	    freeop (ap3);
	    ap3 = &ecx_reg;
	}
	g_code (op_mov, IL4, ap1, &eax_reg);
	switch (ep->etp->type) {
	case bt_long:
	case bt_int32:
	    g_code (op_cdq, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    break;
	default:
	    g_code (op_xor, IL4, &edx_reg, &edx_reg);
	    break;
	}
	g_code (op, IL4, ap3, NIL_ADDRESS);
	if (mod) {
	    g_code (op_mov, IL4, &edx_reg, ap1);
	} else {
	    g_code (op_mov, IL4, &eax_reg, ap1);
	}
	break;
    case op_asr:
    case op_shr:
    case op_shl:
	/* evaluate the RHS */
	ap3 = g_expr (ep->v.p[1], F_ALL);
	validate (ap1);

	if (ap3->mode != am_immed) {
	    g_code (op_mov, ilen, ap3, &ecx_reg);
	    g_code (op, ilen, &ecx_reg, ap1);
	    freeop (ap3);
	    break;
	}
	/*FALLTHRU */
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

    if ((FLAGS) (flags & F_NOVALUE) == F_NONE) {
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

    if (!uses_structassign) {
	FATAL ((__FILE__, "structassign", "USES"));
    }
    if (size == 4L) {
	if (ap1->mode == am_dreg) {
	    ap1 = mk_indirect (ap1->preg, NIL_EXPR);
	} else {
	    g_code (op_mov, IL4, ap1, &edi_reg);
	    ap1 = mk_indirect (EDI, NIL_EXPR);
	}
	if (ap2->mode == am_dreg) {
	    ap2 = mk_indirect (ap2->preg, NIL_EXPR);
	} else {
	    g_code (op_mov, IL4, ap2, &esi_reg);
	    ap2 = mk_indirect (ESI, NIL_EXPR);
	}
	ap3 = data_register ();
	g_code (op_mov, IL4, ap2, ap3);
	g_code (op_mov, IL4, ap3, ap1);
	freeop (ap3);
    } else {
	switch (ap2->mode) {
	case am_mreg:
	    g_code (op_mov, IL4, mk_high (ap1), mk_high (ap2));
	    g_code (op_mov, IL4, mk_low (ap1), mk_low (ap2));
	    break;
	default:
	    g_code (op_lea, IL4, ap1, &edi_reg);
	    g_code (op_lea, IL4, ap2, &esi_reg);
	    if ((size % 4L) == 0L) {
		ap3 = mk_immed ((SIZE) ((USIZE) size >> 2));
		ap3 = mk_legal (ap3, (FLAGS) (F_DREG | F_ECX), tp_long);
		g_code (op_rep, IL0, NIL_ADDRESS, NIL_ADDRESS);
		g_code (op_smov, IL4, NIL_ADDRESS, NIL_ADDRESS);
	    } else if ((size % 2L) == 0L) {
		ap3 = mk_immed ((SIZE) ((USIZE) size >> 1));
		ap3 = mk_legal (ap3, (FLAGS) (F_DREG | F_ECX), tp_short);
		g_code (op_rep, IL0, NIL_ADDRESS, NIL_ADDRESS);
		g_code (op_smov, IL2, NIL_ADDRESS, NIL_ADDRESS);
	    } else {
		ap3 = mk_immed (size);
		ap3 = mk_legal (ap3, (FLAGS) (F_DREG | F_ECX), tp_char);
		g_code (op_rep, IL0, NIL_ADDRESS, NIL_ADDRESS);
		g_code (op_smov, IL1, NIL_ADDRESS, NIL_ADDRESS);
	    }
	    freeop (ap3);
	    break;
	}
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

    switch (tp->type) {
    case bt_longdouble:
    case bt_double:
    case bt_float:
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
	    return mk_legal (ap1, flags, ep->etp);
	}
	/* FALLTHRU */
#endif /* FLOAT_IEEE */
    case bt_struct:
    case bt_union:
      array:
	ap2 = g_expr (ep->v.p[1], F_ALL);
	ap1 = g_expr (ep->v.p[0], F_ALL);
	validate (ap2);
	structassign (ap1, ap2, size);
	freeop (ap1);
	return mk_legal (ap2, flags, tp_pointer);
    case bt_pointer32:
	if (is_array_type (tp) || is_array_assignment (tp)) {
	    goto array;
	}
	/*FALLTHRU */
    default:
	switch (ep->v.p[0]->nodetype) {
	case en_fieldref:
	    /*
	     * Field assignment
	     */
	    /* get the value */
	    mask = bitmask (ep->v.p[0]->v.bit.width);
	    ap1 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED | F_VOL));
	    if (ap1->mode == am_immed) {
		ap1->u.offset->v.i &= (IVAL) mask;
		ap3 = mk_immed ((IVAL)
				((USIZE) ap1->u.offset->v.i << (int) ep->v.p
				 [0]->v.bit.offset));
	    } else {
		if (flags & F_NOVALUE) {
		    ap3 = ap1;
		    g_code (op_and, (ILEN) size, mk_immed ((IVAL) mask), ap3);
		} else {
		    if (is_signed_type (tp)) {
			SIZE    i =
			    tp_int->size * 8L -
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
	    ap1 = g_expr (ep->v.p[1], F_ALL);
	    ap2 = g_expr (ep->v.p[0], F_ALL);
	    validate (ap1);
	    switch (ap1->mode) {
	    default:
		if (ap2->mode != am_dreg) {
		    ap3 = data_register ();
		    g_code (op_mov, (ILEN) size, ap1, ap3);
		    g_code (op_mov, (ILEN) size, ap3, ap2);
		    freeop (ap3);
		    freeop (ap2);
		    freeop (ap1);
		    if (flags & F_NOVALUE) {
			ap1 = NIL_ADDRESS;
		    } else {
			ap1 = data_register ();
			g_code (op_mov, (ILEN) size, ap3, ap1);
		    }
		    break;
		}
		/*FALLTHRU */
	    case am_dreg:
	    case am_immed:
		g_code (op_mov, (ILEN) size, ap1, ap2);
		freeop (ap2);
		break;
	    case am_mreg:
		g_code (op_mov, (ILEN) size, mk_high (ap1), mk_high (ap2));
		g_code (op_mov, (ILEN) size, mk_low (ap1), mk_low (ap2));
		freeop (ap2);
		break;

	    }
	    break;
	}
	return mk_legal (ap1, flags, ep->etp);
    }
}

/*
 * push the operand expression onto the stack. return the number of bytes
 * pushed
 */
static SIZE push_param P1 (const EXPR *, ep)
{
    ADDRESS *ap;

#ifdef FLOAT_IEEE
    ADDRESS *ap1;

#endif /* FLOAT_IEEE */
    SIZE    size = ep->etp->size;

    switch (size) {
    case 1L:
    case 2L:
    case 3L:
	ap = g_expr (ep, F_DREG);
	g_code (op_push, IL4, ap, NIL_ADDRESS);
	break;
    case 4L:
	ap = g_expr (ep, F_ALL);
	switch (ap->mode) {
#ifdef FLOAT_IEEE
	case am_freg:
	    g_code (op_sub, IL4, mk_immed (size), &esp_reg);
	    ap1 = mk_indirect (ESP, NIL_EXPR);
	    g_fcode (op_fstp, (ILEN) size, ap1, NIL_ADDRESS);
	    break;
#endif /* FLOAT_IEEE */
	default:
	    g_code (op_push, IL4, ap, NIL_ADDRESS);
	    break;
	}
	break;
    case 8L:
	ap = g_expr (ep, F_ALL);
	switch (ap->mode) {
#ifdef FLOAT_IEEE
	case am_freg:
	    g_code (op_sub, IL4, mk_immed (size), &esp_reg);
	    ap1 = mk_indirect (ESP, NIL_EXPR);
	    g_fcode (op_fstp, (ILEN) size, ap1, NIL_ADDRESS);
	    break;
#endif /* FLOAT_IEEE */
	default:
	    g_code (op_push, IL4, mk_high (ap), NIL_ADDRESS);
	    g_code (op_push, IL4, mk_low (ap), NIL_ADDRESS);
	    break;
	}
	break;
    default:
	ap = g_expr (ep, F_ALL);
	g_code (op_sub, IL4, mk_immed (size), &esp_reg);
	structassign (&esp_reg, ap, size);
	break;
    }
    freeop (ap);
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
#if 0
	if (ap->mode == am_freg) {
	    g_fcode (op_fstp, IL10, mk_reg (ST0), NIL_ADDRESS);
	}
#endif
	return NIL_ADDRESS;
    }
    switch (tp->type) {
#ifdef FLOAT_IEEE
    case bt_float:
	if (fpu_option && fpu_return_option) {
	    ap = float_register ();
	} else {
	    ap = data_register ();
	}
	return ap;
    case bt_double:
    case bt_longdouble:
	if (fpu_option && fpu_return_option) {
	    ap = float_register ();
	} else {
	    ap = mdata_register ();
	}
	return ap;
#endif /* FLOAT_IEEE */
    default:
	if (flags & F_DREG) {
	    ap = data_register ();
	    if (ap->preg != reg_usage->result->reg[0]) {
		g_code (op_mov, IL4, &eax_reg, ap);
	    }
	    return ap;
	}
    }
    FATAL ((__FILE__, "func_result", "flags = 0x%x", flags));
    return NIL_ADDRESS;
}


/*
 * generate a function call node and return the address mode of the result.
 */
static ADDRESS *g_fcall P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap, *ap1;
    SIZE    size;
    EXPR   *ep0 = ep->v.p[0];

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
	     *       "Forced" stack optimisation.   This will not suppress
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
    size = g_parms (ep->v.p[1]);	/* generate parameters */
    /*
     * for functions returning a structure or a union, push a pointer to the
     * return value as additional argument The scratch space will be
     * allocated in the stack frame of the calling function.
     */
    if (is_structure_type (ep->etp)) {
	ap = mk_scratch (ep->etp->size);
	ap1 = data_register ();
	g_code (op_lea, IL4, ap, ap1);
	g_code (op_push, IL4, ap1, NIL_ADDRESS);
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
	ap = copy_addr (ap, am_ind);
	ap->u.offset = NIL_EXPR;
	freeop (ap);
	break;
    }
    g_code (op_call, IL0, ap, NIL_ADDRESS);
    ap = func_result (flags, size, ep->etp);
    return mk_legal (ap, flags, ep->etp);
}

/*
 * generates code for a en_cast node
 */
static ADDRESS *g_cast P4 (ADDRESS *, ap, TYP *, tp1, TYP *, tp2, FLAGS,
			   flags)
{
#ifdef FLOAT_IEEE
    ADDRESS *ap1;

#endif /* FLOAT_IEEE */

    if (flags & F_NOVALUE) {
#ifdef FLOAT_IEEE
	if (ap->mode == am_freg) {
	    g_fcode (op_fstp, IL10, mk_reg (ST0), NIL_ADDRESS);
	}
#endif /* FLOAT_IEEE */
	freeop (ap);
	return NIL_ADDRESS;
    }
    /* casts to a narrower integer type are no-ops since the 386 is low-endian */
    /* to avoid code duplication, float/double is shared */
    switch (tp2->type) {
	/* type to cast to */
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	switch (tp1->type) {
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    return mk_legal (ap, flags, tp2);
	case bt_uchar:
	case bt_schar:
	case bt_char:
	case bt_charu:
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	    ap = g_cast (ap, tp1, tp_long, F_ALL);
	    return g_cast (ap, tp_long, tp2, flags);
	case bt_int32:
	case bt_long:
	    /*
	     * For the conversion signed long --> float/double, there is
	     * a 387 instruction
	     */
	    if (fpu_option) {
		switch (ap->mode) {
		case am_direct:
		case am_ind:
		case am_indx:
		case am_indx2:
		    /*
		     * FPU-code for a signed long that is in memory
		     */
		    g_code (op_fild, IL4, ap, NIL_ADDRESS);
		    break;
		default:
		    /*
		     * FPU-code for a signed long that is in a register:
		     * the value is written to the stack and loaded from there
		     * since there is no direct path between CPU and FPU registers.
		     */
		    ap1 = mk_scratch (4L);
		    g_code (op_mov, IL4, ap, ap1);
		    g_code (op_fild, IL4, ap1, NIL_ADDRESS);
		}
		freeop (ap);
		ap = float_register ();
		return mk_legal (ap, flags, tp2);
	    }
	    /*FALLTHRU */
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    ap1 = data_register ();
	    g_code (op_xor, IL4, ap1, ap1);
	    g_code (op_push, IL4, ap1, NIL_ADDRESS);
	    g_code (op_push, IL4, ap, NIL_ADDRESS);
	    g_code (op_fildl, IL4, mk_indirect (ESP, NIL_EXPR), NIL_ADDRESS);
	    g_code (op_add, IL4, mk_immed (8L), mk_reg (ESP));
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
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    g_code (op_sub, IL4, mk_immed (4L), mk_reg (ESP));
	    g_fcode (op_fistp, IL8, mk_indirect (ESP, NIL_EXPR), NIL_ADDRESS);
	    freeop (ap);
	    ap = data_register ();
	    g_code (op_pop, IL4, ap, NIL_ADDRESS);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	case bt_uchar:
	case bt_schar:
	case bt_char:
	case bt_charu:
	case bt_ushort:
	case bt_short:
	case bt_int16:
	case bt_uint16:
	case bt_int32:
	case bt_uint32:
	case bt_ulong:
	case bt_long:
	case bt_pointer32:
	    return mk_legal (ap, flags, tp2);
	default:
	    break;
	}
	break;
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
	switch (tp1->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    g_code (op_sub, IL4, mk_immed (4L), mk_reg (ESP));
	    g_fcode (op_fistp, IL8, mk_indirect (ESP, NIL_EXPR), NIL_ADDRESS);
	    freeop (ap);
	    ap = data_register ();
	    g_code (op_pop, IL4, ap, NIL_ADDRESS);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
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
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_pointer32:
	case bt_ulong:
	    return mk_legal (ap, flags, tp2);
	default:
	    break;
	}
	break;
    case bt_int32:
    case bt_long:
	switch (tp1->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    g_code (op_sub, IL4, mk_immed (4L), mk_reg (ESP));
	    g_fcode (op_fistp, IL8, mk_indirect (ESP, NIL_EXPR), NIL_ADDRESS);
	    freeop (ap);
	    ap = data_register ();
	    g_code (op_pop, IL4, ap, NIL_ADDRESS);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	case bt_short:
	case bt_int16:
	case bt_ushort:
	case bt_uint16:
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_uint32:
	case bt_ulong:
	case bt_long:
	case bt_pointer32:
	case bt_func:
	    return mk_legal (ap, flags, tp1);
	default:
	    break;
	}
	break;
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	switch (tp1->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    g_code (op_sub, IL4, mk_immed (8L), mk_reg (ESP));
	    g_fcode (op_fistpl, IL8, mk_indirect (ESP, NIL_EXPR),
		     NIL_ADDRESS);
	    freeop (ap);
	    ap = data_register ();
	    ap1 = data_register ();
	    g_code (op_pop, IL4, ap, NIL_ADDRESS);
	    g_code (op_pop, IL4, ap1, NIL_ADDRESS);
	    freeop (ap1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	case bt_short:
	case bt_int16:
	case bt_ushort:
	case bt_uint16:
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_uint32:
	case bt_ulong:
	case bt_long:
	case bt_pointer32:
	case bt_func:
	    return mk_legal (ap, flags, tp1);
	default:
	    break;
	}
	break;
    default:
	break;
    }
    FATAL ((__FILE__, "g_cast", ""));
    return NIL_ADDRESS;
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
 * generate code to do a comparison of the two operands of node.
 * returns the op code for the branch to perform after the compare.
 */
static OPCODE g_compare P3 (const EXPR *, ep, OPCODE, op1, OPCODE, op2)
{
    EXPR   *ep0 = ep->v.p[0];
    EXPR   *ep1 = ep->v.p[1];
    ADDRESS *ap1, *ap2;
    FLAGS   flagx;
    TYP    *tp = ep0->etp;

    switch (tp->type) {
    case bt_schar:
    case bt_char:
    case bt_short:
    case bt_int16:
    case bt_int32:
    case bt_long:
	op1 = op2;
	/*FALLTHRU */
    case bt_uchar:
    case bt_charu:
    case bt_ushort:
    case bt_uint16:
    case bt_uint32:
    case bt_pointer32:
    case bt_ulong:
	ap1 = g_expr (ep0, F_ALL);
	flagx = (ap1->mode == am_immed) ? (FLAGS) (F_MEM | F_DREG) : F_DREG;
	ap2 = g_expr (ep1, flagx);
	validate (ap1);
	sync_stack ();
	g_code (op_cmp, (ILEN) tp->size, ap1, ap2);
	freeop (ap2);
	freeop (ap1);
	break;
#ifdef FLOAT_IEEE
    case bt_longdouble:
    case bt_double:
    case bt_float:
	if (fpu_option) {
	    ap1 = g_expr (ep0, F_MEM);
	    ap2 = g_expr (ep1, F_FREG);
	    validate (ap1);
	    g_fcode (op_fcomp, (ILEN) tp->size, ap1, NIL_ADDRESS);
	    if (is_register_used (EAX)) {
		g_code (op_push, IL4, &eax_reg, NIL_ADDRESS);
		g_code (op_fnstsw, IL0, &ax_reg, NIL_ADDRESS);
		g_code (op_sahf, IL0, NIL_ADDRESS, NIL_ADDRESS);
		g_code (op_pop, IL4, &eax_reg, NIL_ADDRESS);
	    } else {
		g_code (op_fnstsw, IL0, &ax_reg, NIL_ADDRESS);
		g_code (op_sahf, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    }
	    freeop (ap2);
	    freeop (ap1);
	} else {
	    push_rtl_params (ep0, ep1);
	    switch (tp->type) {
	    case bt_float:
		call_library (SUP_FPCMP);
		break;
	    case bt_double:
		call_library (SUP_FPCMP);
		break;
	    case bt_longdouble:
		call_library (SUP_FPCMP);
		break;
	    default:
		CANNOT_REACH_HERE ();
	    }
	}
	break;
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_compare", "illegal type %d", tp->type));
	break;
    }
    return op1;
}

static OPCODE g_test P2 (const EXPR *, ep, OPCODE, op1)
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
    case bt_int32:
    case bt_uint32:
    case bt_ulong:
    case bt_long:
    case bt_pointer32:
	ap = g_expr (ep, F_DREG);
	sync_stack ();
	g_code (op_test, (ILEN) ep->etp->size, ap, ap);
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
	    g_code (op_push, IL4, &eax_reg, NIL_ADDRESS);
	    g_code (op_fnstsw, IL0, &ax_reg, NIL_ADDRESS);
	    g_code (op_sahf, IL0, NIL_ADDRESS, NIL_ADDRESS);
	    g_code (op_pop, IL4, &eax_reg, NIL_ADDRESS);
	} else {
	    g_code (op_fnstsw, IL0, &ax_reg, NIL_ADDRESS);
	    g_code (op_sahf, IL0, NIL_ADDRESS, NIL_ADDRESS);
	}
	freeop (ap);
	break;
#endif /* FLOAT_IEEE */
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return op1;
}

/*
 * general expression evaluation. returns the addressing mode of the result.
 */
static ADDRESS *g_expr P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;
    LABEL   lab0, lab1;
    OPCODE  op;

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
	g_code (op_lea, IL4, ap2, ap1);
	return mk_legal (ap1, flags, ep->etp);
    case en_register:
	ap1 = mk_reg (ep->v.r);
	return mk_legal (ap1, flags, ep->etp);
    case en_ref:
	ap1 = g_deref (ep->v.p[0], ep->etp);
	if (is_structure_type (ep->etp) || is_array_type (ep->etp)) {
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
    case en_lsh:
	return g_shift (ep, flags, op_shl);
    case en_rsh:
	return g_shift (ep, flags, op_shr);
    case en_cond:
	return g_hook (ep, flags);
    case en_eq:
	op = g_compare (ep, op_sete, op_sete);
	goto cont1;
    case en_ne:
	op = g_compare (ep, op_setne, op_setne);
	goto cont1;
    case en_lt:
	op = g_compare (ep, op_seta, op_setg);
	goto cont1;
    case en_le:
	op = g_compare (ep, op_setae, op_setge);
	goto cont1;
    case en_gt:
	op = g_compare (ep, op_setb, op_setl);
	goto cont1;
    case en_ge:
	op = g_compare (ep, op_setbe, op_setle);
	goto cont1;
    case en_test:
	op = g_test (ep->v.p[0], op_setne);
	goto cont1;
    case en_not:
	op = g_test (ep->v.p[0], op_sete);
      cont1:
	ap1 = data_register ();
	g_code (op, IL1, ap1, NIL_ADDRESS);
	g_code (op_and, IL4, mk_immed (1L), ap1);
	return mk_legal (ap1, flags, ep->etp);
    case en_land:
    case en_lor:
	lab0 = nextlabel++;
	lab1 = nextlabel++;
	g_falsejp (ep, lab0);
	ap1 = data_register ();
	g_code (op_mov, IL4, mk_immed (1l), ap1);
	g_branch (lab1);
	g_label (lab0);
	g_code (op_mov, IL4, mk_immed (0l), ap1);
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
	return NIL_ADDRESS;
    }
}

PRIVATE void g_expression P1 (const EXPR *, ep)
{
    initstack ();
    if (ep != NIL_EXPR) {
	VOIDCAST g_expr (ep, (FLAGS) (F_ALL | F_NOVALUE));
    }
    checkstack ();
}

/*
 * generate a jump to label if the node passed evaluates to a true condition.
 */
static void g_truejp P2 (const EXPR *, ep, LABEL, label)
{
    LABEL   lab0;
    OPCODE  op;

    if (ep == NIL_EXPR) {
	FATAL ((__FILE__, "g_truejp", "ep == 0"));
    }
    switch (ep->nodetype) {
    case en_icon:
	if (ep->v.i) {
	    g_branch (label);
	}
	break;
    case en_eq:
	op = g_compare (ep, op_je, op_je);
	g_cbranch (op, label);
	break;
    case en_ne:
	op = g_compare (ep, op_jne, op_jne);
	g_cbranch (op, label);
	break;
    case en_lt:
	op = g_compare (ep, op_ja, op_jg);
	g_cbranch (op, label);
	break;
    case en_le:
	op = g_compare (ep, op_jae, op_jge);
	g_cbranch (op, label);
	break;
    case en_gt:
	op = g_compare (ep, op_jb, op_jl);
	g_cbranch (op, label);
	break;
    case en_ge:
	op = g_compare (ep, op_jbe, op_jle);
	g_cbranch (op, label);
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
    case en_not:
	op = g_test (ep->v.p[0], op_je);
	g_cbranch (op, label);
	break;
    case en_test:
	op = g_test (ep->v.p[0], op_jne);
	g_cbranch (op, label);
	break;
    case en_call:		/* library routine whish sets the flags */
	freeop (g_expr (ep, F_ALL));
	g_cbranch (op_jne, label);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}

/*
 * generate code to execute a jump to label if the expression passed is
 * false.
 */
static void g_falsejp P2 (const EXPR *, ep, LABEL, label)
{
    LABEL   lab0;
    OPCODE  op;

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
	op = g_compare (ep, op_jne, op_jne);
	g_cbranch (op, label);
	break;
    case en_ne:
	op = g_compare (ep, op_je, op_je);
	g_cbranch (op, label);
	break;
    case en_lt:
	op = g_compare (ep, op_jbe, op_jle);
	g_cbranch (op, label);
	break;
    case en_le:
	op = g_compare (ep, op_jb, op_jl);
	g_cbranch (op, label);
	break;
    case en_gt:
	op = g_compare (ep, op_jae, op_jge);
	g_cbranch (op, label);
	break;
    case en_ge:
	op = g_compare (ep, op_ja, op_jg);
	g_cbranch (op, label);
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
    case en_not:
	op = g_test (ep->v.p[0], op_jne);
	g_cbranch (op, label);
	break;
    case en_test:
	op = g_test (ep->v.p[0], op_je);
	g_cbranch (op, label);
	break;
    case en_call:		/* library routine whish sets the flags */
	freeop (g_expr (ep, F_ALL));
	g_cbranch (op_je, label);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}

PRIVATE void g_jtrue P2 (const EXPR *, ep, LABEL, label)
{
    initstack ();
    g_truejp (ep, label);
    checkstack ();
}

PRIVATE void g_jfalse P2 (const EXPR *, ep, LABEL, label)
{
    initstack ();
    g_falsejp (ep, label);
    checkstack ();
}

PRIVATE void g_switch_table P4 (const EXPR *, ep, const SWITCH *, sw, UVAL,
				min_caselabel, UVAL, max_caselabel)
{
    ADDRESS *ap, *ap1;

    initstack ();
    ap = g_expr (ep, (FLAGS) (F_DREG | F_VOL));
    ap = g_extend (ap, ep->etp, tp_long);
    /*
     * move the interval
     */
    max_caselabel -= min_caselabel;
    if (min_caselabel != 0) {
	g_code (op_sub, IL4, mk_immed ((IVAL) min_caselabel), ap);
    }
    g_code (op_cmp, (ILEN) ep->etp->size, mk_immed ((IVAL) max_caselabel),
	    ap);
    g_cbranch (op_ja, sw->deflab);
    g_code (op_shl, IL4, mk_immed (2l), ap);
    ap1 = mk_indirect (ap->preg, mk_lcon (sw->tablab));
    g_code (op_mov, IL4, ap1, ap);
    ap1 = copy_addr (ap, am_ind);
    ap1->u.offset = NIL_EXPR;
    /*
     * DO NOT USE OP_BRA here....
     * op_bra is reserved for jumps to internal labels.
     * This keeps things easy in the peephole optimizer
     * While producing assembler output, op_bra and op_jmp yield
     * the same
     */
    sync_stack ();
    g_code (op_jmp, IL0, ap1, NIL_ADDRESS);
    freeop (ap);
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
	    g_code (op_sub, (ILEN) ep->etp->size,
		    mk_immed (stmt->v2.i - (IVAL) min_value), ap);
	    min_value = (UVAL) stmt->v2.i;
	    stmt->v2.l = nextlabel++;
	    g_cbranch (op_je, stmt->v2.l);
	}
    }
    freeop (ap);
    checkstack ();
}

PRIVATE void g_entry P1 (SIZE, frame_size)
{
#ifdef STACK_CHECK
    if (stackcheck_option) {
	SYM    *sp;

	sp = internal_symbol (SUP_STACKCHECK, NIL_TYP);
	g_code (op_push, IL4, mk_immed (frame_size + max_stack_adjust),
		NIL_ADDRESS);
	g_code (op_call, IL0, mk_strlab (nameof (sp)), NIL_ADDRESS);
    }
#endif /* STACK_CHECK */
    if (frame_size == 0L) {
	g_code (op_push, IL4, mk_reg (regframe), NIL_ADDRESS);
	g_code (op_mov, IL4, mk_reg (STACKPTR), mk_reg (regframe));
    } else {
	g_code (op_enter, IL4, mk_immed (frame_size), mk_immed (0L));
    }
    max_stack_adjust = 0L;
}

PRIVATE void g_return P2 (const EXPR *, stmtexp, TYP *, tp)
{
    EXPR   *ep, *ep1;
    ADDRESS *ap;

    initstack ();
    switch (tp->type) {
    case bt_struct:
    case bt_union:
	uses_structassign = TRUE;
	/* assign structure */
	ep = mk_autocon ((SIZE) 8);
	ep = mk_ref (ep, tp_pointer);
	ep1 = mk_ref (ep, tp);
	ep1 = mk_node (en_assign, ep1, copynode (stmtexp), tp);
	VOIDCAST g_expr (ep1, (FLAGS) (F_ALL | F_NOVALUE));

	ap = g_expr (ep, F_ALL);
	g_code (op_mov, IL4, ap, mk_reg (reg_usage->result->reg[0]));
	freeop (ap);
	break;

#ifdef FLOAT_SUPPORT
    case bt_float:
    case bt_longdouble:
    case bt_double:
	/* return floating point value on top of fpu stack */
	/* FP values can be returned in the software stack,
	 * the 386 (in edx:eax), or the 387. Since the result
	 * of a fp expression can be either in the 387 or the
	 * software stack, it must be moved in a number of cases
	 *
	 * If you generate FP instructions, the most efficient
	 * way is clearly to return values in the 387, while
	 * with FP emulation it is most efficient to return them
	 * in the software stack. If you want to link with output
	 * of other compilers, you may need other options.
	 * SysV calling conventions mandate fp return values in the
	 * 387, even if soft fp is used. Gcc 2.x for Minix uses
	 * edx:eax, which is probably the best compromise: It is
	 * well defined (which a software stack is not) and doesn't
	 * need a 387 (or kernel emulation).
	 */
	if (fpu_option && fpu_return_option) {
	    ap = g_expr (stmtexp, F_FREG);
	} else {
	    ADDRESS *ap1;

	    ap = g_expr (stmtexp, F_MEM);
	    g_code (op_mov, IL4, ap, mk_reg (reg_usage->result->reg[0]));
	    ap1 = copy_addr (ap, ap->mode);
	    ap1->u.offset = mk_add (ap1->u.offset, mk_const (4L));
	    g_code (op_mov, IL4, ap1, mk_reg (reg_usage->result->reg[1]));
	}
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
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap = g_expr (stmtexp, F_ALL);
	g_code (op_mov, (ILEN) stmtexp->etp->size, ap,
		mk_reg (reg_usage->result->reg[0]));
	freeop (ap);
	break;
    default:
	FATAL ((__FILE__, "g_return", "illegal type %d", tp->type));
    }
    checkstack ();
}

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
	stackoffset = lc_auto_max + max_scratch + (4L * (SIZE) regs_used);
	ap = mk_indirect (EBP, mk_const (-stackoffset));
	g_code (op_lea, IL4, ap, mk_reg (ESP));
    }
    /*
     * pop clobbered register variables
     */
    for (reg = EAX; reg <= ESI; reg++) {
	if (restore_mask & (REGBIT (reg))) {
	    g_code (op_pop, IL4, mk_reg (reg), NIL_ADDRESS);
	}
    }

    g_code (op_leave, IL0, NIL_ADDRESS, NIL_ADDRESS);
    g_code (op_ret, IL0, NIL_ADDRESS, NIL_ADDRESS);
}

/*
 * allocate will allocate registers for the expressions that have a high
 * enough desirability.
 */
PRIVATE void g_allocate P1 (CSE *, olist)
{
    CSE    *csp;
    REG     regs_to_use[3];
    int     number_of_regs;
    int     reg_use_ptr;
    USES    uses;
    REGMASK mask = (REGMASK) 0;
    REG     reg;

    /*
     * On Sun386i, ebx gets clobbered calling Sun library functions --
     * cannot be used
     */
    if (uses_structassign) {
	number_of_regs = 1;
	regs_to_use[0] = EBX;
	mask = (REGMASK) (REGBIT (ESI) | REGBIT (EDI));
	regs_used = 2;
    } else {
	number_of_regs = 3;
	regs_to_use[0] = EBX;
	regs_to_use[1] = EDI;
	regs_to_use[2] = ESI;
	mask = (REGMASK) 0;
	regs_used = 0;
    }
    reg_use_ptr = 0;
    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	/*
	 * If reg_option is not true, the 'desire' value must be at least
	 * 5000, which I hope can only be achieved by the 'register' attribute
	 */
	uses = desire (csp);
	if (uses < (USES) 3 || (!reg_option && uses < (USES) 5000)) {
	    csp->reg = NO_REG;
	} else if (reg_use_ptr < number_of_regs
		   /*
		    * integer constants may have different types
		    */
		   && csp->exp->nodetype != en_icon
		   /*
		    * the types which are fine: one-byte quantities are illegal
		    * in some of the registers, e.g. %edi and %esi
		    */
		   && (csp->exp->etp->type == bt_pointer32 ||
		       csp->exp->etp->type == bt_long ||
		       csp->exp->etp->type == bt_ulong ||
		       csp->exp->etp->type == bt_int16 ||
		       csp->exp->etp->type == bt_uint16 ||
		       csp->exp->etp->type == bt_int32 ||
		       csp->exp->etp->type == bt_uint32 ||
		       csp->exp->etp->type == bt_short ||
		       csp->exp->etp->type == bt_ushort)) {

	    csp->reg = regs_to_use[reg_use_ptr++];
	    regs_used++;
	    mask |= REGBIT (csp->reg);
	} else {
	    csp->reg = NO_REG;
	}
    }

    /*
     *       Now take into account which registers must be saved by the
     *       function.
     */
    mask &= reglist_to_mask (reg_usage->save);

    for (reg = ESI; reg >= EAX; reg--) {
	if (mask & (REGBIT (reg))) {
	    g_code (op_push, IL4, mk_reg (reg), NIL_ADDRESS);
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

PRIVATE void g_auto_align P0 (void)
{
    if (lc_auto_max % AL_DEFAULT != 0L) {
	lc_auto_max += AL_DEFAULT - (lc_auto_max % AL_DEFAULT);
    }
}

PRIVATE BOOL g_is_bigendian P0 (void)
{
    return FALSE;
}

PRIVATE BOOL g_is_ascending_stack P0 (void)
{
    return FALSE;
}

/*
 *   This routine does any code generator specific transformations
 *   on the expression tree.
 *
 *   For example it can replace operator nodes with calls to runtime
 *   routines.   This allows the global optimiser to perform optimisations
 *   on such calls which wouldn't be possible if the calls were
 *   generated in the code generator routines themselves.
 */

PRIVATE EXPR *g_transform P1 (EXPR *, ep)
{
    TYP    *tp;

#ifdef FLOAT_SUPPORT
    SIZE    size;
    EXPR   *ep1;
    SYM    *sp;

#endif /* FLOAT_SUPPORT */

    if (ep == NIL_EXPR) {
	return ep;
    }
    tp = ep->etp;
    switch (ep->nodetype) {
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
    case en_fcon:
	ep = mk_lcon (mk_flabel (&(ep->v.f), tp));
	ep = mk_ref (ep, tp);
	return ep;
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */
    case en_icon:
    case en_nacon:
    case en_labcon:
    case en_autocon:
    case en_sym:
    case en_str:
	break;

    case en_add:
	if (fpu_option) {
	    return ep;
	}
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    return transform_binary (ep, SUP_FPFADD);
	case bt_double:
	    return transform_binary (ep, SUP_FPADD);
	case bt_longdouble:
	    return transform_binary (ep, SUP_FPLADD);
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_sub:
	if (fpu_option) {
	    return ep;
	}
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    return transform_binary (ep, SUP_FPFSUB);
	case bt_double:
	    return transform_binary (ep, SUP_FPSUB);
	case bt_longdouble:
	    return transform_binary (ep, SUP_FPLSUB);
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_mul:
	if (fpu_option) {
	    return ep;
	}
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    return transform_binary (ep, SUP_FPFMULT);
	case bt_double:
	    return transform_binary (ep, SUP_FPMULT);
	case bt_longdouble:
	    return transform_binary (ep, SUP_FPLMULT);
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_div:
	if (fpu_option) {
	    return ep;
	}
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    return transform_binary (ep, SUP_FPFDIV);
	case bt_double:
	    return transform_binary (ep, SUP_FPDIV);
	case bt_longdouble:
	    return transform_binary (ep, SUP_FPLDIV);
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_ainc:
	if (fpu_option) {
	    return ep;
	}
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    return transform_unary_ref (ep, SUP_FPFINC);
	case bt_double:
	    return transform_unary_ref (ep, SUP_FPINC);
	case bt_longdouble:
	    return transform_unary_ref (ep, SUP_FPLINC);
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_adec:
	if (fpu_option) {
	    return ep;
	}
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    return transform_unary_ref (ep, SUP_FPFDEC);
	case bt_double:
	    return transform_unary_ref (ep, SUP_FPDEC);
	case bt_longdouble:
	    return transform_unary_ref (ep, SUP_FPLDEC);
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_test:
	if (fpu_option) {
	    return ep;
	}
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    return transform_unary_ref (ep, SUP_FPFTST);
	case bt_double:
	    return transform_unary_ref (ep, SUP_FPTST);
	case bt_longdouble:
	    return transform_unary_ref (ep, SUP_FPLTST);
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_uminus:
	if (fpu_option) {
	    return ep;
	}
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    return transform_unary_ref (ep, SUP_FPFNEG);
	case bt_double:
	    return transform_unary_ref (ep, SUP_FPNEG);
	case bt_longdouble:
	    return transform_unary_ref (ep, SUP_FPLNEG);
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_cast:
	switch (tp->type) {
	case bt_char:
	case bt_schar:
	case bt_short:
	case bt_int16:
	case bt_int32:
	case bt_long:
	    if (fpu_option) {
		return ep;
	    }
	    switch (ep->v.p[0]->etp->type) {
#ifdef FLOAT_IEEE
	    case bt_float:
		return transform_unary (ep, SUP_SFTOL);
	    case bt_double:
		return transform_unary (ep, SUP_DFTOL);
	    case bt_longdouble:
		return transform_unary (ep, SUP_LFTOL);
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
	case bt_uchar:
	case bt_charu:
	case bt_ushort:
	case bt_uint16:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    if (fpu_option) {
		return ep;
	    }
	    switch (ep->v.p[0]->etp->type) {
#ifdef FLOAT_IEEE
	    case bt_float:
		return transform_unary (ep, SUP_SFTOUL);
	    case bt_double:
		return transform_unary (ep, SUP_DFTOUL);
	    case bt_longdouble:
		return transform_unary (ep, SUP_LFTOUL);
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
	case bt_float:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_char:
	    case bt_schar:
	    case bt_short:
	    case bt_int16:
		if (!fpu_option)
		    /*FALLTHRU */
	    case bt_charu:
	    case bt_uchar:
	    case bt_ushort:
	    case bt_uint16:
		    ep->v.p[0] =
			mk_node (en_cast, ep->v.p[0], NIL_EXPR, tp_long);
		/* FALLTHRU */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_LTOSF);
		}
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_ULTOSF);
		}
		return ep;
	    case bt_double:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_DFTOSF);
		}
		return ep;
	    case bt_longdouble:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_LFTOSF);
		}
		return ep;
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
	case bt_double:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_char:
	    case bt_schar:
	    case bt_short:
	    case bt_int16:
		if (!fpu_option)
		    /*FALLTHRU */
	    case bt_charu:
	    case bt_uchar:
	    case bt_ushort:
	    case bt_uint16:
		    ep->v.p[0] =
			mk_node (en_cast, ep->v.p[0], NIL_EXPR, tp_long);
		/* FALLTHRU */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_LTODF);
		}
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_ULTODF);
		}
		return ep;
	    case bt_float:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_SFTODF);
		}
		return ep;
	    case bt_longdouble:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_LFTODF);
		}
		return ep;
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
	case bt_longdouble:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_char:
	    case bt_charu:
	    case bt_schar:
	    case bt_uchar:
	    case bt_short:
	    case bt_ushort:
	    case bt_int16:
	    case bt_uint16:
		ep->v.p[0] = mk_node (en_cast, ep->v.p[0], NIL_EXPR, tp_long);
		/* FALLTHRU */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_LTOLF);
		}
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_ULTOLF);
		}
		return ep;
	    case bt_float:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_SFTOLF);
		}
		return ep;
	    case bt_double:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_DFTOLF);
		}
		return ep;
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
	default:
	    return ep;
	}
    case en_eq:
    case en_ne:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
    case en_mod:
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
    case en_asadd:
    case en_assub:
    case en_asmul:
    case en_asdiv:
    case en_asor:
    case en_asxor:
    case en_asand:
    case en_asmod:
    case en_aslsh:
    case en_asrsh:
    case en_fcall:
    case en_call:
    case en_usercall:
    case en_assign:
    case en_ref:
    case en_fieldref:
    case en_not:
    case en_compl:
    case en_deref:
	break;
#ifdef FLOAT_SUPPORT
    case en_asmul2:
	sp = internal_symbol (SUP_ASMUL, ep->etp);
	goto cont1;
    case en_asdiv2:
	sp = internal_symbol (SUP_ASDIV, ep->etp);
      cont1:
	size = (SIZE) ((USIZE) (ep->etp->size * 8L) << 8);	/* size of LHS in bits * 256 */
	switch (ep->v.p[0]->nodetype) {
	case en_fieldref:
	    size = (SIZE) (((USIZE) ep->v.p[0]->v.bit.width) << 8);
	    size |= (tp_long->size * 8L) -
		(SIZE) (ep->v.p[0]->v.bit.offset) -
		(SIZE) (ep->v.p[0]->v.bit.width);
	    /*FALLTHRU */
	case en_ref:
	    size |= (SIZE) ((USIZE) ep->v.p[1]->etp->size << 16);	/* size of RHS */
	    size |= (SIZE) ((USIZE) is_unsigned_type (ep->etp) << 24);	/* sign of LHS */
	    ep->nodetype = en_call;
	    ep1 = mk_node (en_list, ep->v.p[0]->v.p[0], NIL_EXPR, tp_void);
	    ep1 = mk_node (en_list, ep->v.p[1]->v.p[0], ep1, tp_void);
	    ep1 = mk_node (en_list, mk_icon (size, tp_long), ep1, tp_void);
	    ep->v.p[1] = ep1;
	    ep->v.p[0] = mk_symnode (sp);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	break;
#endif /* FLOAT_SUPPORT */
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
#define	MC386_FUNCS	(void *)&mc386_funcs
#else
#define	MC386_FUNCS	(void *)NULL
#endif /* MULTIPLE_PROCESSORS */

static OPTENUM yesnoopts[] = {
    {(const CHAR *) "yes", 1},
    {(const CHAR *) "no", 0},
    {(const CHAR *) NULL, 0}
};

static OPTENUM stackoptions[] = {
    {(const CHAR *) "safest", OPT_SAFE},
    {(const CHAR *) "minimum", OPT_MINIMUM},
    {(const CHAR *) "average", OPT_AVERAGE},
    {(const CHAR *) "maximum", OPT_MAXIMUM},
    {(const CHAR *) NULL, 0}
};

static OPTSET peepset[] = {
    {(const CHAR *) "none", PEEP_NONE},
    {(const CHAR *) "flow", PEEP_FLOW},
    {(const CHAR *) "all", PEEP_ALL},
    {(const CHAR *) NULL, 0}
};

static OPTION opts[] = {
    {
     (const CHAR *) "fpu=", enumeration_option,
     {&fpu_option},
     {&yesnoopts[0]}
     },
    {
     (const CHAR *) "fpureturn=", enumeration_option,
     {&fpu_return_option},
     {&yesnoopts[0]}
     },
    {
     (const CHAR *) "peep=", set_option,
     {&peep_option},
     {&peepset[0]}
     },
    {
     (const CHAR *) "prefix=", string_option,
     {(void *) &external_prefix},
     {NULL}
     },
    {
     (const CHAR *) "reg=", enumeration_option,
     {&reg_option},
     {&yesnoopts[0]}
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
     (const CHAR *) "nasm386", chip_option,
     {&nasmx86_func},
     {MC386_FUNCS}
     },
#endif /* TARGET_MASM */
#ifdef TARGET_MASM
    {
     (const CHAR *) "masm386", chip_option,
     {&masmx86_func},
     {MC386_FUNCS}
     },
#endif /* TARGET_MASM */
#ifdef TARGET_BAS
    {
     (const CHAR *) "bas386", chip_option,
     {&basx86_func},
     {MC386_FUNCS}
     },
#endif /* TARGET_BAS */
#ifdef TARGET_GAS
    {
     (const CHAR *) "gas386", chip_option,
     {&gasx86_func},
     {MC386_FUNCS}
     },
#endif /* TARGET_GAS */
#ifdef TARGET_SYSV
    {
     (const CHAR *) "sysv386", chip_option,
     {&sysvx86_func},
     {MC386_FUNCS}
     },
#endif /* TARGET_SYSV */
#else
#ifdef TARGET_MASM
    {
     (const CHAR *) "masm386", chip_option,
     {NULL},
     {MC386_FUNCS}
     },
#endif /* TARGET_MASM */
#ifdef TARGET_BAS
    {
     (const CHAR *) "bas386", chip_option,
     {NULL},
     {MC386_FUNCS}
     },
#endif /* TARGET_BAS */
#ifdef TARGET_GAS
    {
     (const CHAR *) "gas386", chip_option,
     {NULL},
     {MC386_FUNCS}
     },
#endif /* TARGET_GAS */
#ifdef TARGET_SYSV
    {
     (const CHAR *) "sysv386", chip_option,
     {NULL},
     {MC386_FUNCS}
     },
#endif /* TARGET_SYSV */
#endif /* MULTIPLE_ASSEMBLERS */
    {
     (const CHAR *) NULL, NULL,
     {NULL},
     {NULL}
     }
};

OPTIONS opts386 = {
    (const CHAR *) "Intel 386 ",
    opts
};

#ifdef MULTIPLE_PROCESSORS
struct genfuncs mc386_funcs = {
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
    &alignments_386[0]
};

#undef GEN_MODULE
#endif /* MULTIPLE_PROCESSORS */
#endif /* INTEL_386 */
