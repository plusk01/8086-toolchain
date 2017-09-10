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

#include "config.h"

#ifdef MC680X0

/*****************************************************************************/

#define GEN_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "gen68k.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define AL_DEFAULT	(g_alignments[bt_ellipsis])

/********************************************************** Static Variables */

/*
 *	 Command line options
 */
static unsigned peep_option = (MEMBER (PEEP_INSTRUCTION) | MEMBER (PEEP_JUMPS));	/* peehole optimisations */
static int stackopt_option = OPT_MINIMUM;	/* Use lazy stack optimisation */
static int regframe_option = (int) A6;
static int regdata_option = (int) A5;
static SETVAL regunused_option = Ox0UL;
static SETVAL regtemp_option =
    (MEMBER (D0) | MEMBER (D1) | MEMBER (D2) | MEMBER (A0) | MEMBER (A1) |
     MEMBER (FP0) | MEMBER (FP1) | MEMBER (FP2));
static int codemodel_option = (int) model_absolute;
static int interrupt_option = 0;

#ifdef COLDFIRE_DEFAULT
int     target_option = (int) target_coldfire;

#else
int     target_option = (int) target_68000;

#endif /* COLDFIRE_DEFAULT */

static int regs_used = 0;	/* number of register variable allocated */
static int fregs_used = 0;	/* number of FP register variable allocated */
static REGMASK restore_mask;	/* register restore mask */

#ifdef FLOAT_IEEE
static REGMASK restore_fmask;	/* floating point register restore mask */

#endif /* FLOAT_IEEE */
static SIZE max_stack_adjust = 0L;	/* largest amount stack is altered */
static REG regframe = FRAMEPTR;
static REG regdata = DATAPTR;

static LISTVAL regreturn_list = { 0 };

static ADDRESS push = {
    am_adec, STACKPTR, (REG) 0, (DEEP) 0,
    {NIL_EXPR}		/*lint !e708*/	/* union initialization */
};
static ADDRESS pop = {
    am_ainc, STACKPTR, (REG) 0, (DEEP) 0,
    {NIL_EXPR}		/*lint !e708*/	/* union initialization */
};

/*
 *	 The following tables specify the alignment requirements of the
 *	 basic types depending on the processor type.
 */
static SIZE alignments_68000[] = {
    1L,				/* bt_void              */
    1L,				/* bt_bool              */
    1L,				/* bt_char              */
    1L,				/* bt_charu     */
    1L,				/* bt_uchar     */
    1L,				/* bt_schar     */
    2L,				/* bt_short     */
    2L,				/* bt_ushort    */
    2L,				/* bt_int16     */
    2L,				/* bt_uint16    */
    2L,				/* bt_int32     */
    2L,				/* bt_uint32    */
    2L,				/* bt_long              */
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
    2L,				/* bt_func              */
    2L,				/* bt_bitfield  */
    2L,				/* bt_ubitfield */
    1L,				/* bt_bbitfield */
    2L				/* bt_ellipsis - used for alignment suitable for all types */
};

#ifndef MULTIPLE_PROCESSORS
PRIVATE SIZE *g_alignments = &alignments_68000[0];

#endif /* MULTIPLE_PROCESSORS */

/*********************************************** Static Function Definitions */

static ADDRESS *g_addsub P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_aincdec P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asadd P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asbitfield P_ ((const EXPR *, FLAGS, OPCODE, BOOL));
static ADDRESS *g_asdiv P_ ((const EXPR *, FLAGS, BOOL));
static ADDRESS *g_aslogic P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asmul P_ ((const EXPR *, FLAGS));
static ADDRESS *g_asshift P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_assign P_ ((const EXPR *, FLAGS));
static ADDRESS *g_cast P_ ((ADDRESS *, const TYP *, const TYP *, FLAGS));
static ADDRESS *g_deref P_ ((const EXPR *, const TYP *, FLAGS));
static ADDRESS *g_div P_ ((const EXPR *, FLAGS, BOOL));
static ADDRESS *g_expr P_ ((const EXPR *, FLAGS));
static ADDRESS *g_extend P_ ((ADDRESS *, const TYP *, const TYP *));
static ADDRESS *g_fcall P_ ((const EXPR *, FLAGS));
static ADDRESS *g_fderef P_ ((const EXPR *, FLAGS));
static ADDRESS *g_hook P_ ((const EXPR *, FLAGS));
static ADDRESS *g_index P_ ((const EXPR *));
static ADDRESS *g_logic P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_mul P_ ((const EXPR *, FLAGS));
static ADDRESS *mk_offset P_ ((ADDRESS *, SIZE));
static ADDRESS *g_shift P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_unary P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_xmul P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_shft P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));
static ADDRESS *mk_amode P_ ((AMODE));
static ADDRESS *mk_expr P_ ((AMODE, EXPR *));
static ADDRESS *mk_direct P_ ((EXPR *));
static ADDRESS *mk_high P_ ((ADDRESS *));
static ADDRESS *mk_top P_ ((ADDRESS *));
static ADDRESS *mk_indirect P_ ((REG, EXPR *));
static ADDRESS *mk_legal P_ ((ADDRESS *, FLAGS, const TYP *));
static ADDRESS *mk_low P_ ((ADDRESS *));
static ADDRESS *mk_rmask P_ ((REGMASK));
static ADDRESS *mk_smask P_ ((REGMASK));
static BOOL g_compare P_ ((const EXPR *));
static BOOL is_byte P_ ((const EXPR *));
static BOOL is_ushort P_ ((const EXPR *));
static BOOL tst_short P_ ((const EXPR *));
static BOOL tst_ushort P_ ((const EXPR *));
static SIZE g_parms P_ ((const EXPR *));
static SIZE push_param P_ ((const EXPR *));
static void g_call P_ ((ADDRESS *));
static void g_cbranch P_ ((OPCODE, LABEL));
static void g_falsejp P_ ((const EXPR *, LABEL));
static void g_immed P_ ((OPCODE, const TYP *, IVAL, ADDRESS *));
static void g_rotate P_ ((ADDRESS *, const TYP *, int, const TYP *, int));
static void g_test P_ ((const EXPR *));
static void g_truejp P_ ((const EXPR *, LABEL));
static void structassign P_ ((ADDRESS *, ADDRESS *, SIZE, const TYP *));
static ADDRESS *func_result P_ ((FLAGS, SIZE, const TYP *));

#ifdef FLOAT_SUPPORT
static void push_rtl_params P_ ((const EXPR *, const EXPR *));
#endif /* FLOAT_SUPPORT */

/*********************************************** Global Function Definitions */

#ifdef MULTIPLE_PROCESSORS
PRIVATE BOOL g_is_ascending_stack P_ ((void));
PRIVATE BOOL g_is_bigendian P_ ((void));
PRIVATE EXPR *g_transform P_ ((EXPR *));
PRIVATE void g_allocate P_ ((CSE *));
PRIVATE void g_auto_align P_ ((void));
PRIVATE void g_entry P_ ((SIZE));
PRIVATE void g_epilogue P_ ((void));
PRIVATE void g_expression P_ ((const EXPR *));
PRIVATE void g_flush P_ ((SYM *));
PRIVATE void g_initialize P_ ((void));
PRIVATE void g_jfalse P_ ((const EXPR *, LABEL));
PRIVATE void g_jtrue P_ ((const EXPR *, LABEL));
PRIVATE void g_label P_ ((LABEL));
PRIVATE void g_return P_ ((const EXPR *, TYP *));
PRIVATE void g_stack P_ ((SIZE));
PRIVATE void g_switch_compare P_ ((const EXPR *, STMT *));
PRIVATE void g_switch_table P_ ((const EXPR *, const SWITCH *, UVAL, UVAL));

#endif /* MULTIPLE_PROCESSORS */

/*****************************************************************************/

/*
 * this module contains all of the code generation routines for evaluating
 * expressions and conditions.
 */

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
 * copy an address mode structure.
 */
ADDRESS *copy_addr P2 (const ADDRESS *, ap, AMODE, mode)
{
    ADDRESS *newap;

    assert (ap);
    newap = (ADDRESS *) xalloc (sizeof (ADDRESS));

    *newap = *ap;
    newap->mode = mode;
    return newap;
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
 * generate a direct reference to a string label.
 */
static ADDRESS *mk_strlab P1 (const CHAR *, s)
{
    EXPR   *ep;

    ep = mk_node (en_nacon, NIL_EXPR, NIL_EXPR, tp_void);
    ep->v.str = s;
    return mk_expr (am_direct, ep);
}

#if defined(FLOAT_IEEE) || defined(STACK_CHECK)
/*
 * generate a call to a library routine.
 * it is assumed that lib_name won''t be clobbered
 */
static void call_library P1 (const CHAR *, lib_name)
{
    SYM    *sp;

    sp = internal_symbol (lib_name, tp_void);
    g_call (mk_strlab (nameof (sp)));
}

#endif /* FLOAT_IEEE || STACK_CHECK */

/*
 *	 make an address reference to a register.
 */
ADDRESS *mk_reg P1 (REG, r)
{
    ADDRESS *ap;

    switch (r) {
    case D0:
    case D1:
    case D2:
    case D3:
    case D4:
    case D5:
    case D6:
    case D7:
	ap = mk_amode (am_dreg);
	ap->preg = r;
	break;
    case A0:
    case A1:
    case A2:
    case A3:
    case A4:
    case A5:
    case A6:
    case A7:
	ap = mk_amode (am_areg);
	ap->preg = r;
	break;
    case FP0:
    case FP1:
    case FP2:
    case FP3:
    case FP4:
    case FP5:
    case FP6:
    case FP7:
	ap = mk_amode (am_freg);
	ap->preg = r;
	break;
    default:
	CANNOT_REACH_HERE ();
    }
    return ap;
}

/*
 *	 make an address reference to 3 registers.
 */
ADDRESS *mk_xreg P3 (REG, r1, REG, r2, REG, r3)
{
    ADDRESS *ap;

    ap = mk_amode (am_xreg);
    ap->preg = r1;
    ap->sreg = r2;
    ap->u.xreg = r3;
    return ap;
}

/*
 *	 make an address reference to 2 registers.
 */
ADDRESS *mk_mreg P2 (REG, r1, REG, r2)
{
    ADDRESS *ap;

    ap = mk_amode (am_mreg);
    ap->preg = r1;
    ap->sreg = r2;
    return ap;
}

/*
 * returns addressing mode of form offset(regframe)
 * size is rounded up to AL_DEFAULT
 */
static ADDRESS *mk_scratch P1 (SIZE, size)
{
    ADDRESS *ap;
    SIZE    default_alignment = AL_DEFAULT;

    /* round up the request */
    if (size % default_alignment) {
	size += default_alignment - (size % default_alignment);
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
    ap =
	mk_indirect (regframe,
		     mk_const ((IVAL) -(lc_auto_max + act_scratch)));
    return ap;
}

/*
 * generate the mask address structure.
 */
static ADDRESS *mk_rmask P1 (REGMASK, mask)
{
    ADDRESS *ap;

    ap = mk_amode (am_rmask);
    ap->u.mask = mask;
    return ap;
}

/*
 * generate the mask address structure.
 */
static ADDRESS *mk_smask P1 (REGMASK, mask)
{
    ADDRESS *ap;

    ap = mk_amode (am_smask);
    ap->u.mask = mask;
    return ap;
}

/*
 * make a direct reference to a node.
 */
static ADDRESS *mk_direct P1 (EXPR *, ep)
{
    return mk_expr (am_direct, ep);
}

/*
 * make an indirect reference to a node
 */
static ADDRESS *mk_indirect P2 (REG, reg, EXPR *, ep)
{
    ADDRESS *ap;

    ap = mk_expr (am_indx, ep);
    ap->preg = reg;
    return ap;
}

/*
 * return true if the node passed can be generated as a short offset.
 */
BOOL is_short P1 (const EXPR *, ep)
{
    return is_icon (ep) && (ep->v.i >= -32768L && ep->v.i <= 32767L);
}

/*
 * return true if the node passed can be generated as a short offset.
 */
static BOOL is_ushort P1 (const EXPR *, ep)
{
    return is_icon (ep) && (ep->v.i >= 0L && ep->v.i <= 65535L);
}

static BOOL is_byte P1 (const EXPR *, ep)
{
    return is_icon (ep) && (ep->v.i >= -128L && ep->v.i <= 127L);
}

/*
 * tests if node is a integer constant falling in the range of uns. short or
 * if node is cast from uns. short, uns. char or char.
 */
static BOOL tst_ushort P1 (const EXPR *, ep)
{
    if (is_ushort (ep)) {
	return TRUE;
    }
    switch (ep->etp->type) {
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
    case bt_ushort:
    case bt_uint16:
	return TRUE;
    default:
	break;
    }
    if (ep->nodetype == en_cast) {
	switch (ep->v.p[0]->etp->type) {
	case bt_char:
	case bt_charu:
	case bt_uchar:
	case bt_schar:
	case bt_ushort:
	case bt_uint16:
	    return TRUE;
	default:
	    return FALSE;
	}
    }
    return FALSE;
}

/*
 * tests if node is a integer constant falling in the range of short or if
 * node is cast from short.
 */
static BOOL tst_short P1 (const EXPR *, ep)
{
    if (is_short (ep)) {
	return TRUE;
    }
    switch (ep->etp->type) {
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
    case bt_short:
    case bt_int16:
	return TRUE;
    default:
	break;
    }
    if (ep->nodetype == en_cast) {
	switch (ep->v.p[0]->etp->type) {
	case bt_char:
	case bt_charu:
	case bt_uchar:
	case bt_schar:
	case bt_short:
	case bt_int16:
	    return TRUE;
	default:
	    return FALSE;
	}
    }
    return FALSE;
}

void g_move P3 (ILEN, len, ADDRESS *, ap1, ADDRESS *, ap2)
{
    switch (ap2->mode) {
    case am_areg:
	g_code (op_movea, len, ap1, ap2);
	break;
    default:
	g_code (op_move, len, ap1, ap2);
	break;
    }
}

static void g_movem P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
#ifdef COLDFIRE
    if (is_coldfire ()) {
	switch (ap1->mode) {
	case am_ainc:
	    ap1 = copy_addr (ap1, am_ind);
	    g_code (op_movem, IL4, ap1, ap2);
	    g_code (op_add, IL4,
		    mk_immed (4L * count_registers (ap2->u.mask)),
		    mk_reg (ap1->preg));
	    return;
	case am_adec:
	    g_code (op_sub, IL4,
		    mk_immed (4L * count_registers (ap2->u.mask)),
		    mk_reg (ap1->preg));
	    ap1 = copy_addr (ap1, am_ind);
	    g_code (op_movem, IL4, ap1, ap2);
	    return;
	default:
	    break;
	}
	switch (ap2->mode) {
	case am_ainc:
	    ap2 = copy_addr (ap2, am_ind);
	    g_code (op_movem, IL4, ap1, ap2);
	    g_code (op_add, IL4,
		    mk_immed (4L * count_registers (ap1->u.mask)),
		    mk_reg (ap2->preg));
	    return;
	case am_adec:
	    g_code (op_sub, IL4,
		    mk_immed (4L * count_registers (ap1->u.mask)),
		    mk_reg (ap2->preg));
	    ap2 = copy_addr (ap2, am_ind);
	    g_code (op_movem, IL4, ap1, ap2);
	    return;
	default:
	    break;
	}
    }
#endif /* COLDFIRE */
    g_code (op_movem, IL4, ap1, ap2);
}

#ifdef FLOAT_IEEE
static void g_fmovem P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
#ifdef COLDFIRE
    if (is_coldfire ()) {
	switch (ap1->mode) {
	case am_ainc:
	    ap1 = copy_addr (ap1, am_ind);
	    g_fcode (op_fmovem, IL12, ap1, ap2);
	    g_code (op_add, IL4,
		    mk_immed (12L * count_registers (ap2->u.mask)),
		    mk_reg (ap1->preg));
	    return;
	case am_adec:
	    g_code (op_sub, IL4,
		    mk_immed (12L * count_registers (ap2->u.mask)),
		    mk_reg (ap1->preg));
	    ap1 = copy_addr (ap1, am_ind);
	    g_fcode (op_fmovem, IL12, ap1, ap2);
	    return;
	default:
	    break;
	}
	switch (ap2->mode) {
	case am_ainc:
	    ap2 = copy_addr (ap2, am_ind);
	    g_fcode (op_fmovem, IL12, ap1, ap2);
	    g_code (op_add, IL4,
		    mk_immed (12L * count_registers (ap1->u.mask)),
		    mk_reg (ap2->preg));
	    return;
	case am_adec:
	    g_code (op_sub, IL4,
		    mk_immed (12L * count_registers (ap1->u.mask)),
		    mk_reg (ap2->preg));
	    ap2 = copy_addr (ap2, am_ind);
	    g_fcode (op_fmovem, IL12, ap1, ap2);
	    return;
	default:
	    break;
	}
    }
#endif /* COLDFIRE */
    g_fcode (op_fmovem, IL12, ap1, ap2);
}
#endif /* FLOAT_IEEE */

static void g_call P1 (ADDRESS *, ap)
{
    ADDRESS *ap1;
    EXPR   *ep;
    LABEL   label;

    if (ap == NIL_ADDRESS)
	return;

    switch (ap->mode) {
    case am_immed:
	g_code (op_trap, IL0, ap, NIL_ADDRESS);
	break;
    case am_direct:
	switch (codemodel_option) {
	case model_absolute:
	    g_code (op_jsr, IL0, ap, NIL_ADDRESS);
	    break;
	case model_small:
	    label = nextlabel++;
	    ap = copy_addr (ap, am_indxpc);
	    ep = mk_node (en_sub, ap->u.offset, mk_lcon (label), tp_void);
	    ap->u.offset =
		mk_node (en_add, ep, mk_icon ((IVAL) 2L, tp_int), tp_void);
	    g_label (label);
	    g_code (op_jsr, IL0, ap, NIL_ADDRESS);
	    break;
	case model_large:
	    ap1 = address_register ();
	    label = nextlabel++;
	    ap = copy_addr (ap, am_immed);
	    ep = mk_node (en_sub, ap->u.offset, mk_lcon (label), tp_void);
	    ap->u.offset =
		mk_node (en_add, ep, mk_icon ((IVAL) 2L, tp_int), tp_void);
	    g_move (IL4, ap, ap1);
	    freeop (ap1);
	    ap = copy_addr (ap1, am_indx2pc);
	    ap->u.offset = mk_const ((IVAL) 0L);
	    g_label (label);
	    g_code (op_jsr, IL0, ap, NIL_ADDRESS);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	break;
    case am_ind:
	g_code (op_jsr, IL0, ap, NIL_ADDRESS);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}

static void g_add P3 (ILEN, len, ADDRESS *, ap1, ADDRESS *, ap2)
{
    switch (ap2->mode) {
    case am_areg:
	g_code (op_adda, len, ap1, ap2);
	break;
    default:
	g_code (op_add, len, ap1, ap2);
	break;
    }
}

static void g_sub P3 (ILEN, len, ADDRESS *, ap1, ADDRESS *, ap2)
{
    switch (ap2->mode) {
    case am_areg:
	g_code (op_suba, len, ap1, ap2);
	break;
    default:
	g_code (op_sub, len, ap1, ap2);
	break;
    }
}

static void g_move8 P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
    switch (ap1->mode) {
    case am_mreg:
	if (ap2->mode == am_mreg) {
	    if (ap2->preg == ap1->preg && ap2->sreg == ap1->sreg) {
		return;
	    }
	} else {
	    if (ap1->preg < ap1->sreg) {
		g_movem (mk_smask ((REGMASK)
				   REGBIT (ap1->preg) | REGBIT (ap1->sreg)),
			 ap2);
		return;
	    }
	}
	break;
#ifdef FLOAT_IEEE
    case am_freg:
	g_fcode (op_fmove, IL8, ap1, ap2);
	return;
#endif /* FLOAT_IEEE */
    default:
	break;
    }
    if (ap2->mode == am_mreg && ap1->mode != am_immed) {
	if (ap2->preg < ap2->sreg && ap1->mode != am_mreg) {
	    g_movem (ap1, mk_rmask ((REGMASK)
				    REGBIT (ap2->preg) | REGBIT (ap2->sreg)));
	    return;
	}
    }
    switch (ap2->mode) {
    case am_adec:
	g_move (IL4, mk_high (ap1), mk_high (ap2));
	g_move (IL4, mk_low (ap1), mk_low (ap2));
	break;
#ifdef FLOAT_IEEE
    case am_freg:
	g_fcode (op_fmove, IL8, ap1, ap2);
	break;
#endif /* FLOAT_IEEE */
    default:
#ifdef COLDFIRE
	if (is_coldfire ()) {
	    ADDRESS *ap = data_register ();

	    g_move (IL4, mk_high (ap1), ap);
	    g_move (IL4, ap, mk_high (ap2));
	    g_move (IL4, mk_low (ap1), ap);
	    g_move (IL4, ap, mk_low (ap2));
	    freeop (ap);
	    return;
	}
#endif /* COLDIFRE */
	g_move (IL4, mk_high (ap1), mk_high (ap2));
	g_move (IL4, mk_low (ap1), mk_low (ap2));
	break;
    }
}

static void g_move12 P2 (ADDRESS *, ap1, ADDRESS *, ap2)
{
    switch (ap1->mode) {
    case am_xreg:
	if (ap2->mode == am_xreg) {
	    if (ap2->preg == ap1->preg &&
		ap2->sreg == ap1->sreg && ap2->u.xreg == ap1->u.xreg)
		return;
	} else {
	    if (ap1->preg < ap1->sreg && ap1->sreg < ap1->u.xreg) {
		g_movem (mk_smask ((REGMASK)
				   REGBIT (ap1->preg) |
				   REGBIT (ap1->sreg) |
				   REGBIT (ap1->u.xreg)), ap2);
		return;
	    }
	}
	break;
#ifdef FLOAT_IEEE
    case am_freg:
	g_fcode (op_fmove, IL12, ap1, ap2);
	return;
#endif /* FLOAT_IEEE */
    default:
	break;
    }
    if (ap2->mode == am_xreg) {
	if (ap2->preg < ap2->sreg &&
	    ap2->sreg < ap2->u.xreg && ap1->mode != am_xreg) {
	    g_movem (ap1, mk_rmask ((REGMASK)
				    REGBIT (ap2->preg) |
				    REGBIT (ap2->sreg) |
				    REGBIT (ap2->u.xreg)));
	    return;
	}
    }
    switch (ap2->mode) {
    case am_adec:
	g_move (IL4, mk_top (ap1), mk_top (ap2));
	g_move (IL4, mk_high (ap1), mk_high (ap2));
	g_move (IL4, mk_low (ap1), mk_low (ap2));
	break;
#ifdef FLOAT_IEEE
    case am_freg:
	g_fcode (op_fmove, IL12, ap1, ap2);
	break;
#endif /* FLOAT_IEEE */
    default:
#ifdef COLDFIRE
	if (is_coldfire ()) {
	    ADDRESS *ap = data_register ();

	    g_move (IL4, mk_top (ap1), ap);
	    g_move (IL4, ap, mk_top (ap2));
	    g_move (IL4, mk_high (ap1), ap);
	    g_move (IL4, ap, mk_high (ap2));
	    g_move (IL4, mk_low (ap1), ap);
	    g_move (IL4, ap, mk_low (ap2));
	    freeop (ap);
	    return;
	}
#endif /* COLDFIRE */
	g_move (IL4, mk_top (ap1), mk_top (ap2));
	g_move (IL4, mk_high (ap1), mk_high (ap2));
	g_move (IL4, mk_low (ap1), mk_low (ap2));
	break;
    }
}

/*
 * mk_legal will coerce the addressing mode in ap into a mode that is
 * satisfactory for the flag word.
 */
static ADDRESS *mk_legal P3 (ADDRESS *, ap, FLAGS, flags, const TYP *, tp)
{
    ADDRESS *ap2;
    ILEN    ilen = (ILEN) tp->size;

    if (flags & F_NOVALUE) {
	freeop (ap);
	return NIL_ADDRESS;
    }
    if (ap == NIL_ADDRESS) {
	FATAL ((__FILE__, "mk_legal", "ap = 0"));
    }
#ifdef COLDFIRE
    if ((flags & F_COLD) && (ilen < IL4) && is_coldfire ()) {
	ap = g_extend (ap, tp, is_unsigned_type (tp) ? tp_ulong : tp_long);
    }
#endif /* COLDFIRE */
    switch (ap->mode) {
    case am_immed:
	if (flags & F_IMMED) {
	    return ap;		/* mode ok */
	}
	break;
    case am_areg:
	if (flags & F_AREG
	    && (!(flags & F_VOL) || is_temporary_register (ap->preg))) {
	    return ap;
	}
	break;
    case am_dreg:
	if (flags & F_DREG
	    && (!(flags & F_VOL) || is_temporary_register (ap->preg))) {
	    return ap;
	}
	break;
    case am_mreg:
	if (flags & F_DREG &&
	    (!(flags & F_VOL) || (is_temporary_register (ap->sreg) &&
				  is_temporary_register (ap->preg))) &&
	    (tp->size > 4L)) {
	    return ap;
	}
	break;
    case am_xreg:
	if (flags & F_DREG &&
	    (!(flags & F_VOL) || (is_temporary_register (ap->u.xreg) &&
				  is_temporary_register (ap->sreg) &&
				  is_temporary_register (ap->preg)))) {
	    return ap;
	}
	break;
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_indx3:
    case am_indx4:
    case am_direct:
    case am_ainc:
	if (flags & F_MEM) {
	    return ap;
	}
	break;
    case am_freg:
	if ((flags & F_FREG)
	    && (!(flags & F_VOL) || is_temporary_register (ap->preg))) {
	    return ap;
	}
	break;
    default:
	break;
    }
    if ((flags & (F_DREG | F_AREG)) == (F_DREG | F_AREG)) {
	/* decide, which mode is better */
	if (ap->mode == am_immed) {
	    if (is_byte (ap->u.offset)) {
		flags = (FLAGS) (flags & F_DREG);
	    } else if (is_short (ap->u.offset) && ilen == IL4) {
		flags = (FLAGS) (flags & F_AREG);
	    }
	}
	if (is_free_data () && (flags & F_DREG) && tp->type != bt_pointer32) {
	    freeop (ap);	/* maybe we can use it... */
	    ap2 = data_register ();	/* allocate to dreg */
	    switch (ap->mode) {
#ifdef FLOAT_IEEE
	    case am_freg:
		g_fcode (op_fmove, ilen, ap, ap2);
		break;
#endif /* FLOAT_IEEE */
	    case am_mreg:
		g_move (ilen, mk_high (ap), ap2);
		break;
	    default:
		switch (ilen) {
		case IL8:
		    g_code (op_move, IL4, mk_high (ap), ap2);
		    break;
		default:
		    g_move (ilen, ap, ap2);
		    break;
		}
	    }
	    return ap2;
	}
	if (is_free_addr () && (flags & F_AREG)) {
	    freeop (ap);
	    ap2 = address_register ();
	    g_move (ilen, ap, ap2);
	    return ap2;
	}
    }
    if (flags & F_DREG) {
	freeop (ap);		/* maybe we can use it... */
	switch (ilen) {
	case IL1:
	case IL2:
	case IL4:
	    ap2 = data_register ();	/* allocate to dreg */
	    switch (ap->mode) {
#ifdef FLOAT_IEEE
	    case am_freg:
		g_fcode (op_fmove, ilen, ap, ap2);
		break;
#endif /* FLOAT_IEEE */
	    case am_mreg:
		g_move (ilen, mk_high (ap), ap2);
		break;
	    default:
		g_move (ilen, ap, ap2);
		break;
	    }
	    return ap2;
	case IL8:
	    ap2 = mdata_register ();
	    g_move8 (ap, ap2);
	    return ap2;
	case IL12:
	    ap2 = xdata_register ();
	    g_move12 (ap, ap2);
	    return ap2;
	default:
	    break;
	}
    }
    if (flags & F_AREG) {
	if (ilen < IL2) {
	    FATAL (
		   (__FILE__, "mk_legal",
		    "illegal size %d --> An, mode=%d, flags=0x%x", ilen,
		    ap->mode, (int) flags));
	} else {
	    freeop (ap);
	    ap2 = address_register ();
	    g_move (ilen, ap, ap2);
	    return ap2;
	}
    }
#ifdef FLOAT_IEEE
    if (flags & F_FREG) {
	freeop (ap);		/* maybe we can use it... */
	switch (tp->type) {
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap2 = float_register ();
	    g_fcode (op_fmove, ilen, ap, ap2);
	    return ap2;
	case bt_char:
	case bt_schar:
	case bt_short:
	case bt_int16:
	case bt_long:
	case bt_int32:
	    ap2 = float_register ();
	    g_code (op_fmove, ilen, ap, ap2);
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
	    g_move (ilen, ap, ap2);
	    break;
	case 8L:
	    g_move8 (ap, ap2);
	    break;
	case 12L:
	    g_move8 (ap, ap2);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
	return ap2;
    }
    FATAL ((__FILE__, "mk_legal", ""));
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*
 * If ap is an immediate value between -128 and 127 and the size of the
 * operation is 4 bytes then load the value into a data register if
 * there is one free.
 */
static ADDRESS *mk_quick P2 (ADDRESS *, ap, const TYP *, tp)
{
    if (tp->size == 4L && is_free_data () &&
	ap->mode == am_immed && is_byte (ap->u.offset)) {
	ap = mk_legal (ap, F_DREG, tp);
    }
    return ap;
}

/*
 * If ap is an immediate value between -128 and 127, excluding the values
 * 1 to 8, and the size of the operation is 4 bytes then load the value into
 * a data register if there is one free.
 * This is used for "quick" operations to the "add" and "sub" commands
 * which aren't covered by the addq and subq instructions.
 * Don't "quick"en the value 0 as this is best left for the peephole
 * optimiser as it can then generally remove the instruction.
 */
static ADDRESS *mk_quick2 P2 (ADDRESS *, ap, const TYP *, tp)
{
    if (ap->mode == am_immed && is_icon (ap->u.offset) &&
	(ap->u.offset->v.i < 0L || ap->u.offset->v.i > 8L)) {
	ap = mk_quick (ap, tp);
    }
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
	g_add (IL4, mk_immed ((IVAL) bytes), mk_reg (STACKPTR));
	stack_offset -= bytes;
	if (max_stack_adjust < bytes) {
	    max_stack_adjust = bytes;
	}
    }
}

/*
 * Generate an instruction which takes an immediate option with optimal
 * (for space) instruction(s).
 */
static void g_immed P4 (OPCODE, op, const TYP *, tp, IVAL, i, ADDRESS *, ap)
{
    ADDRESS *ap1 = mk_immed (i);

    ap1 = mk_quick (ap1, tp);
    g_code (op, (ILEN) tp->size, ap1, ap);
    freeop (ap1);
}

static void g_immed2 P4 (OPCODE, op, const TYP *, tp, IVAL, i, ADDRESS *, ap)
{
    ADDRESS *ap1 = mk_immed (i);

    ap1 = mk_quick2 (ap1, tp);
    g_code (op, (ILEN) tp->size, ap1, ap);
    freeop (ap1);
}

static ADDRESS *g_extend P3 (ADDRESS *, ap, const TYP *, tp1, const TYP *, tp2)
{
    if (ap->mode == am_immed) {
	return ap;
    }
    switch (tp2->type) {
    case bt_int16:
    case bt_uint16:
    case bt_short:
    case bt_ushort:
	switch (tp1->type) {
	case bt_char:
	case bt_schar:
	    ap = mk_legal (ap, F_DREG, tp1);
	    g_code (op_ext, IL2, ap, NIL_ADDRESS);
	    break;
	case bt_bool:
	case bt_uchar:
	case bt_charu:
	    g_code (op_and, IL2, mk_immed ((IVAL) 255L), ap);
	    break;
	default:
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
	    ap = mk_legal (ap, F_DREG, tp1);
	    if (target_option >= target_coldfire) {
		g_code (op_extb, IL4, ap, NIL_ADDRESS);
	    } else {
		g_code (op_ext, IL2, ap, NIL_ADDRESS);
		g_code (op_ext, IL4, ap, NIL_ADDRESS);
	    }
	    break;
	case bt_int16:
	case bt_short:
	    ap = mk_legal (ap, F_DREG, tp1);
	    g_code (op_ext, IL4, ap, NIL_ADDRESS);
	    break;
	case bt_bool:
	case bt_uchar:
	case bt_charu:
	    g_code (op_and, IL4, mk_immed ((IVAL) 255L), ap);
	    break;
	case bt_uint16:
	case bt_ushort:
	    g_code (op_and, IL4, mk_immed ((IVAL) 65535L), ap);
	    break;
	default:
	    break;
	}
	break;
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	switch (tp1->type) {
	    ADDRESS *ap1;

	case bt_char:
	case bt_schar:
	    ap = mk_legal (ap, F_DREG | F_AREG | F_MEM, tp1);
	    freeop (ap);
	    ap1 = mdata_register ();
	    g_code (op_move, IL1, ap, mk_high (ap1));
	    if (target_option >= target_coldfire) {
		g_code (op_extb, IL4, mk_high (ap1), NIL_ADDRESS);
	    } else {
		g_code (op_ext, IL2, mk_high (ap1), NIL_ADDRESS);
		g_code (op_ext, IL4, mk_high (ap1), NIL_ADDRESS);
	    }
	    g_code (op_slt, IL0, mk_low (ap1), NIL_ADDRESS);
	    VOIDCAST g_extend (mk_low (ap1), tp_schar, tp_long);

	    ap = ap1;
	    break;
	case bt_bool:
	case bt_uchar:
	case bt_charu:
	    ap = mk_legal (ap, F_DREG | F_AREG | F_MEM, tp1);
	    freeop (ap);
	    ap1 = mdata_register ();
	    g_code (op_move, IL1, ap, mk_high (ap1));
	    g_code (op_and, IL4, mk_immed ((IVAL) 255L), mk_high (ap1));
	    g_code (op_clr, IL4, mk_low (ap1), NIL_ADDRESS);
	    ap = ap1;
	    break;
	case bt_int16:
	case bt_short:
	    ap = mk_legal (ap, F_DREG | F_AREG | F_MEM, tp1);
	    freeop (ap);
	    ap1 = mdata_register ();
	    g_code (op_move, IL2, ap, mk_high (ap1));
	    g_code (op_ext, IL4, mk_high (ap1), NIL_ADDRESS);
	    g_code (op_slt, IL0, mk_low (ap1), NIL_ADDRESS);
	    VOIDCAST g_extend (mk_low (ap1), tp_schar, tp_long);

	    ap = ap1;
	    break;
	case bt_uint16:
	case bt_ushort:
	    ap = mk_legal (ap, F_DREG | F_AREG | F_MEM, tp1);
	    freeop (ap);
	    ap1 = mdata_register ();
	    g_code (op_move, IL2, ap, mk_high (ap1));
	    g_code (op_and, IL4, mk_immed ((IVAL) 65535L), mk_high (ap));
	    g_code (op_clr, IL4, mk_low (ap1), NIL_ADDRESS);
	    ap = ap1;
	    break;
	case bt_int32:
	case bt_long:
	    ap = mk_legal (ap, F_DREG | F_AREG | F_MEM, tp1);
	    freeop (ap);
	    ap1 = mdata_register ();
	    g_code (op_move, IL4, ap, mk_high (ap1));
	    g_code (op_slt, IL0, mk_low (ap1), NIL_ADDRESS);
	    VOIDCAST g_extend (mk_low (ap1), tp_schar, tp_long);

	    ap = ap1;
	    break;
	case bt_uint32:
	case bt_ulong:
	    ap = mk_legal (ap, F_DREG, tp1);
	    freeop (ap);
	    ap1 = mdata_register ();
	    g_code (op_move, IL4, ap, mk_high (ap1));
	    g_code (op_clr, IL4, mk_low (ap1), NIL_ADDRESS);
	    ap = ap1;
	    break;
	default:
	    break;
	}
	break;
#endif /* LONGLONG_SUPPORT */
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return ap;
}

static ADDRESS *mk_low P1 (ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_immed:
	if (is_icon (ap->u.offset)) {
	    EXPR   *ep = ap->u.offset;

	    if (is_signed_type (ep->etp)) {
		IVAL    i = ep->v.i;

#ifdef LONGLONG
		i >>= 32;
#else
		if (ep->v.i < 0) {
		    i = (IVAL) -1;
		} else {
		    i = 0;
		}
#endif /* LONGLONG */
		return mk_immed (i);
	    } else {
		UVAL    u = ep->v.u;

#ifdef LONGLONG
		u >>= 32;
#else
		u = 0;
#endif /* LONGLONG */
		return mk_immed ((IVAL) u);
	    }
	}
	break;
    case am_ainc:
    case am_adec:
    case am_freg:
	return ap;
    case am_mreg:
    case am_xreg:
	return mk_reg (ap->preg);
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_indx3:
    case am_indx4:
    case am_direct:
	return mk_offset (ap, 0L);
    default:
	FATAL ((__FILE__, "mk_low", "illegal mode %d", ap->mode));
	break;
    }
    return NIL_ADDRESS;
}

static ADDRESS *mk_high P1 (ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_immed:
	if (is_icon (ap->u.offset)) {
#ifdef LONGLONG
	    return mk_immed ((IVAL) (ap->u.offset->v.u & 0xFFFFFFFFUL));
#else
	    return mk_immed (ap->u.offset->v.i);
#endif
	}
	break;
    case am_dreg:
    case am_areg:
    case am_ainc:
    case am_adec:
	return ap;
    case am_mreg:
    case am_xreg:
	return mk_reg (ap->sreg);
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_indx3:
    case am_indx4:
    case am_direct:
	return mk_offset (ap, 4L);
    default:
	FATAL ((__FILE__, "mk_high", "illegal mode %d", ap->mode));
	break;
    }
    return NIL_ADDRESS;
}

static ADDRESS *mk_top P1 (ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_ainc:
    case am_adec:
	return ap;
    case am_xreg:
	return mk_reg (ap->u.xreg);
    case am_ind:
    case am_indx:
    case am_indx2:
    case am_indx3:
    case am_indx4:
    case am_direct:
	return mk_offset (ap, 8L);
    default:
	FATAL ((__FILE__, "mk_top", "illegal mode %d", ap->mode));
	break;
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*
 * return ap, if ap can be switched to address a location with a short
 * offset, otherwise return 0.	  Typical application:
 * cast long -> short: 8(a6) --> 10(a6) offset is a small number (1,2 or 3)
 */
static ADDRESS *mk_offset P2 (ADDRESS *, ap, SIZE, off)
{
    switch (ap->mode) {
    case am_ind:
	if (off) {
	    ap = copy_addr (ap, am_indx);
	    ap->u.offset = mk_const ((IVAL) off);
	}
	return ap;
    case am_indx:
	if (is_icon (ap->u.offset)
	    && ((IVAL) off + ap->u.offset->v.i <= (IVAL) 32767L)) {
	    ap = copy_addr (ap, ap->mode);
	    ap->u.offset = mk_const (ap->u.offset->v.i + (IVAL) off);
	    return ap;
	}
	break;
    case am_indx2:
    case am_indx3:
    case am_indx4:
	if (is_icon (ap->u.offset) &&
	    ((IVAL) off + ap->u.offset->v.i <= (IVAL) 127L
	     || target_option < target_68020)) {
	    ap = copy_addr (ap, ap->mode);
	    ap->u.offset = mk_const (ap->u.offset->v.i + (IVAL) off);
	    return ap;
	}
	break;
    case am_direct:
	ap = copy_addr (ap, ap->mode);
	ap->u.offset = mk_add (ap->u.offset, mk_const ((IVAL) off));
	return ap;
    default:
	break;
    }
    /* special value indicating that it must be done by hand */
    return NIL_ADDRESS;
}

/*
 * generate code to evaluate an index node and return the addressing
 * mode of the result.
 */
static ADDRESS *g_index P1 (const EXPR *, ep)
{
    ADDRESS *ap1, *ap2;
    EXPR   *ep0 = ep->v.p[0];
    EXPR   *ep1 = ep->v.p[1];

    /*
     *      Try and ensure that we evaluate address registers first ...
     *      this leads to better code
     */
    if (ep1->nodetype == en_register && is_address_register (ep1->v.r)) {
	ep0 = ep->v.p[1];
	ep1 = ep->v.p[0];
    }
    if (ep1->nodetype == en_register && ep0->nodetype == en_register) {
	if (is_address_register (ep0->v.r)) {
	    /* first node is address register */
	    ap1 = g_expr (ep0, F_AREG);
	    ap1 = copy_addr (ap1, am_none);
	    ap2 = g_expr (ep1, (FLAGS) (F_AREG | F_DREG));
	    if (ap2->mode == am_dreg) {
		/* 0(Ax,Dx) */
		ap1->mode = am_indx2;
	    } else {
		/* 0(Ax,Ay) */
		ap1->mode = am_indx3;
	    }
	    ap1->sreg = ap2->preg;
	    ap1->u.offset = mk_const ((IVAL) 0L);
	    return ap1;
	} else if (is_address_register (ep1->v.r)) {
	    /* second node is address register */
	    ap1 = g_expr (ep1, F_AREG);
	    ap1 = copy_addr (ap1, am_indx2);	/* 0(Ax,Dx) */
	    ap1->sreg = ep0->v.r;
	    ap1->u.offset = mk_const ((IVAL) 0L);
	    return ap1;
	}
    }
    /*
     *               The general case (no register)
     */

    ap1 = g_expr (ep0, (FLAGS) (F_AREG | F_IMMED));
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
    /*
     *               possible combinations:
     *
     *                               F_AREG  + F_AREG
     *                               F_AREG  + F_DREG
     *                               F_AREG  + F_IMMED
     *                               F_IMMED + F_IMMED
     *                               F_IMMED + F_AREG
     */

    if (ap1->mode == am_areg) {
	/*
	 *       watch out for:
	 *                       register(addr) + address_register
	 *                       register(addr) + data_register
	 */
	if (!is_temporary_register (ap1->preg)) {
	    /* ap1 = register variable address register */
	    ap1 = copy_addr (ap1, ap1->mode);
	    switch (ap2->mode) {
	    case am_dreg:
		/* 0(Ax,Dy) */
		ap1->mode = am_indx2;
		ap1->sreg = ap2->preg;
		ap1->deep = ap2->deep;
		ap1->u.offset = mk_const ((IVAL) 0L);
		return ap1;
	    case am_areg:
		/* 0(Ax,Ay) */
		ap1->mode = am_indx3;
		ap1->sreg = ap2->preg;
		ap1->deep = ap2->deep;
		ap1->u.offset = mk_const ((IVAL) 0L);
		return ap1;
	    case am_immed:
		if (!is_short (ap2->u.offset)) {
		    /* we want to add to ap1 later... */
		    ap1 =
			mk_legal (ap1, (FLAGS) (F_AREG | F_VOL), tp_pointer);
		}
		break;
	    default:
		break;
	    }
	}
	/*
	 *       watch out for:
	 *                       address_register + register(data)
	 */
	if (ap2->mode == am_dreg && !is_temporary_data_register (ap2->preg)) {
	    ap1 = copy_addr (ap1, am_indx2);
	    ap1->sreg = ap2->preg;
	    ap1->u.offset = mk_const ((IVAL) 0L);
	    return ap1;
	}
    }
    if (ap2->mode == am_immed) {
	if (ap1->mode == am_immed) {
	    ap1 = copy_addr (ap1, am_direct);
	    ap1->u.offset = mk_add (ap1->u.offset, ap2->u.offset);
	    return ap1;
	}
	if (is_short (ap2->u.offset)) {
	    ap1 = mk_legal (ap1, F_AREG, tp_pointer);
	    ap1 = copy_addr (ap1, am_indx);
	    ap1->u.offset = ap2->u.offset;
	    return ap1;
	}
    }
    freeop (ap2);		/* release any temps in ap2 */
    if (!is_temporary_register (ap1->preg)) {
	/* ap1 is not volatile ... */
	ap1 = mk_legal (ap1, (FLAGS) (F_AREG | F_VOL), tp_pointer);
    }
    g_add (IL4, ap2, ap1);	/* add left to address reg */
    ap1 = copy_addr (ap1, am_ind);
    return ap1;			/* return indirect */
}

/*
 * return the addressing mode of a dereferenced node.
 */
static ADDRESS *g_deref P3 (const EXPR *, ep, const TYP *, tp, FLAGS, flags)
{
    ADDRESS *ap1;
    SIZE    size = tp->size;
    EXPR   *ep2;

    /*
     * If a reference to a struct/union is required, return a
     * pointer to the struct instead
     */
    if (is_structure_type (tp)
#ifdef FLOAT_SUPPORT
	|| is_complex_type (tp)
#endif /* FLOAT_SUPPORT */
	|| is_array_assignment (tp)) {
	return g_expr (ep, F_ALL);
    }
    switch (ep->nodetype) {
    case en_add:
	return g_index (ep);
    case en_global:
	ep2 = mk_global (SUP_DATA, ep);
	switch (datamodel_option) {
	case model_small:
	    ap1 = mk_indirect (regdata, ep2);
	    break;
	case model_large:
	    ap1 = address_register ();
	    g_move (IL4, mk_expr (am_immed, ep2), ap1);
	    ap1 = copy_addr (ap1, am_indx3);
	    ap1->sreg = regdata;
	    ap1->u.offset = mk_const ((IVAL) 0L);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	return ap1;
    case en_autocon:
	if (ep->v.i >= -32768L && ep->v.i <= 32767L) {
	    ap1 = mk_indirect (regframe, mk_const (ep->v.i));
	} else {
	    ap1 = address_register ();
	    g_move (IL4, mk_immed (ep->v.i), ap1);
	    g_add (IL4, mk_reg (regframe), ap1);
	    ap1 = copy_addr (ap1, am_ind);
	}
	return ap1;
    case en_ainc:
	/* special 68000 instructions */
	if ((size == 1L || size == 2L || size == 4L)
	    && ep->v.p[1]->v.i == (IVAL) size
	    && ep->v.p[0]->nodetype == en_register
	    && is_address_register (ep->v.p[0]->v.r)
	    && !(flags & F_USES)) {
	    /* (An)+ */
	    ap1 = mk_amode (am_ainc);
	    ap1->preg = ep->v.p[0]->v.r;
	    return ap1;
	}
	/*lint -fallthrough */
    default:
	ap1 = g_expr (ep, (FLAGS) (F_AREG | F_IMMED));	/* generate address */
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
    TYP    *tp = ep->etp;

    ap = g_deref (ep->v.p[0], tp, F_ALL);
    ap = mk_legal (ap, (FLAGS) (F_DREG | F_VOL), tp);
    g_rotate (ap, tp, (int) ep->v.bit.offset, tp, (int) ep->v.bit.width);
    return mk_legal (ap, flags, tp);
}

/*============================================================================*/
#ifdef FLOAT_SUPPORT
static void push_rtl_params P2 (const EXPR *, ep1, const EXPR *, ep2)
{
#ifdef FLOAT_IEEE
    ADDRESS *ap;

    is_parameter++;
    temp_inv (NIL_REGUSAGE);
    ap = g_expr (ep1, F_MEM);
    g_code (op_pea, IL0, ap, NIL_ADDRESS);
    freeop (ap);
    if (ep2) {
	ap = g_expr (ep2, F_MEM);
	g_code (op_pea, IL0, ap, NIL_ADDRESS);
	freeop (ap);
    }
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
    is_parameter++;
    temp_inv (NIL_REGUSAGE);
    VOIDCAST push_param (ep1);

    if (ep2) {
	VOIDCAST push_param (ep2);
    }
#endif /* FLOAT_MFFP */
    is_parameter--;
}
#endif /* FLOAT_SUPPORT */

/*============================================================================*/

/*
 * generate code to evaluate a unary minus or complement. float: unary minus
 * calls a library function
 */
static ADDRESS *g_unary P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap;
    TYP    *tp = ep->etp;

    switch (tp->type) {
    case bt_uchar:
    case bt_schar:
    case bt_char:
    case bt_charu:
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
	g_code (op, (ILEN) tp->size, ap, NIL_ADDRESS);
	return mk_legal (ap, flags, tp);
#ifdef LONGLONG_SUPPORT
    case bt_ulonglong:
    case bt_longlong:
	ap = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
	g_code (op, IL4, mk_high (ap), NIL_ADDRESS);
	switch (op) {
	case op_neg:
	    g_code (op_negx, IL4, mk_low (ap), NIL_ADDRESS);
	    break;
	case op_not:
	    g_code (op, IL4, mk_low (ap), NIL_ADDRESS);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	return mk_legal (ap, flags, tp);
#endif /* LONGLONG_SUPPORT */
    default:
	FATAL (
	       (__FILE__, "g_unary", "illegal type %d or operation %d",
		tp->type, op));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*============================================================================*/

/*
 * generate an auto increment or decrement node. op should be either op_add
 * (for increment) or op_sub (for decrement).
 */
static ADDRESS *g_aincdec P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2, *ap3;
    TYP    *tp = ep->etp;
    FLAGS   flagx;

    switch (tp->type) {
    case bt_bool:
	ap1 = g_expr (ep->v.p[0], F_ALL);
	switch (op) {
	case op_add:
	    g_move ((ILEN) tp->size, mk_immed ((IVAL) 1L), ap1);
	    break;
	default:
	    g_move ((ILEN) tp->size, mk_immed ((IVAL) 0L), ap1);
	    break;
	}
	return mk_legal (ap1, flags, tp);
    case bt_uchar:
    case bt_schar:
    case bt_char:
    case bt_charu:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
#ifdef COLDFIRE
	if (is_coldfire ()) {
	    if (ep->v.p[0]->nodetype == en_fieldref) {
		return g_asbitfield (ep, flags, op, TRUE);
	    }
	    ap1 = temp_reg (flags);
	    ap2 = g_expr (ep->v.p[0], F_ALL);
	    validate (ap1);
	    g_move ((ILEN) tp->size, ap2, ap1);
	    ap1 =
		g_extend (ap1, tp, is_signed_type (tp) ? tp_long : tp_ulong);
	    ap3 = g_expr (ep->v.p[1], F_IMMED);
	    validate (ap1);
	    switch (op) {
	    case op_add:
		g_add (IL4, ap3, ap1);
		break;
	    default:
		g_sub (IL4, ap3, ap1);
		break;
	    }
	    g_move ((ILEN) tp->size, ap1, ap2);
	    if (!(flags & F_NOVALUE)) {
		switch (op) {
		case op_add:
		    g_sub (IL4, ap3, ap1);
		    break;
		default:
		    g_add (IL4, ap3, ap1);
		    break;
		}
	    }
	    freeop (ap3);
	    freeop (ap2);
	    return mk_legal (ap1, flags, tp);
	}
	/*lint -fallthrough */
#endif /* COLDFIRE */
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, TRUE);
	}
	flagx = is_coldfire ()? F_DREG : F_IMMED;
	if (flags & F_NOVALUE) {	/* don''t need result */
	    ap1 = g_expr (ep->v.p[0], F_ALL);
	    ap2 = g_expr (ep->v.p[1], flagx);
	    switch (op) {
	    case op_add:
		g_add ((ILEN) tp->size, ap2, ap1);
		break;
	    default:
		g_sub ((ILEN) tp->size, ap2, ap1);
		break;
	    }
	} else {
	    ap1 = temp_reg (flags);
	    ap2 = g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_USES));
	    validate (ap1);
	    g_move ((ILEN) tp->size, ap2, ap1);
	    ap3 = g_expr (ep->v.p[1], flagx);
	    validate (ap2);
	    switch (op) {
	    case op_add:
		g_add ((ILEN) tp->size, ap3, ap2);
		break;
	    default:
		g_sub ((ILEN) tp->size, ap3, ap2);
		break;
	    }
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	flagx = F_DREG | F_IMMED;
	if (flags & F_NOVALUE) {	/* don''t need result */
	    ADDRESS *ap4;

	    ap1 = g_expr (ep->v.p[0], F_ALL);
	    ap2 = g_expr (ep->v.p[1], flagx);
	    validate (ap1);
	    ap3 = data_register ();
	    g_code (op_move, IL4, mk_low (ap2), ap3);
	    ap4 = data_register ();
	    g_code (op_move, IL4, mk_low (ap1), ap4);
	    switch (op) {
	    case op_add:
		g_add (IL4, mk_high (ap2), mk_high (ap1));
		g_code (op_addx, IL4, ap3, ap4);
		break;
	    default:
		g_sub (IL4, mk_high (ap2), mk_high (ap1));
		g_code (op_subx, IL4, ap3, ap4);
		break;
	    }
	    g_code (op_move, IL4, ap4, mk_low (ap1));
	    freeop (ap4);
	    freeop (ap3);
	    freeop (ap2);
	} else {
	    ADDRESS *ap4;

	    ap1 = g_expr (ep->v.p[0], F_ALL);
	    g_move8 (ap1, &push);
	    ap2 = g_expr (ep->v.p[1], flagx);
	    validate (ap1);
	    ap3 = data_register ();
	    g_code (op_move, IL4, mk_low (ap2), ap3);
	    ap4 = data_register ();
	    g_code (op_move, IL4, mk_low (ap1), ap4);
	    switch (op) {
	    case op_add:
		g_add (IL4, mk_high (ap2), mk_high (ap1));
		g_code (op_addx, IL4, ap3, ap4);
		break;
	    default:
		g_sub (IL4, mk_high (ap2), mk_high (ap1));
		g_code (op_subx, IL4, ap3, ap4);
		break;
	    }
	    g_code (op_move, IL4, ap4, mk_low (ap1));
	    freeop (ap4);
	    freeop (ap3);
	    freeop (ap2);
	    freeop (ap1);
	    ap1 = mdata_register ();
	    g_move8 (&pop, ap1);
	}
	validate (ap1);
	return mk_legal (ap1, flags, tp);
#endif /* LONGLONG_SUPPORT */
    default:
	FATAL ((__FILE__, "g_aincdec", "illegal type %d or float", tp->type));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*============================================================================*/

/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */
static ADDRESS *g_addsub P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;
    TYP    *tp = ep->etp;

    switch (tp->type) {
    case bt_uchar:
    case bt_schar:
    case bt_char:
    case bt_charu:
	ap1 =
	    g_expr (ep->v.p[0], (FLAGS) (F_VOL | F_COLD | (flags & F_DREG)));
	ap2 = g_expr (ep->v.p[1], (FLAGS) ((F_ALL & ~F_AREG) | F_COLD));
	validate (ap1);		/* in case push occurred */
	switch (op) {
	case op_add:
	    g_add ((ILEN) tp->size, ap2, ap1);
	    break;
	default:
	    g_sub ((ILEN) tp->size, ap2, ap1);
	    break;
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap1 =
	    g_expr (ep->v.p[0],
		    (FLAGS) (F_VOL | F_COLD | (flags & (F_DREG | F_AREG))));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_ALL | F_COLD));
	validate (ap1);		/* in case push occurred */
	switch (op) {
	case op_add:
	    g_add ((ILEN) tp->size, ap2, ap1);
	    break;
	default:
	    g_sub ((ILEN) tp->size, ap2, ap1);
	    break;
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_VOL | F_COLD | F_DREG));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_IMMED | F_MEM | F_COLD));
	validate (ap1);
	{
	    ADDRESS *ap3 = data_register ();

	    g_code (op_move, IL4, mk_low (ap2), ap3);
	    validate (ap1);	/* in case push occurred */
	    switch (op) {
	    case op_add:
		g_code (op_add, IL4, mk_high (ap2), mk_high (ap1));
		g_code (op_addx, IL4, ap3, mk_low (ap1));
		break;
	    default:
		g_code (op_sub, IL4, mk_high (ap2), mk_high (ap1));
		g_code (op_subx, IL4, ap3, mk_low (ap1));
		break;
	    }
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#endif /* LONGLONG_SUPPORT */

#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	op = (op == op_add) ? op_fadd : op_fsub;
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_VOL | F_FREG));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_FREG | F_MEM));
	validate (ap1);		/* in case push occurred */
	g_fcode (op, (ILEN) tp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_addsub", "illegal type %d", tp->type));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*
 * generate a plus equal or a minus equal node.
 */
static ADDRESS *g_asadd P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    FLAGS   flagx, flagx2;
    ADDRESS *ap1, *ap2;
    TYP    *tp = ep->etp;

    switch (tp->type) {
    case bt_bool:
	ap1 = g_expr (ep->v.p[0], F_ALL);
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_IMMED | F_DREG));
	validate (ap1);
	switch (ap2->mode) {
	case am_immed:
	    switch (op) {
	    case op_add:
		ap2->u.offset->v.i = (IVAL) (ap2->u.offset->v.i != 0L);
		break;
	    default:
		ap2->u.offset->v.i = (IVAL) (ap2->u.offset->v.i == 0L);
		break;
	    }
	    break;
	default:
	    break;
	}
	g_code (op_or, (ILEN) tp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    case bt_char:
    case bt_charu:
    case bt_schar:
    case bt_uchar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
#ifdef COLDFIRE
	if (is_coldfire ()) {
	    ADDRESS *ap3;

	    if (ep->v.p[0]->nodetype == en_fieldref) {
		return g_asbitfield (ep, flags, op, TRUE);
	    }
	    ap1 = temp_reg (flags);
	    ap2 = g_expr (ep->v.p[0], F_ALL);
	    validate (ap1);
	    g_move ((ILEN) tp->size, ap2, ap1);
	    ap1 =
		g_extend (ap1, tp, is_signed_type (tp) ? tp_long : tp_ulong);
	    ap3 = g_expr (ep->v.p[1], (FLAGS) (F_IMMED | F_DREG));
	    validate (ap1);
	    switch (op) {
	    case op_add:
		g_add (IL4, ap3, ap1);
		break;
	    default:
		g_sub (IL4, ap3, ap1);
		break;
	    }
	    g_move ((ILEN) tp->size, ap1, ap2);
	    freeop (ap3);
	    freeop (ap2);
	    return mk_legal (ap1, flags, tp);
	}
	/*lint -fallthrough */
#endif /* COLDFIRE */
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}
	flagx = (flags & F_NOVALUE) ? F_ALL : (FLAGS) (F_ALL | F_USES);
#ifdef COLDFIRE
	flagx2 = F_DREG;
	if (!is_coldfire ()) {
	    flagx2 |= F_IMMED;
	}
#else
	flagx2 = F_DREG | F_IMMED;
#endif /* COLDFIRE */
	ap1 = g_expr (ep->v.p[0], flagx);
	ap2 = g_expr (ep->v.p[1], flagx2);
	ap2 = mk_quick2 (ap2, tp);
	validate (ap1);
	switch (op) {
	case op_add:
	    g_add ((ILEN) tp->size, ap2, ap1);
	    break;
	default:
	    g_sub ((ILEN) tp->size, ap2, ap1);
	    break;
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	flagx =
	    (flags & F_NOVALUE) ? F_ALL : (FLAGS) (F_DREG | F_MEM | F_USES);
#ifdef COLDFIRE
	flagx2 = F_DREG;
	if (!is_coldfire ()) {
	    flagx2 |= F_IMMED;
	}
#else
	flagx2 = F_DREG | F_IMMED;
#endif /* COLDFIRE */
	ap1 = g_expr (ep->v.p[0], flagx);
	ap2 = g_expr (ep->v.p[1], flagx2);
	validate (ap1);
	switch (ap1->mode) {
	    ADDRESS *ap3;

	case am_ind:
	case am_indx:
	case am_indx2:
	case am_indx3:
	case am_indx4:
	case am_direct:
	    switch (ap2->mode) {
		ADDRESS *ap4;

	    case am_mreg:
		ap4 = data_register ();
		g_code (op_move, IL4, mk_low (ap1), ap4);
		g_code (op, IL4, mk_high (ap2), mk_high (ap1));
		switch (op) {
		case op_add:
		    g_code (op_addx, IL4, mk_low (ap2), ap4);
		    break;
		default:
		    g_code (op_subx, IL4, mk_low (ap2), ap4);
		    break;
		}
		g_code (op_move, IL4, ap4, mk_low (ap1));
		freeop (ap4);
		break;
	    default:
		ap3 = data_register ();
		validate (ap2);
		g_code (op_move, IL4, mk_low (ap2), ap3);
		validate (ap1);
		ap4 = data_register ();
		g_code (op_move, IL4, mk_low (ap1), ap4);
		switch (op) {
		case op_add:
		    g_code (op_add, IL4, mk_high (ap2), mk_high (ap1));
		    g_code (op_addx, IL4, ap3, ap4);
		    break;
		default:
		    g_code (op_sub, IL4, mk_high (ap2), mk_high (ap1));
		    g_code (op_subx, IL4, ap3, ap4);
		    break;
		}
		g_code (op_move, IL4, ap4, mk_low (ap1));
		freeop (ap4);
		freeop (ap3);
		break;
	    }
	    break;
	case am_mreg:
	    ap3 = data_register ();
	    g_code (op_move, IL4, mk_low (ap2), ap3);
	    validate (ap1);
	    switch (op) {
	    case op_add:
		g_code (op_add, IL4, mk_high (ap2), mk_high (ap1));
		g_code (op_addx, IL4, ap3, mk_low (ap1));
		break;
	    default:
		g_code (op_sub, IL4, mk_high (ap2), mk_high (ap1));
		g_code (op_subx, IL4, ap3, mk_low (ap1));
		break;
	    }
	    freeop (ap3);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	freeop (ap2);
	validate (ap1);
	return mk_legal (ap1, flags, tp);
#endif /* LONGLONG_SUPPORT */

#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	op = (op == op_add) ? op_fadd : op_fsub;
	flagx = (flags & F_NOVALUE) ? F_ALL : (FLAGS) (F_ALL | F_USES);
	ap1 = g_expr (ep->v.p[0], flagx);
	ap2 = g_expr (ep->v.p[1], F_FREG);
	validate (ap1);
	g_fcode (op, (ILEN) tp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "asadd", "illegal type %d", tp->type));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*============================================================================*/

/*
 * generate code to evaluate a restricted binary node and return the
 * addressing mode of the result.
 */
static ADDRESS *g_logic P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2, *ap3;
    ILEN    ilen = (ILEN) ep->etp->size;
    FLAGS   flagx;

#ifdef COLDFIRE
    if (is_coldfire ()) {
	ilen = IL4;
    }
#endif /* COLDFIRE */
    switch (ep->etp->size) {
    case IL8:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_VOL | D_REG | F_COLD));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_MEM | F_IMMED | F_COLD));
	ap3 = data_register ();
	validate (ap1);		/* in case push occurred */
	g_code (op_move, IL4, mk_high (ap2), ap3);
	g_code (op, IL4, ap3, mk_high (ap1));
	g_code (op_move, IL4, mk_low (ap2), ap3);
	g_code (op, IL4, ap3, mk_low (ap1));
	freeop (ap3);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    default:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_VOL | F_DREG | F_COLD));
	if (op == op_eor) {
	    flagx = (FLAGS) (F_DREG | F_IMMED | F_COLD);
	} else {
	    flagx = (FLAGS) ((F_ALL | F_COLD) & ~F_AREG);
	}
	if (ap1->mode == am_mreg)
	    flagx = (FLAGS) (F_MEM | F_IMMED | F_COLD);
	ap2 = g_expr (ep->v.p[1], flagx);
	ap2 = mk_quick (ap2, ep->etp);
	validate (ap1);		/* in case push occurred */
	g_code (op, ilen, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, ep->etp);
    }
}

/*
 * generate a &= or a |= ep.
 */
static ADDRESS *g_aslogic P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    FLAGS   flagx;
    ADDRESS *ap1, *ap2;
    TYP    *tp = ep->etp;
    EXPR   *ep0 = ep->v.p[0];

    if (ep0->nodetype == en_fieldref) {
	UVAL    mask = (UVAL) bitmask (ep0->v.bit.width);
	EXPR   *ep2;

	ap1 = g_expr (ep->v.p[1], (FLAGS) (F_IMMED | F_DREG | F_VOL));
	if (ap1->mode == am_immed) {
	    if (op == op_or)
		ap1->u.offset->v.u &= mask;
	    else
		ap1->u.offset->v.u |= ~mask;
	    ap1->u.offset->v.u <<= (int) ep0->v.bit.offset;
	} else {
	    if (op == op_or)
		g_immed (op_and, tp, (IVAL) mask, ap1);
	    else
		g_immed (op_or, tp, (IVAL) ~mask, ap1);
	    g_rotate (ap1, ep->etp, -(int) ep0->v.bit.offset, tp_void, 0);
	}
	ep2 = mk_ref (ep0->v.bit.p, tp_pointer);
	ap2 = g_expr (ep2, F_MEM);
	validate (ap1);
	g_code (op, (ILEN) tp->size, ap1, ap2);
	freeop (ap2);
	if (!(flags & F_NOVALUE)) {
	    freeop (ap1);
	    ap1 = data_register ();
	    g_code (op_move, (ILEN) tp->size, ap2, ap1);
	    g_rotate (ap1, tp, (int) ep0->v.bit.offset, tp,
		      (int) ep0->v.bit.width);
	}
	return mk_legal (ap1, flags, tp);
    }
    flagx = (flags & F_NOVALUE) ? F_ALL : (FLAGS) (F_ALL | F_USES);
    ap1 = g_expr (ep0, flagx);
    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED | F_COLD));
    ap2 = mk_quick (ap2, ep->etp);
    validate (ap1);
    switch (tp->size) {
    case IL8:
	g_code (op, IL4, mk_high (ap2), mk_high (ap1));
	g_code (op, IL4, mk_low (ap2), mk_low (ap1));
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    default:
	if (ap1->mode == am_areg || is_coldfire ()) {
	    ADDRESS *ap3 = data_register ();

	    g_move ((ILEN) tp->size, ap1, ap3);
	    ap3 = mk_legal (ap3, (FLAGS) (F_DREG | F_COLD), tp);
	    g_code (op, (ILEN) tp->size, ap2, ap3);
	    g_move ((ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	} else {
	    g_code (op, (ILEN) tp->size, ap2, ap1);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    }
}

/*============================================================================*/

static ADDRESS *g_shft P4 (OPCODE, op, ILEN, ilen, ADDRESS *, ap1, ADDRESS *,
			   ap2)
{
    LABEL   label;

    switch (ilen) {
    case IL1:
    case IL2:
    case IL4:
	/* quick constant only legal if 1<=const<=8 */
	if (ap1->mode == am_immed && is_icon (ap1->u.offset)) {
	    IVAL    i = ap1->u.offset->v.i;

	    if (i == 16L && ilen == IL4) {
		switch (op) {
		case op_asl:
		    g_code (op_swap, IL0, ap2, NIL_ADDRESS);
		    g_code (op_clr, IL2, ap2, NIL_ADDRESS);
		    break;
		case op_asr:
		    g_code (op_swap, IL0, ap2, NIL_ADDRESS);
		    g_code (op_ext, IL4, ap2, NIL_ADDRESS);
		    break;
		case op_lsr:
		    g_code (op_clr, IL2, ap2, NIL_ADDRESS);
		    g_code (op_swap, IL0, ap2, NIL_ADDRESS);
		    break;
		default:
		    CANNOT_REACH_HERE ();
		}
		return ap1;
	    }
	    if ((i > 8L) || (i < 1L)) {
		ap1 = mk_legal (ap1, F_DREG, tp_char);
	    }
	}
	validate (ap2);
	g_code (op, ilen, ap1, ap2);
	break;
    case IL8:
	if (ap1->mode == am_immed && is_icon (ap1->u.offset)) {
	    IVAL    i = ap1->u.offset->v.i;

	    if (i == 32L) {
		switch (op) {
		case op_asl:
		    g_code (op_move, IL4, mk_high (ap2), mk_low (ap2));
		    g_code (op_clr, IL4, mk_high (ap2), NIL_ADDRESS);
		    break;
		case op_asr:
		    g_code (op_move, IL4, mk_low (ap2), mk_high (ap2));
		    g_code (op_slt, IL0, mk_low (ap2), NIL_ADDRESS);
		    break;
		case op_lsr:
		    g_code (op_move, IL4, mk_low (ap2), mk_high (ap2));
		    g_code (op_clr, IL4, mk_low (ap2), NIL_ADDRESS);
		    break;
		default:
		    CANNOT_REACH_HERE ();
		}
		return ap1;
	    }
	}
	label = nextlabel++;
	ap1 = mk_legal (ap1, F_DREG, tp_ulong);
	g_label (label);
	switch (op) {
	case op_asl:
	    g_code (op, IL4, mk_high (ap2), NIL_ADDRESS);
	    g_code (op_roxl, IL4, mk_low (ap2), NIL_ADDRESS);
	    break;
	case op_asr:
	    g_code (op, IL4, mk_low (ap2), NIL_ADDRESS);
	    g_code (op_roxr, IL4, mk_high (ap2), NIL_ADDRESS);
	    break;
	case op_lsr:
	    g_code (op, IL4, mk_low (ap2), NIL_ADDRESS);
	    g_code (op_roxr, IL4, mk_high (ap2), NIL_ADDRESS);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	g_code (op_dbra, IL0, ap1, mk_label (label));
	break;
    default:
	CANNOT_REACH_HERE ();
    }
    return ap1;
}

/*
 * generate code to evaluate a shift node and return the address mode of the
 * result.
 */
static ADDRESS *g_shift P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;
    TYP    *tp = ep->etp;

    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL | F_COLD));
    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED | F_COLD));
    validate (ap1);
    ap2 = g_shft (op, (ILEN) tp->size, ap2, ap1);
    freeop (ap2);

    return mk_legal (ap1, flags, tp);
}

/*
 * generate shift equals operators.
 */
static ADDRESS *g_asshift P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    FLAGS   flagx;
    ADDRESS *ap1, *ap2, *ap3;
    TYP    *tp = ep->etp;

    switch (tp->type) {
    case bt_uchar:
    case bt_schar:
    case bt_char:
    case bt_charu:
    case bt_ushort:
    case bt_short:
    case bt_int16:
    case bt_uint16:
#ifdef COLDFIRE
	if (is_coldfire ()) {
	    if (ep->v.p[0]->nodetype == en_fieldref) {
		return g_asbitfield (ep, flags, op, FALSE);
	    }
	    flagx = (FLAGS) (F_ALL | F_USES);
	    ap1 = g_expr (ep->v.p[0], flagx);
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED));

	    /* quick constant if 1<=const<=8 */
	    if (ap2->mode == am_immed && is_icon (ap2->u.offset)
		&& (ap2->u.offset->v.i > 8L || ap2->u.offset->v.i < 1L)) {
		ap2 = mk_legal (ap2, F_DREG, tp_char);
	    }
	    validate (ap1);
	    ap3 = data_register ();
	    g_move ((ILEN) tp->size, ap1, ap3);
	    ap3 = mk_legal (ap3, (FLAGS) (F_DREG | F_COLD), tp);
	    g_code (op, IL4, ap2, ap3);
	    g_move ((ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	    freeop (ap2);
	    return mk_legal (ap1, flags, tp);
	}
	/*lint -fallthrough */
#endif /* COLDFIRE */
    case bt_int32:
    case bt_uint32:
    case bt_ulong:
    case bt_long:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}
	flagx = (FLAGS) (F_ALL | F_USES);
	ap1 = g_expr (ep->v.p[0], flagx);
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED));

	/* quick constant if 1<=const<=8 */
	if (ap2->mode == am_immed && is_icon (ap2->u.offset)
	    && (ap2->u.offset->v.i > 8L || ap2->u.offset->v.i < 1L)) {
	    ap2 = mk_legal (ap2, F_DREG, tp_char);
	}
	validate (ap1);
	if (ap1->mode == am_dreg) {
	    g_code (op, (ILEN) tp->size, ap2, ap1);
	} else {
#if 0
	    /*
	     * assemblers cannot agree on the operands for this special
	     * version of the shift operator
	     */
	    if (tp->size == 2L && is_icon (ep->v.p[1])
		&& ep->v.p[1]->v.i == 1) {
		g_code (op, 2, ap1, NIL_ADDRESS);
		return mk_legal (ap1, flags, tp);
	    }
#endif
	    ap3 = data_register ();
	    g_move ((ILEN) tp->size, ap1, ap3);
	    g_code (op, (ILEN) tp->size, ap2, ap3);
	    g_move ((ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    default:
	FATAL ((__FILE__, "g_asshift", "illegal type %d", tp->type));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*============================================================================*/


/*
 * generate code to evaluate a divide operator
 */
static ADDRESS *g_div P3 (const EXPR *, ep, FLAGS, flags, BOOL, mod)
{
    ADDRESS *ap1, *ap2;
    OPCODE  op = op_divu;
    TYP    *tp = ep->etp;
    TYP    *tp2 = tp_ushort;
    EXPR   *ep0 = ep->v.p[0];
    EXPR   *ep1 = ep->v.p[1];

    switch (tp->type) {
    case bt_char:
    case bt_schar:
	tp = tp_short;
	op = op_divs;
	/*lint -fallthrough */
    case bt_uchar:
    case bt_charu:
	ap1 = g_expr (ep0, (FLAGS) (F_DREG | F_VOL));
	ap1 = g_extend (ap1, tp, tp2);
	ap2 = g_expr (ep1, (FLAGS) (F_ALL & ~F_AREG));
	ap2 = g_extend (ap2, tp, tp2);
	validate (ap1);
	ap1 = g_extend (ap1, tp, tp_long);
	g_code (op, IL0, ap2, ap1);
	if (mod) {
	    g_code (op_swap, IL0, ap1, NIL_ADDRESS);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);

    case bt_short:
    case bt_int16:
	op = op_divs;
	/*lint -fallthrough */
    case bt_ushort:
    case bt_uint16:
	ap1 = g_expr (ep0, (FLAGS) (F_DREG | F_VOL));
	ap2 = g_expr (ep1, (FLAGS) (F_ALL & ~F_AREG));
	validate (ap1);
	ap1 = g_extend (ap1, ep0->etp, tp_long);
	g_code (op, IL0, ap2, ap1);
	if (mod) {
	    g_code (op_swap, IL0, ap1, NIL_ADDRESS);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);

    case bt_int32:
    case bt_long:
	op = op_divs;
	/*lint -fallthrough */
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	ap1 = g_expr (ep0, (FLAGS) (F_DREG | F_VOL));
	ap2 = g_expr (ep1, (FLAGS) (F_ALL & ~F_AREG));
	validate (ap1);
	g_code (op, IL4, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);

#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap1 = g_expr (ep0, (FLAGS) (F_FREG | F_VOL));
	ap2 = g_expr (ep1, (FLAGS) (F_FREG | F_MEM));
	validate (ap1);
	g_fcode (op_fdiv, (ILEN) tp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_div", "%d: illegal type %d", mod, tp->type));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*
 * generate /= and %= nodes.
 */
static ADDRESS *g_asdiv P3 (const EXPR *, ep, FLAGS, flags, BOOL, mod)
{
    ADDRESS *ap1, *ap2, *ap3;
    OPCODE  op = op_divu;
    TYP    *tp = ep->etp;
    FLAGS   xflag = (FLAGS) (F_ALL & ~F_AREG);

    switch (tp->type) {
    case bt_char:
    case bt_schar:
	xflag = (FLAGS) (F_DREG | F_IMMED);
	/*lint -fallthrough */
    case bt_short:
    case bt_int16:
	op = op_divs;
	goto common;	    /*lint !e801*/  /* use of goto is deprecated*/
    case bt_charu:
    case bt_uchar:
	xflag = (FLAGS) (F_DREG | F_IMMED);
	/*lint -fallthrough */
    case bt_ushort:
    case bt_uint16:
      common:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, mod);
	}
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_USES));
	ap2 = g_expr (ep->v.p[1], xflag);
	ap2 = g_extend (ap2, tp, tp_short);
	validate (ap1);
	if (ap1->mode == am_dreg) {
	    ap1 = g_extend (ap1, tp, tp_long);
	    g_code (op, IL0, ap2, ap1);
	    if (mod) {
		g_code (op_swap, IL0, ap1, NIL_ADDRESS);
	    }
	} else {
	    ap3 = data_register ();
	    g_move ((ILEN) tp->size, ap1, ap3);
	    ap3 = g_extend (ap3, tp, tp_long);
	    g_code (op, IL0, ap2, ap3);
	    if (mod) {
		g_code (op_swap, IL0, ap3, NIL_ADDRESS);
	    }
	    g_move ((ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    case bt_int32:
    case bt_long:
	op = op_divs;
	/*lint -fallthrough */
    case bt_ulong:
    case bt_uint32:
    case bt_pointer32:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, mod);
	}
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_USES));
	ap2 = g_expr (ep->v.p[1], xflag);
	validate (ap1);
	if (ap1->mode == am_dreg) {
	    g_code (op, IL4, ap2, ap1);
	} else {
	    ap3 = data_register ();
	    g_move ((ILEN) tp->size, ap1, ap3);
	    g_code (op, IL4, ap2, ap3);
	    g_move ((ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_USES));
	ap2 = g_expr (ep->v.p[1], F_FREG);
	validate (ap1);
	if (ap1->mode == am_freg) {
	    g_fcode (op_fdiv, (ILEN) tp->size, ap2, ap1);
	} else {
	    ap3 = float_register ();
	    g_fcode (op_fmove, (ILEN) tp->size, ap1, ap3);
	    g_fcode (op_fdiv, (ILEN) tp->size, ap2, ap3);
	    g_fcode (op_fmove, (ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "asdiv", "%d: illegal type %d", mod, tp->type));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*============================================================================*/

/*
 * Attempt to inline multiple by constants which are "near" a power of 2
 */
static ADDRESS *g_mul_icon P2 (const EXPR *, ep, FLAGS, flags)
{
    OPCODE  op;
    IVAL    j, value, power, i = ep->v.p[1]->v.i;
    ADDRESS *ap1, *ap2, *ap3;
    BOOL    negate;

    if (i < 0L) {
	i = -i;
	negate = TRUE;
    } else {
	negate = FALSE;
    }
    for (j = 0L, value = 1L; value != 0L && value < i; j++, value <<= 1L) {
	if ((power = (IVAL) pwrof2 (i - value)) != -1L) {
	    op = op_add;
	} else if ((power = (IVAL) pwrof2 (i + value)) != -1L) {
	    op = op_sub;
	} else {
	    continue;
	}
	ap1 = g_expr (ep->v.p[0], flags);
	ap2 = data_register ();
	if (ap1->mode == am_dreg) {
	    validate (ap1);
	    ap3 = ap1;
	} else {
	    ap3 = data_register ();
	    validate (ap1);
	    g_code (op_move, IL4, ap1, ap3);
	}
	g_code (op_move, IL4, ap3, ap2);
	freeop (g_shft (op_asl, IL4, mk_immed (power - j), ap3));
	g_code (op, IL4, ap2, ap3);
	freeop (g_shft (op_asl, IL4, mk_immed (j), ap3));
	if (negate) {
	    g_code (op_neg, IL4, ap3, NIL_ADDRESS);
	}
	if (ap1 != ap3) {
	    g_code (op_move, IL4, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return ap1;
    }
    return NIL_ADDRESS;
}

/*
 * performs a mixed-mode multiplication
 */
static ADDRESS *g_xmul P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;

    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_DREG | F_VOL));
    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_ALL & ~F_AREG));
    validate (ap1);
    g_code (op, IL0, ap2, ap1);
    freeop (ap2);
    return mk_legal (ap1, flags, ep->etp);
}

static ADDRESS *g_mul P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;
    OPCODE  op = op_mulu;
    TYP    *tp = ep->etp;
    EXPR   *ep0 = ep->v.p[0];
    EXPR   *ep1 = ep->v.p[1];
    TYP    *tp2 = tp_ushort;

    switch (tp->type) {
    case bt_char:
    case bt_schar:
	tp2 = tp_short;
	op = op_muls;
	/*lint -fallthrough */
    case bt_charu:
    case bt_uchar:
	ap1 = g_expr (ep0, (FLAGS) (F_DREG | F_VOL));
	ap1 = g_extend (ap1, tp, tp2);
	ap2 = g_expr (ep1, (FLAGS) (F_DREG | F_IMMED | F_VOL));
	ap2 = g_extend (ap2, tp, tp2);
	validate (ap1);
	g_code (op, IL0, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);

    case bt_short:
    case bt_int16:
	op = op_muls;
	/*lint -fallthrough */
    case bt_ushort:
    case bt_uint16:
	ap1 = g_expr (ep0, (FLAGS) (F_DREG | F_VOL));
	ap2 = g_expr (ep1, (FLAGS) (F_ALL & ~F_AREG));
	validate (ap1);
	g_code (op, IL0, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    case bt_int32:
    case bt_long:
	op = op_muls;
	/*lint -fallthrough */
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	/*
	 * special optimization possible if there are patterns matching the
	 * 68000 mulu, muls instructions. ugly, but it gives a big
	 * performance increase
	 */
	if (tst_ushort (ep0) && tst_ushort (ep1)) {
	    ep0->etp = tp_ushort;
	    ep1->etp = tp_ushort;
	    return g_xmul (ep, flags, op_mulu);
	}
	if (tst_short (ep0) && tst_short (ep1)) {
	    ep0->etp = tp_short;
	    ep1->etp = tp_short;
	    return g_xmul (ep, flags, op_muls);
	}
	if (is_icon (ep1)) {
	    ap1 = g_mul_icon (ep, (FLAGS) (F_DREG | F_VOL));
	    if (ap1 != NIL_ADDRESS) {
		return mk_legal (ap1, flags, tp);
	    }
	}
	if (target_option < target_68020) {
	    return g_expr (g_transform ((EXPR *) ep), flags);
	}
	ap1 = g_expr (ep0, (FLAGS) (F_DREG | F_VOL));
	ap2 = g_expr (ep1, (FLAGS) (F_ALL & ~F_AREG));
	validate (ap1);
	g_code (op, IL4, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);

#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap1 = g_expr (ep0, (FLAGS) (F_FREG | F_VOL));
	ap2 = g_expr (ep1, (FLAGS) (F_FREG | F_MEM));
	validate (ap1);
	g_fcode (op_fmul, (ILEN) tp->size, ap2, ap1);
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "g_mul", "illegal type %d", tp->type));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*
 * generate a *= ep.
 */
static ADDRESS *g_asmul P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2, *ap3;
    OPCODE  op = op_mulu;
    TYP    *tp = ep->etp;
    FLAGS   xflags = F_ALL;

    switch (tp->type) {
    case bt_char:
    case bt_schar:
	xflags = (FLAGS) (F_DREG | F_IMMED);
	/*lint -fallthrough */
    case bt_short:
    case bt_int16:
	op = op_muls;
	goto common;	    /*lint !e801*/  /* use of goto is deprecated*/
    case bt_charu:
    case bt_uchar:
	xflags = (FLAGS) (F_DREG | F_IMMED);
	/*lint -fallthrough */
    case bt_ushort:
    case bt_uint16:
      common:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_USES));
	ap2 = g_expr (ep->v.p[1], xflags);
	ap2 = g_extend (ap2, tp, tp_short);
	validate (ap1);
	if (ap1->mode == am_dreg) {
	    ap1 = g_extend (ap1, tp, tp_short);
	    g_code (op, IL0, ap2, ap1);
	} else {
	    ap3 = data_register ();
	    g_move ((ILEN) tp->size, ap1, ap3);
	    ap3 = g_extend (ap3, tp, tp_short);
	    g_code (op, IL0, ap2, ap3);
	    g_move ((ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
    case bt_int32:
    case bt_long:
	op = op_muls;
	/*lint -fallthrough */
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	if (ep->v.p[0]->nodetype == en_fieldref) {
	    return g_asbitfield (ep, flags, op, FALSE);
	}
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_USES));
	ap2 = g_expr (ep->v.p[1], xflags);
	validate (ap1);
	if (ap1->mode == am_dreg) {
	    g_code (op, IL4, ap2, ap1);
	} else {
	    ap3 = data_register ();
	    g_move ((ILEN) tp->size, ap1, ap3);
	    g_code (op, IL4, ap2, ap3);
	    g_move ((ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_USES));
	ap2 = g_expr (ep->v.p[1], F_FREG);
	validate (ap1);
	if (ap1->mode == am_freg) {
	    g_fcode (op_fmul, (ILEN) tp->size, ap2, ap1);
	} else {
	    ap3 = float_register ();
	    g_fcode (op_fmove, (ILEN) tp->size, ap1, ap3);
	    g_fcode (op_fmul, (ILEN) tp->size, ap2, ap3);
	    g_fcode (op_fmove, (ILEN) tp->size, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, tp);
#endif /* FLOAT_IEEE */
    default:
	FATAL ((__FILE__, "asmul", "illegal type %d", tp->type));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*============================================================================*/

/*
 * generate code to evaluate a condition operator node (?:)
 */
static ADDRESS *g_hook P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;
    FLAGS   flagx;
    BOOL    result_is_void = FALSE;
    TYP    *tp = ep->etp;
    SIZE    offset;
    LABEL   false_label = nextlabel++;
    LABEL   end_label = nextlabel++;

    switch (tp->type) {
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
    case bt_floatcomplex:
    case bt_doublecomplex:
    case bt_longdoublecomplex:
    case bt_struct:
    case bt_union:
	tp = tp_pointer;
	/*lint -fallthrough */
    default:
	flagx = (FLAGS) (flags & (F_DREG | F_AREG)) == F_AREG ?
	    (FLAGS) (F_AREG | F_VOL) : (FLAGS) (F_DREG | F_VOL);
	break;
    }

    temp_inv (NIL_REGUSAGE);	/* I do not think I can avoid that */
    offset = stack_offset;
    stack_offset = 0L;

    /* all scratch registers are void */

    g_falsejp (ep->v.p[0], false_label);
    ep = ep->v.p[1];

    /* all scratch registers are void */

    ap1 = g_expr (ep->v.p[0], flagx);
#ifdef FLOAT_IEEE
    if (flagx == F_MEM) {
	ADDRESS *ap;

	freeop (ap1);
	ap = address_register ();
	g_code (op_lea, IL0, ap1, ap);
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
	ap = address_register ();
	g_code (op_lea, IL0, ap2, ap);
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
 * rotate a bitfield into the required position (assumes ap is in a register)
 */
static void g_rotate P5 (ADDRESS *, ap, const TYP *, etp, int, offset, const TYP *, tp,
			 int, width)
{
    OPCODE  op;
    int     w;
    ADDRESS *ap1;
    ILEN    ilen = (ILEN) etp->size;

    switch (tp->type) {
    case bt_char:
    case bt_schar:
    case bt_short:
    case bt_int16:
    case bt_int32:
    case bt_long:
	/* signed bitfield */
	w = (int) (((ilen * 8) - offset) - width);
	if (w > 0) {
	    if (w > 8) {
		ap1 = data_register ();
		g_move (ilen, mk_immed ((IVAL) w), ap1);
	    } else {
		ap1 = mk_immed ((IVAL) w);
	    }
	    g_code (op_rol, ilen, ap1, ap);
	    freeop (ap1);
	}
	w = (int) ((ilen * 8) - width);
	if (w > 0) {
	    if (w > 8) {
		ap1 = data_register ();
		g_move (ilen, mk_immed ((IVAL) w), ap1);
	    } else {
		ap1 = mk_immed ((IVAL) w);
	    }
	    g_code (op_asr, ilen, ap1, ap);
	    freeop (ap1);
	}
	break;

    default:
	/* offset is in range -31..31 */
	if (offset < 0) {
	    offset = (int) (ilen * 8 + offset);
	}
	/* offset in range 0..31 */
	if (ilen == IL4 && offset > 8 && offset < 24) {
	    g_code (op_swap, IL0, ap, NIL_ADDRESS);
	}
	offset = (offset + 16) % 16;

	/* offset in range 0..15 */
	if (offset != 0) {
	    if (offset > 8) {
		op = op_rol;
		offset = 16 - offset;
	    } else {
		op = op_ror;
	    }
	    g_code (op, ilen, mk_immed ((IVAL) offset), ap);
	}
	if (!is_void (tp)) {
	    g_immed (op_and, etp, (IVAL) bitmask ((BITSIZE) width), ap);
	}
    }
}


/*
 * generate the code for assign operators in bitfield
 */
static ADDRESS *g_asbitfield P4 (const EXPR *, ep, FLAGS, flags, OPCODE, op,
				 BOOL, swap)
{
    ADDRESS *ap1, *ap2, *ap3;
    EXPR   *ep1, *lnode = ep->v.bit.p;
    TYP    *tp;
    int     width = (int) lnode->v.bit.width;
    int     offset = (int) lnode->v.bit.offset;
    ILEN    ilen = (ILEN) lnode->etp->size;
    UVAL    mask;

    /* Evaluate the address of the LHS */
    ep1 = mk_ref (lnode->v.bit.p, tp_pointer);
    ap2 = g_expr (ep1, F_MEM);

    /* Now get the value of the LHS, rotate and mask out unwanted bits */
    ap1 = data_register ();
    g_move (ilen, ap2, ap1);
    switch (op) {
    case op_muls:
	if (ilen < IL4) {
	    tp = tp_short;
	    break;
	}
	/*lint -fallthrough */
    case op_divs:
	tp = tp_long;
	break;
    case op_mulu:
	if (ilen < IL4) {
	    tp = tp_ushort;
	    break;
	}
	/*lint -fallthrough */
    case op_divu:
	tp = tp_ulong;
	break;
    default:
	tp = lnode->etp;
	break;
    }
    g_rotate (ap1, tp, (int) offset, lnode->etp, width);

    /* evaluate the RHS */
    ap3 = g_expr (ep->v.p[1], (FLAGS) (F_DREG | F_IMMED));
    validate (ap1);
    validate (ap2);

    /* now do the operation, masking the result back to the required size */
    if (ilen == IL4) {
	switch (op) {
	case op_divs:
	    g_code (op, IL4, ap3, ap1);
	    swap = FALSE;
	    break;
	case op_divu:
	    g_code (op, IL4, ap3, ap1);
	    swap = FALSE;
	    break;
	case op_muls:
	case op_mulu:
	    g_code (op, IL4, ap3, ap1);
	    break;
	default:
	    g_code (op, ilen, ap3, ap1);
	    break;
	}
    } else {
	switch (op) {
	case op_divs:
	case op_divu:
	    g_code (op, IL0, ap3, ap1);
	    if (swap) {
		g_code (op_swap, IL0, ap1, NIL_ADDRESS);
	    }
	    swap = FALSE;
	    break;
	case op_muls:
	case op_mulu:
	    g_code (op, IL0, ap3, ap1);
	    break;
	default:
	    g_code (op, ilen, ap3, ap1);
	    break;
	}
    }
    freeop (ap3);
    mask = (UVAL) bitmask ((BITSIZE) width);
    g_immed (op_and, lnode->etp, (IVAL) mask, ap1);

    /* rotate result back into position, and store */
    g_rotate (ap1, lnode->etp, -offset, tp_void, 0);
    g_immed (op_and, lnode->etp, (IVAL) ~(mask << offset), ap2);
    g_code (op_or, ilen, ap1, ap2);
    freeop (ap1);
    freeop (ap2);

    /* return a result */
    ap2 = data_register ();
    g_move (IL4, ap1, ap2);
    ap1 = ap2;

    if ((FLAGS) (flags & F_NOVALUE) == F_NONE) {
	/* result value needed */
	g_rotate (ap1, lnode->etp, offset, tp_void, 0);
	if (swap) {
	    /* post increment/decrement restore original value */
	    switch (op) {
	    case op_add:
		g_sub (ilen, mk_immed ((IVAL) 1L), ap1);
		g_immed (op_and, ep->etp, (IVAL) mask, ap1);
		break;
	    case op_sub:
		g_add (ilen, mk_immed ((IVAL) 1L), ap1);
		g_immed (op_and, ep->etp, (IVAL) mask, ap1);
		break;
	    default:
		break;
	    }
	}
    }
    return mk_legal (ap1, flags, ep->etp);
}


/*
 * assign structure from ap1 to ap2
 * ap1, ap2 are scratch address registers
 */
static void structassign P4 (ADDRESS *, ap1, ADDRESS *, ap2, SIZE, size,
			     const TYP *, tp)
{
    SIZE    loop;
    int     rest;
    ADDRESS *ap3;
    LABEL   label;
    SIZE    i;
    SIZE    algn = alignment_of_type (tp);
    ILEN    ilen;

    ap1 = copy_addr (ap1, am_ainc);
    ap2 = copy_addr (ap2, am_ainc);
    if ((size % 2L == 0L) && (algn % 2L == 0L)) {
	ilen = IL4;
    } else {
	ilen = IL1;
    }
    loop = size / (SIZE) ilen;
    rest = (int) (size % (SIZE) ilen);
    if (((loop <= 10L) && (target_option == target_68000)) ||	/* loop-unrolling only on 68000 */
	((loop <= 2L) && (target_option > target_68000))) {	/* on 68010 and higher: unroll only the trivial case */
	for (i = 1L; i <= loop; i++)
	    g_move (ilen, ap1, ap2);
    } else {
	loop--;			/* for dbra */
	ap3 = data_register ();
	freeop (ap3);
	label = nextlabel++;
	if (loop <= 65535L) {	/* single loop */
	    g_move (IL2, mk_immed ((IVAL) loop), ap3);
	    g_label (label);
	    g_move (ilen, ap1, ap2);
	    g_code (op_dbra, IL0, ap3, mk_label (label));
	} else {		/* extended loop */
	    g_move (IL4, mk_immed ((IVAL) loop), ap3);
	    g_label (label);
	    g_move (ilen, ap1, ap2);
	    g_code (op_dbra, IL0, ap3, mk_label (label));
	    g_sub (IL4, mk_immed ((IVAL) 65536L), ap3);
	    g_cbranch (op_bhs, label);
	}
    }
    if (rest >= 2) {
	rest -= 2;
	g_move (IL2, ap1, ap2);
    }
    if (rest >= 1) {
	g_move (IL1, ap1, ap2);
    }
}

/*
 * generate code for an assignment node.
 */
static ADDRESS *g_assign P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2, *ap3;
    EXPR   *ep0, *ep1;
    TYP    *tp = ep->etp;
    ILEN    ilen;
    UVAL    mask;
    FLAGS   flagx;

    flagx = is_coldfire ()? (FLAGS) (F_DREG | F_AREG | F_COLD) : F_ALL;

    if (!(flags & F_NOVALUE)) {
	flagx |= F_USES;
    }
    switch (tp->type) {
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
	/*
	 * Other parts of this module return a pointer to a struct in a register,
	 * not the struct itself
	 */
	ap1 = g_expr (ep->v.p[1], (FLAGS) (F_AREG | F_VOL));
	ap2 = g_expr (ep->v.p[0], (FLAGS) (F_AREG | F_VOL));
	validate (ap1);

	/* hacky: save ap1 if needed later, structassign destroys it */
	if (flags & F_NOVALUE) {
	    /* no need to save any registers */
	    structassign (ap1, ap2, tp->size, tp);
	    freeop (ap2);
	    freeop (ap1);
	    return NIL_ADDRESS;
	} else {
	    ap3 = address_register ();
	    g_move (IL4, ap1, ap3);
	    structassign (ap3, ap2, tp->size, tp);
	    freeop (ap3);
	    freeop (ap2);
	    validate (ap1);
	    return mk_legal (ap1, flags, tp_pointer);
	}
    default:
      common:
	ilen = (ILEN) tp->size;
	switch (ilen) {
	case IL1:
	case IL2:
	case IL4:
	case IL8:
	case IL12:
	    ep0 = ep->v.p[0];
	    switch (ep0->nodetype) {
	    case en_fieldref:
		/*
		 * Field assignment
		 */
		/* get the value */
		mask = (UVAL) bitmask (ep0->v.bit.width);
		ap1 = g_expr (ep->v.p[1], (FLAGS) (F_IMMED | F_DREG | F_VOL));
		if (ap1->mode == am_immed) {
		    ap1->u.offset->v.i &= (IVAL) mask;
		    ap3 = mk_immed ((IVAL)
				    ((UVAL) ap1->u.offset->v.i << (int) ep0->v.bit.offset));
		} else {
		    if (flags & F_NOVALUE) {
			ap3 = ap1;
			g_immed (op_and, ep->etp, (IVAL) mask, ap3);
		    } else {
			/* result value needed */
			if (is_signed_type (tp)) {
			    SIZE    i = (tp_int->size * 8L -
				(SIZE) ep0->v.bit.width) -
				(SIZE) ep0->v.bit.offset;

			    ap3 = mk_immed ((IVAL) i);
			    ap3 = g_shft (op_asl, ilen, ap3, ap1);
			    ap3 = g_shft (op_asr, ilen, ap3, ap1);
			    freeop (ap3);
			    ap3 = data_register ();
			    g_move (IL4, ap1, ap3);
			    g_immed (op_and, ep->etp, (IVAL) mask, ap3);
			} else {
			    g_immed (op_and, ep->etp, (IVAL) mask, ap1);
			    ap3 = data_register ();
			    g_move (IL4, ap1, ap3);
			}
		    }
		    g_rotate (ap3, ep->etp, -(int) ep0->v.bit.offset, tp_void,
			      0);
		}
		mask <<= ep0->v.bit.offset;
		ep1 = mk_ref (ep0->v.bit.p, tp_pointer);
		ap2 = g_expr (ep1, F_MEM);
		validate (ap3);
		if (ap3->mode != am_immed || ap3->u.offset->v.u != mask)
		    g_immed (op_and, ep->etp, (IVAL) ~mask, ap2);
		if (ap3->mode != am_immed || ap3->u.offset->v.u != 0)
		    g_code (op_or, ilen, ap3, ap2);
		freeop (ap2);
		if (!(flags & F_NOVALUE)) {
		    freeop (ap3);
		    validate (ap1);
		}
		break;

		/*
		 * we want to pass the right hand side as the expression value.
		 * This can''t be done if the left side is a register variable
		 * on which the right hand side addressing mode depends. But if
		 * the left side IS a register variable, it is desirable to pass
		 * the left side, so no problem.
		 */
	    case en_register:
		/* pass the left side as expr. value */
		ap1 = g_expr (ep->v.p[0], flagx);
		ap2 = g_expr (ep->v.p[1], F_ALL);
		validate (ap1);
#ifdef FLOAT_IEEE
		if (ap1->mode == am_freg) {
		    g_fcode (op_fmove, ilen, ap2, ap1);
		} else
#endif /* FLOAT_IEEE */
		    g_move (ilen, ap2, ap1);
		freeop (ap2);
		break;
	    default:
		if (ilen > IL4) {
		    flagx |= F_ALL;
		}
		/* pass the right side as expr. value */
		/* normally, this is more efficient */
		ap1 = g_expr (ep->v.p[1], flagx);
		ap2 = g_expr (ep->v.p[0], F_ALL);
		validate (ap1);
		switch (ilen) {
		case IL1:
		case IL2:
		case IL4:
#ifdef FLOAT_IEEE
		    if (ap1->mode == am_freg || ap2->mode == am_freg) {
			g_fcode (op_fmove, ilen, ap1, ap2);
		    } else
#endif /* FLOAT_IEEE */
			g_move (ilen, ap1, ap2);
		    break;
		case IL8:
		    g_move8 (ap1, ap2);
		    break;
		case IL12:
		    g_move12 (ap1, ap2);
		    break;
		default:
		    CANNOT_REACH_HERE ();
		    break;
		}
		freeop (ap2);
		break;
	    }
	    return mk_legal (ap1, flags, tp);
	default:
	    FATAL ((__FILE__, "g_assign", "size = %ld", tp->size));
	}
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
}

/*
 * push the operand expression onto the stack. return the number of bytes
 * pushed
 */
static SIZE push_param P1 (const EXPR *, ep)
{
    ADDRESS *ap, *ap1;
    SIZE    size = ep->etp->size;

    /* pushing of structures and unions */
    switch (ep->etp->type) {
    case bt_floatcomplex:
    case bt_doublecomplex:
    case bt_longdoublecomplex:
    case bt_struct:
    case bt_union:
	/*
	 * It is possible that struct/union is an odd number of bytes if
	 * it is comprised only of chars - but we must ensure that the stack
	 * is always an even number of bytes.
	 */
	if ((USIZE) size & (USIZE) 1) {
	    size++;
	}
	if (is_lvalue (ep)) {
	    ep = ep->v.p[0];
	}
	/* all other cases return a pointer to the struct anyway */
	/* allocate stack space */
	g_add (IL4, mk_immed ((IVAL) -size), mk_reg (STACKPTR));
	ap = g_expr (ep, (FLAGS) (F_AREG | F_VOL));
	ap1 = address_register ();
	validate (ap);
	g_move (IL4, mk_reg (STACKPTR), ap1);
	/* now, copy it on stack - the same as structassign */
	structassign (ap, ap1, size, ep->etp);
	freeop (ap1);
	freeop (ap);
	break;
    default:
	ap = g_expr (ep, F_ALL);

	/*
	 * This is a hook for the peephole optimizer, which will convert lea
	 * <ea>,An + pea (An) ==> pea <ea>
	 */

	if (ap->mode == am_areg && size == 4L
	    && is_temporary_register (ap->preg)) {
	    ap = copy_addr (ap, am_ind);
	    g_code (op_pea, IL0, ap, NIL_ADDRESS);
	} else {

	    switch (size) {
	    case 1L:
		/*
		 * char parameters with prototype ... push an extra
		 * "junk" byte to keep the stack aligned.
		 */
		size++;
		ap = mk_legal (ap, F_DREG, ep->etp);
		/*lint -fallthrough */
	    case 2L:
	    case 4L:
		g_move ((ILEN) size, ap, &push);
		break;
	    case 8L:
		g_move8 (ap, &push);
		break;
	    case 12L:
		g_move12 (ap, &push);
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	}
	freeop (ap);
	break;
    }
    return size;
}

/*
 * push a list of parameters onto the stack and return the number of
 * bytes that they occupy on the stack.
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


/*
 *	 Return value is the addressing mode of the result
 *	 This routine does not use mk_legal and takes care of the
 *	 stuff itself.
 */

static ADDRESS *func_result P3 (FLAGS, flags, SIZE, bytes, const TYP *, tp)
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
	ADDRESS *ap1;

    case bt_longdouble:
	if (flags & F_FREG && fpu_option) {
	    ap = float_register ();
	    if (fpu_return_option) {
		ap1 = mk_reg (FP0);
	    } else {
		ap1 = mk_smask ((REGMASK)
				REGBIT (reg_usage->result->reg[0]) |
				REGBIT (reg_usage->result->reg[1]) |
				REGBIT (reg_usage->result->reg[2]));
		g_movem (ap1, &push);
		ap1 = &pop;
	    }
	    g_fcode (op_fmove, (ILEN) tp->size, ap1, ap);
	    return ap;
	} else if (flags & F_DREG) {
	    ap = xdata_register ();
	    ap1 = mk_xreg (reg_usage->result->reg[0],
			   reg_usage->result->reg[1],
			   reg_usage->result->reg[2]);
	    g_move12 (ap1, ap);
	    return ap;
	} else if (flags & F_MEM) {
	    ap = mk_scratch (tp->size);
#ifdef MOVEMBUG
	    g_move (IL4, mk_reg (reg_usage->result->reg[0]), mk_low (ap));
	    g_move (IL4, mk_reg (reg_usage->result->reg[1]), mk_high (ap));
	    g_move (IL4, mk_reg (reg_usage->result->reg[2]), mk_top (ap1));
#else
	    g_movem (mk_smask ((REGMASK)
			       REGBIT (reg_usage->result->reg[0]) |
			       REGBIT (reg_usage->result->reg[1]) |
			       REGBIT (reg_usage->result->reg[2])), ap);
#endif /* MOVEMBUG */
	    return ap;
	}
	break;
    case bt_double:
	if (flags & F_FREG && fpu_option) {
	    if (fpu_return_option) {
		ap1 = mk_reg (FP0);
	    } else {
		g_movem (mk_smask ((REGMASK)
				   REGBIT (reg_usage->result->reg[0]) |
				   REGBIT (reg_usage->result->reg[1])),
			 &push);
		ap1 = &pop;
	    }
	    ap = float_register ();
	    g_fcode (op_fmove, (ILEN) tp->size, ap1, ap);
	    return ap;
	} else if (flags & F_DREG) {
	    ap = mdata_register ();
	    g_move8 (mk_mreg
		     (reg_usage->result->reg[0], reg_usage->result->reg[1]),
		     ap);
	    return ap;
	} else if (flags & F_MEM) {
	    ap = mk_scratch (tp->size);
#ifdef MOVEMBUG
	    g_move (IL4, mk_reg (reg_usage->reg[0]), mk_low (ap));
	    g_move (IL4, mk_reg (reg_usage->reg[1]), mk_high (ap));
#else
	    g_movem (mk_smask ((REGMASK)
			       REGBIT (reg_usage->result->reg[0]) |
			       REGBIT (reg_usage->result->reg[1])), ap);
#endif /* MOVEMBUG */
	    return ap;
	}
	break;
    case bt_float:
	if (flags & F_FREG && fpu_option) {
	    if (fpu_return_option) {
		ap1 = mk_reg (FP0);
	    } else {
		g_move (IL4, mk_reg (reg_usage->result->reg[0]), &push);
		ap1 = &pop;
	    }
	    ap = float_register ();
	    g_fcode (op_fmove, (ILEN) tp->size, ap1, ap);
	    return ap;
	} else if (flags & F_DREG) {
	    ap = data_register ();
	    g_move (IL4, mk_reg (reg_usage->result->reg[0]), ap);
	    return ap;
	} else if (flags & F_MEM) {
	    ap = mk_scratch (tp->size);
	    g_move (IL4, mk_reg (reg_usage->result->reg[0]), ap);
	    return ap;
	}
	break;
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
    case bt_float:
    case bt_double:
    case bt_longdouble:
#endif /* FLOAT_MFFP */
    case bt_char:
    case bt_uchar:
    case bt_charu:
    case bt_schar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer16:
    case bt_pointer32:
    case bt_floatcomplex:	/* always returned as a pointer to result */
    case bt_doublecomplex:	/* always returned as a pointer to result */
    case bt_longdoublecomplex:	/* always returned as a pointer to result */
    case bt_struct:		/* always returned as a pointer to result */
    case bt_union:		/* always returned as a pointer to result */
	ap = temp_reg (flags);
	g_move (IL4, mk_reg (reg_usage->result->reg[0]), ap);
	return ap;
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	if (flags & F_DREG) {
	    ap = mdata_register ();
	    g_move8 (mk_mreg
		     (reg_usage->result->reg[0], reg_usage->result->reg[1]),
		     ap);
	    return ap;
	} else if (flags & F_MEM) {
	    ap = mk_scratch (tp->size);
#ifdef MOVEMBUG
	    g_move (IL4, mk_reg (reg_usage->reg[0]), mk_low (ap));
	    g_move (IL4, mk_reg (reg_usage->reg[1]), mk_high (ap));
#else
	    g_movem (mk_smask ((REGMASK)
			       REGBIT (reg_usage->result->reg[0]) |
			       REGBIT (reg_usage->result->reg[1])), ap);
#endif /* MOVEMBUG */
	    return ap;
	}
	break;
#endif /* LONGLONG_SUPPORT */
    default:
	FATAL (
	       (__FILE__, "func_result", "illegal mode (%d, %d)", tp->type,
		flags));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate a function call node and return the address mode of the result.
 */
static ADDRESS *g_fcall P2 (const EXPR *, ep, FLAGS, flags)
{
    EXPR   *ep0 = ep->v.p[0];
    TYP    *tp = ep->etp;
    ADDRESS *ap;
    SIZE    size;

    if (!is_parameter && ep->nodetype != en_call) {
	switch (stackopt_option) {

	case OPT_SAFE:
	    g_stack (stack_offset);
	    break;

	case OPT_MINIMUM:
	    /*
	     *               Perform a stack optimisation unless:
	     *               1.  The function call is via a variable
	     *               2.  The function starts with an underscore character
	     *               3.  The alloca() routine is called
	     */
	    if ((ep0->nodetype != en_nacon) ||
		(ep0->v.str[0] == (CHAR) '_') || (ep0->v.str == alloca_name)) {
		g_stack (stack_offset);
	    }
	    break;

	case OPT_AVERAGE:
	    /*
	     *               "Average" stack optimisation.   This will not suppress
	     *               the optimisation on encountering calls to functions
	     *               whose names begin with underscore.
	     */
	    if ((ep0->nodetype != en_nacon) || (ep0->v.str == alloca_name)) {
		g_stack (stack_offset);
	    }
	    break;

	case OPT_MAXIMUM:
	    /*
	     *               "Maximum" stack optimisation.   This will not suppress
	     *               the optimisation on encountering calls to functions
	     *               whose names begin with underscore or via a function
	     *               variable.
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
    /* push any used addr&data temps */
#if 0
    temp_inv (register_usage (tp));
#else
    temp_inv (NULL);
#endif
    size = g_parms (ep->v.p[1]);	/* generate parameters */
    if (is_structure_type (tp)
#ifdef FLOAT_SUPPORT
	|| is_complex_type (tp)
#endif /* FLOAT_SUPPORT */
	) {
	/*
	 *       For functions returning a structure or a union, push
	 *       a pointer to the return value as additional argument.
	 *       The scratch space will be allocated in the stack frame
	 *       of the calling function.
	 */
	ap = mk_scratch (tp->size);
	g_code (op_pea, IL0, ap, NIL_ADDRESS);
	size += tp_pointer->size;
	freeop (ap);
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
    case en_icon:
	ap = g_expr (ep0, F_IMMED);
	freeop (ap);
	break;
    case en_str:
	ap = g_expr (ep0, F_ALL);
	freeop (ap);
	break;
    default:
	ap = g_expr (ep0, F_AREG);
	ap = copy_addr (ap, am_ind);
	freeop (ap);
	break;
    }
    g_call (ap);

    return func_result (flags, size, tp);
}

/*
 *	 Generates code for an en_cast node.   The item addressed by
 *	 'ap' is cast from type 'tp1' to type 'tp2'.
 *
 *	 If there is no FPU then the casts to/from floating point
 *	 types will have already been changed into function calls.
 */
static ADDRESS *g_cast P4 (ADDRESS *, ap, const TYP *, tp1, const TYP *, tp2, FLAGS,
			   flags)
{
    ADDRESS *ap1;
    FLAGS   flagx;

    if (flags & F_NOVALUE) {
	freeop (ap);
	return NIL_ADDRESS;
    }
    if (is_same_type (tp1, tp2)) {
	/*
	 *       This can happen with the g_xmul stuff, where a cast from
	 *       (u)short to long now casts from (u)short to (u)short for
	 *       a 68000 mulu or muls instruction.
	 *       It is safe to cut things short then.
	 *       It should not happen with types other than (u)short, but
	 *       it does not harm either.
	 */
	switch (tp1->type) {
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	    return mk_legal (ap, flags, tp1);
	default:
	    FATAL ((__FILE__, "g_cast", "tp1==tp2 (%d)", tp1->type));
	    break;
	}
    }
    switch (tp2->type) {	/* switch: type to cast to */
    case bt_bool:
	switch (tp1->type) {
	case bt_bool:
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
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	    ap = mk_legal (ap, F_DREG | F_MEM, tp1);
	    g_code (op_tst, (ILEN) tp1->size, ap, NIL_ADDRESS);
	    freeop (ap);
	    ap1 = data_register ();
	    g_code (op_seq, IL0, ap1, NIL_ADDRESS);
	    g_immed (op_and, tp_uchar, (IVAL) 1L, ap1);
	    return mk_legal (ap1, flags, tp2);
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	case bt_ulonglong:
	    ap = mk_legal (ap, F_DREG, tp1);
	    g_code (op_or, IL4, mk_high (ap), mk_low (ap));
	    freeop (ap);
	    ap1 = data_register ();
	    g_code (op_seq, IL0, ap1, NIL_ADDRESS);
	    g_immed (op_and, tp_uchar, (IVAL) 1L, ap1);
	    return mk_legal (ap1, flags, tp2);
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    ap = mk_legal (ap, flags, tp2);
	    g_code (op_and, IL1, mk_immed ((IVAL) 1L), ap);
	    return ap;
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
    case bt_char:
    case bt_charu:
    case bt_schar:
    case bt_uchar:
	switch (tp1->type) {
	case bt_bool:
	case bt_uchar:
	case bt_schar:
	case bt_char:
	case bt_charu:
	    return mk_legal (ap, flags, tp2);
	case bt_ushort:
	case bt_short:
	case bt_int16:
	case bt_uint16:
	    if ((ap1 = mk_offset (ap, 1L)) == NIL_ADDRESS) {
		ap1 = mk_legal (ap, F_DREG, tp1);
	    }
	    return mk_legal (ap1, flags, tp2);
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	    if ((ap1 = mk_offset (ap, 3L)) == NIL_ADDRESS) {
		ap1 = mk_legal (ap, F_DREG, tp1);
	    }
	    return mk_legal (ap1, flags, tp2);
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	case bt_ulonglong:
	    ap = mk_legal (ap, F_DREG, tp1);
	    freeop (ap);
	    ap1 = data_register ();
	    g_code (op_move, IL1, mk_high (ap), ap1);
	    return mk_legal (ap1, flags, tp2);
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
    case bt_ushort:
    case bt_short:
    case bt_int16:
    case bt_uint16:
	switch (tp1->type) {
	case bt_bool:
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	    ap = mk_legal (ap, (FLAGS) (F_DREG | F_VOL), tp1);
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	    if ((ap1 = mk_offset (ap, 2L)) == NIL_ADDRESS) {
		ap1 = mk_legal (ap, F_DREG, tp1);
	    }
	    return mk_legal (ap1, flags, tp2);
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	case bt_ulonglong:
	    ap = mk_legal (ap, F_DREG | F_MEM, tp1);
	    freeop (ap);
	    ap1 = data_register ();
	    g_code (op_move, IL2, mk_high (ap), ap1);
	    return mk_legal (ap1, flags, tp2);
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	switch (tp1->type) {
	case bt_bool:
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	case bt_ushort:
	case bt_uint16:
	    ap = mk_legal (ap, (FLAGS) (F_DREG | F_VOL), tp1);
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_short:
	case bt_int16:
	    flagx = (FLAGS) (flags & (F_DREG | F_AREG));
	    if (flagx == F_NONE) {
		flagx = (FLAGS) (F_DREG | F_AREG);
	    }
	    ap = mk_legal (ap, flagx, tp1);
	    if (ap->mode == am_dreg) {
		ap = g_extend (ap, tp1, tp2);
	    }
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	case bt_func:
	    return mk_legal (ap, flags, tp2);
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	case bt_ulonglong:
	    ap = mk_legal (ap, F_DREG | F_MEM, tp1);
	    freeop (ap);
	    ap1 = data_register ();
	    g_code (op_move, IL4, mk_high (ap), ap1);
	    return mk_legal (ap1, flags, tp2);
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	switch (tp1->type) {
	case bt_bool:
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	case bt_ushort:
	case bt_uint16:
	case bt_short:
	case bt_int16:
	    ap = mk_legal (ap, (FLAGS) (F_DREG | F_VOL), tp1);
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	case bt_func:
	    ap = mk_legal (ap, (FLAGS) (F_DREG | F_MEM | F_VOL), tp1);
	    ap = g_extend (ap, tp1, tp2);
	    return mk_legal (ap, flags, tp2);
	case bt_longlong:
	case bt_ulonglong:
	    return mk_legal (ap, flags, tp2);
#ifdef FLOAT_IEEE
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ap = mk_legal (ap, F_FREG, tp1);
	    return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
	default:
	    break;
	}
	break;
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
    case bt_double:
    case bt_longdouble:
    case bt_float:
	switch (tp1->type) {
	case bt_bool:
	case bt_uchar:
	case bt_charu:
	case bt_ushort:
	case bt_uint16:
	    ap = mk_legal (ap, (FLAGS) (F_FREG | F_VOL), tp_long);
	    break;
#if 0
	case bt_float:
	    if (ap->mode == am_freg) {
		/*
		 *       A double or long double value which is in the FP
		 *       registers must be stored and then reloaded in order
		 *       to correctly round the bits of a float.  Although
		 *       this looks like a NULL operation it isn't!
		 */
		ap1 = mk_scratch (tp1->size);
		g_fcode (op_fmove, (ILEN) tp1->size, ap, ap1);
		g_fcode (op_fmove, (ILEN) tp1->size, ap1, ap);
	    }
	    /*lint -fallthrough */
#endif
	default:
	    ap = mk_legal (ap, (FLAGS) (F_FREG | F_VOL), tp1);
	    break;
	}
	return mk_legal (ap, flags, tp2);
#endif /* FLOAT_IEEE */
    default:
	break;
    }
    FATAL (
	   (__FILE__, "g_cast", "illegal combination type1=%d, type2=%d",
	    tp1->type, tp2->type));
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
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
    BTYPE   type;
    OPCODE  op;
    TYP    *tp = ep->etp;
    EXPR   *ep2;

    if (ep == NIL_EXPR) {
	FATAL ((__FILE__, "g_expr", "ep == 0"));
    }
    if (tst_const (ep)) {
	ap1 = mk_expr (am_immed, copynode (ep));
	return mk_legal (ap1, flags, tp);
    }
    type = tp->type;
    switch (ep->nodetype) {
    case en_autocon:
	ap1 = address_register ();
	if ((ep->v.i >= (IVAL) -32768L) && (ep->v.i <= (IVAL) 32767L)) {
	    ap2 = mk_indirect (regframe, copynode (ep));
	    g_code (op_lea, IL0, ap2, ap1);
	} else {
	    g_move (IL4, mk_immed (ep->v.i), ap1);
	    g_add (IL4, mk_reg (regframe), ap1);
	}
	return mk_legal (ap1, flags, tp);
    case en_global:
	ap1 = address_register ();
	ep2 = mk_global (SUP_DATA, ep);
	switch (datamodel_option) {
	case model_small:
	    /* assume 16 bit offset */
	    ap2 = mk_indirect (regdata, ep2);
	    g_code (op_lea, IL0, ap2, ap1);
	    break;
	case model_large:
	    /* assume 32 bit offset */
	    g_move (IL4, mk_expr (am_immed, ep2), ap1);
	    g_add (IL4, mk_reg (regdata), ap1);
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	return mk_legal (ap1, flags, tp);
    case en_register:
	ap1 = mk_reg (ep->v.r);
	return mk_legal (ap1, flags, tp);
    case en_ref:
	/*
	 * g_deref uses flags and size only to test F_USES
	 *
	 * If the result is not used, autoincrement addressing
	 * modes are wrong!
	 */
	if (flags & F_NOVALUE) {
	    ap1 = g_deref (ep->v.p[0], tp, (FLAGS) (flags | F_USES));
	} else {
	    ap1 = g_deref (ep->v.p[0], tp, flags);
	}
	if (is_structure_type (tp)
#ifdef FLOAT_SUPPORT
	    || is_complex_type (tp)
#endif /* FLOAT_SUPPORT */
	    || is_array_type (tp)) {
	    return mk_legal (ap1, flags, tp_pointer);
	} else {
	    return mk_legal (ap1, flags, tp);
	}
    case en_fieldref:
	return g_fderef (ep, flags);
    case en_uminus:
	return g_unary (ep, flags, op_neg);
    case en_compl:
	return g_unary (ep, flags, op_not);
    case en_add:
	return g_addsub (ep, flags, op_add);
    case en_sub:
	return g_addsub (ep, flags, op_sub);
    case en_and:
	return g_logic (ep, flags, op_and);
    case en_or:
	return g_logic (ep, flags, op_or);
    case en_xor:
	return g_logic (ep, flags, op_eor);
    case en_mul:
	return g_mul (ep, flags);
    case en_div:
	return g_div (ep, flags, FALSE);
    case en_mod:
	return g_div (ep, flags, TRUE);
    case en_lsh:
	return g_shift (ep, flags, op_asl);
    case en_rsh:
	return g_shift (ep, flags, (is_unsigned_type (tp) ? op_lsr : op_asr));
    case en_asadd:
	return g_asadd (ep, flags, op_add);
    case en_assub:
	return g_asadd (ep, flags, op_sub);
    case en_asand:
	return g_aslogic (ep, flags, op_and);
    case en_asor:
	return g_aslogic (ep, flags, op_or);
    case en_asxor:
	return g_aslogic (ep, flags, op_eor);
    case en_aslsh:
	return g_asshift (ep, flags, op_asl);
    case en_asrsh:
	return g_asshift (ep, flags,
			  (is_unsigned_type (tp) ? op_lsr : op_asr));
    case en_asmul:
	return g_asmul (ep, flags);
    case en_asdiv:
	return g_asdiv (ep, flags, FALSE);
    case en_asmod:
	return g_asdiv (ep, flags, TRUE);
    case en_assign:
	return g_assign (ep, flags);
    case en_ainc:
	return g_aincdec (ep, flags, op_add);
    case en_adec:
	return g_aincdec (ep, flags, op_sub);
    case en_eq:
	VOIDCAST g_compare (ep);
	op = op_seq;
	goto cont1;	    /*lint !e801*/  /* use of goto is deprecated*/
    case en_ne:
	VOIDCAST g_compare (ep);
	op = op_sne;
	goto cont1;	    /*lint !e801*/  /* use of goto is deprecated*/
    case en_lt:
	op = (OPCODE) (g_compare (ep) ? op_shi : op_sgt);
	goto cont1;	    /*lint !e801*/  /* use of goto is deprecated*/
    case en_le:
	op = (OPCODE) (g_compare (ep) ? op_shs : op_sge);
	goto cont1;	    /*lint !e801*/  /* use of goto is deprecated*/
    case en_gt:
	op = (OPCODE) (g_compare (ep) ? op_slo : op_slt);
	goto cont1;	    /*lint !e801*/  /* use of goto is deprecated*/
    case en_ge:
	op = (OPCODE) (g_compare (ep) ? op_sls : op_sle);
	goto cont1;	    /*lint !e801*/  /* use of goto is deprecated*/
    case en_test:
	g_test (ep->v.p[0]);
	op = op_sne;
	goto cont1;	    /*lint !e801*/  /* use of goto is deprecated*/
    case en_not:
	g_test (ep->v.p[0]);
	op = op_seq;
      cont1:
	ap1 = data_register ();
	g_code (op, IL0, ap1, NIL_ADDRESS);
	g_immed (op_and, tp, (IVAL) 1L, ap1);
	return mk_legal (ap1, flags, tp);
    case en_land:
    case en_lor:
	lab0 = nextlabel++;
	lab1 = nextlabel++;
	g_falsejp (ep, lab0);
	ap1 = data_register ();
	g_code (op_moveq, IL0, mk_immed ((IVAL) 1L), ap1);
	g_branch (lab1);
	g_label (lab0);
	g_code (op_moveq, IL0, mk_immed ((IVAL) 0L), ap1);
	g_label (lab1);
	return mk_legal (ap1, flags, tp);
    case en_cond:
	return g_hook (ep, flags);
    case en_comma:
	freeop (g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_NOVALUE)));
	return g_expr (ep->v.p[1], flags);
    case en_fcall:
    case en_call:
    case en_usercall:
	return g_fcall (ep, flags);
    case en_cast:
	/*
	 * On the 68000, suppress all casts between any of
	 * long, unsigned long, pointer
	 */
	switch (type) {
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_int32:
	    case bt_uint32:
	    case bt_long:
	    case bt_ulong:
	    case bt_pointer32:
		return g_expr (ep->v.p[0], flags);
	    default:
		break;
	    }
	    break;
	default:
	    break;
	}
	/*
	 * The cast really results in some work
	 */
	return g_cast (g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_USES)),
		       ep->v.p[0]->etp, tp, flags);
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

    case en_literal:
	g_init ((EXPR *) ep);
	return NIL_ADDRESS;

    case en_fcon:
    default:
	FATAL ((__FILE__, "g_expr", "uncoded node %d", ep->nodetype));
    }
    return NIL_ADDRESS;	/*lint !e527*/	/* unreachable */
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
 * generate code to do a comparison of the two operands of node. returns 1 if
 * it was an unsigned comparison
 */
static BOOL g_compare P1 (const EXPR *, ep)
{
    EXPR   *ep0 = ep->v.p[0];
    EXPR   *ep1 = ep->v.p[1];
    ADDRESS *ap1, *ap2, *ap3;
    TYP    *tp = ep0->etp;

    switch (tp->type) {
    case bt_bool:
    case bt_uchar:
    case bt_schar:
    case bt_char:
    case bt_charu:
    case bt_ushort:
    case bt_short:
    case bt_int16:
    case bt_uint16:
#ifdef COLDFIRE
	if (is_coldfire ()) {
	    ap1 = g_expr (ep0, (FLAGS) (F_DREG | F_AREG | F_IMMED));
	    ap1 =
		g_extend (ap1, tp,
			  is_unsigned_type (tp) ? tp_ulong : tp_long);
	    ap2 = g_expr (ep1, (FLAGS) (F_AREG | F_DREG));
	    ap2 =
		g_extend (ap2, tp,
			  is_unsigned_type (tp) ? tp_ulong : tp_long);
	    validate (ap1);
	    switch (ap2->mode) {
	    case am_areg:
		/*
		 * sorry, no "TST.L An" on the 68000, but we can move to a data
		 * register if one is free
		 */
		if (is_icon (ep0) && ep0->v.i == 0L && is_free_data ()) {
		    ap3 = data_register ();
		    g_move (IL4, ap2, ap3);
		    /* tst.l ap3 not needed */
		    freeop (ap3);
		    break;
		};
		/*lint -fallthrough */
	    default:
		g_code (op_cmp, IL4, ap1, ap2);
		break;
	    }
	    freeop (ap2);
	    freeop (ap1);
	    return is_unsigned_type (tp);
	}
	/*lint -fallthrough */
#endif /* COLDFIRE */
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap1 = g_expr (ep0, F_ALL);
	if (ap1 == NIL_ADDRESS)
	    return FALSE;

	if (ap1->mode == am_immed) {
	    ap2 = g_expr (ep1, (FLAGS) (F_ALL & ~F_IMMED));
	} else {
	    ap2 = g_expr (ep1, (FLAGS) (F_AREG | F_DREG));
	}
	if (ap2 == NIL_ADDRESS)
	    return FALSE;

	validate (ap1);
	switch (ap2->mode) {
	case am_areg:
	    /*
	     * sorry, no "TST.L An" on the 68000, but we can move to a data
	     * register if one is free
	     */
	    if (is_icon (ep0) && ep0->v.i == 0L && is_free_data ()) {
		ap3 = data_register ();
		g_move (IL4, ap2, ap3);
		/* tst.l ap3 not needed */
		freeop (ap3);
		break;
	    };
	    /*lint -fallthrough */
	default:
	    g_code (op_cmp, (ILEN) tp->size, ap1, ap2);
	    break;
#ifdef CMP_BUG
	case am_direct:
	    if (IandD_option) {
		g_code (op_cmp, (ILEN) tp->size, ap1, ap2);
	    } else {
		ap3 = data_register ();
		g_move ((ILEN) tp->size, ap2, ap3);
		g_code (op_cmp, (ILEN) tp->size, ap1, ap3);
		freeop (ap3);
	    };
	    break;
#endif
	}
	freeop (ap2);
	freeop (ap1);
	return is_unsigned_type (tp);
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	ap1 = g_expr (ep->v.p[1], (FLAGS) (F_VOL | F_COLD | F_DREG));
	ap2 = g_expr (ep->v.p[0], (FLAGS) (F_IMMED | F_MEM | F_COLD));
	ap3 = data_register ();
	g_code (op_move, IL4, mk_low (ap2), ap3);
	validate (ap1);
	g_code (op_sub, IL4, mk_high (ap2), mk_high (ap1));
	g_code (op_subx, IL4, ap3, mk_low (ap1));
	freeop (ap3);
	freeop (ap2);
	freeop (ap1);
	return is_unsigned_type (tp);
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
    case bt_float:
    case bt_double:
    case bt_longdouble:
	if (fpu_option) {
	    ap1 = g_expr (ep0, F_ALL);
	    ap2 = g_expr (ep1, F_FREG);
	    validate (ap1);
	    g_fcode (op_fcmp, (ILEN) tp->size, ap1, ap2);
	    freeop (ap2);
	    freeop (ap1);
	} else {
	    push_rtl_params (ep0, ep1);
	    switch (tp->type) {
	    case bt_float:
		call_library (SUP_SFCMP);
		break;
	    case bt_double:
		call_library (SUP_DFCMP);
		break;
	    case bt_longdouble:
		call_library (SUP_LFCMP);
		break;
	    default:
		CANNOT_REACH_HERE ();
		break;
	    }
	}
	return FALSE;
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
    case bt_float:
    case bt_double:
    case bt_longdouble:
	push_rtl_params (ep0, ep1);
	call_library (SUP_FPCMP);
	return FALSE;
#endif /* FLOAT_MFFP */
    default:
	FATAL ((__FILE__, "g_compare", "illegal type %d", tp->type));
    }
    return FALSE;	/*lint !e527*/	/* unreachable */
}

/*
 * Test the expression and set the condition codes accordingly
 */
static void g_test P1 (const EXPR *, ep)
{
    TYP    *tp = ep->etp;
    ADDRESS *ap = g_expr (ep, (FLAGS) (F_DREG | F_MEM));

    switch (tp->type) {
#ifdef FLOAT_IEEE
    case bt_longdouble:
	{
	    ADDRESS *ap2 = data_register ();

	    g_move (IL4, mk_top (ap), ap2);
	    g_code (op_or, IL4, mk_high (ap), ap2);
	    g_code (op_or, IL4, mk_low (ap), ap2);
	    freeop (ap2);
	}
	break;
    case bt_double:
	{
	    ADDRESS *ap2 = data_register ();

	    g_move (IL4, mk_high (ap), ap2);
	    g_code (op_or, IL4, mk_low (ap), ap2);
	    freeop (ap2);
	}
	break;
#endif /* FLOAT_IEEE */
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	{
	    ADDRESS *ap2 = data_register ();

	    g_move (IL4, mk_high (ap), ap2);
	    g_code (op_or, IL4, mk_low (ap), ap2);
	    freeop (ap2);
	}
	break;
#endif /* LONGLONG_SUPPORT */
    default:
	g_code (op_tst, (ILEN) tp->size, ap, NIL_ADDRESS);
	break;
    }
    freeop (ap);
}

static void check_push P1 (const EXPR *, ep)
{
    switch (ep->nodetype) {
    case en_lt:
    case en_gt:
    case en_le:
    case en_ge:
    case en_eq:
    case en_ne:
	switch (ep->v.p[1]->nodetype) {
	case en_fcall:
	case en_call:
	case en_usercall:
	    temp_inv (NIL_REGUSAGE);
	    break;
	default:
	    break;
	}
	break;
    default:
	break;
    }
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
	VOIDCAST g_compare (ep);
	op = op_beq;
	g_cbranch (op, label);
	break;
    case en_ne:
	VOIDCAST g_compare (ep);
	op = op_bne;
	g_cbranch (op, label);
	break;
    case en_lt:
	op = (OPCODE) (g_compare (ep) ? op_bhi : op_bgt);
	g_cbranch (op, label);
	break;
    case en_le:
	op = (OPCODE) (g_compare (ep) ? op_bhs : op_bge);
	g_cbranch (op, label);
	break;
    case en_gt:
	op = (OPCODE) (g_compare (ep) ? op_blo : op_blt);
	g_cbranch (op, label);
	break;
    case en_ge:
	op = (OPCODE) (g_compare (ep) ? op_bls : op_ble);
	g_cbranch (op, label);
	break;
    case en_land:
	lab0 = nextlabel++;
	check_push (ep->v.p[1]);
	g_falsejp (ep->v.p[0], lab0);
	g_truejp (ep->v.p[1], label);
	g_label (lab0);
	break;
    case en_lor:
	check_push (ep->v.p[1]);
	g_truejp (ep->v.p[0], label);
	g_truejp (ep->v.p[1], label);
	break;
    case en_not:
	g_test (ep->v.p[0]);
	g_cbranch (op_beq, label);
	break;
    case en_test:
	g_test (ep->v.p[0]);
	g_cbranch (op_bne, label);
	break;
    case en_call:		/* library routine which sets the flags */
	freeop (g_expr (ep, F_ALL));
	g_cbranch (op_bne, label);
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
    switch (ep->nodetype) {
    case en_icon:
	if (!ep->v.i) {
	    g_branch (label);
	}
	break;
    case en_eq:
	VOIDCAST g_compare (ep);
	op = op_bne;
	g_cbranch (op, label);
	break;
    case en_ne:
	VOIDCAST g_compare (ep);
	op = op_beq;
	g_cbranch (op, label);
	break;
    case en_lt:
	op = (OPCODE) (g_compare (ep) ? op_bls : op_ble);
	g_cbranch (op, label);
	break;
    case en_le:
	op = (OPCODE) (g_compare (ep) ? op_blo : op_blt);
	g_cbranch (op, label);
	break;
    case en_gt:
	op = (OPCODE) (g_compare (ep) ? op_bhs : op_bge);
	g_cbranch (op, label);
	break;
    case en_ge:
	op = (OPCODE) (g_compare (ep) ? op_bhi : op_bgt);
	g_cbranch (op, label);
	break;
    case en_land:
	check_push (ep->v.p[1]);
	g_falsejp (ep->v.p[0], label);
	g_falsejp (ep->v.p[1], label);
	break;
    case en_lor:
	lab0 = nextlabel++;
	check_push (ep->v.p[1]);
	g_truejp (ep->v.p[0], lab0);
	g_falsejp (ep->v.p[1], label);
	g_label (lab0);
	break;
    case en_not:
	g_test (ep->v.p[0]);
	g_cbranch (op_bne, label);
	break;
    case en_test:
	g_test (ep->v.p[0]);
	g_cbranch (op_beq, label);
	break;
    case en_call:		/* library routine which sets the flags */
	freeop (g_expr (ep, F_ALL));
	g_cbranch (op_beq, label);
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
    ADDRESS *ap, *ap1, *ap2;
    ILEN    ilen = (ILEN) ep->etp->size;
    TYP    *tp = ep->etp;

#ifdef COLDFIRE
    if (is_coldfire ()) {
	tp = is_unsigned_type (tp) ? tp_ulong : tp_long;
    }
#endif /* COLDFIRE */

    initstack ();
    ap = g_expr (ep, (FLAGS) (F_DREG | F_VOL | F_COLD));
    ap1 = address_register ();
    g_code (op_lea, IL0, mk_label (sw->tablab), ap1);
    /*
     * move the interval
     */
    max_caselabel -= min_caselabel;
    switch (tp->size) {
    case IL8:
	if (min_caselabel != (UVAL) 0) {
	    g_immed2 (op_sub, tp_long, (IVAL) min_caselabel, mk_high (ap));
	}
	g_immed (op_cmp, tp_long, (IVAL) max_caselabel, mk_high (ap));
	break;
    default:
	if (min_caselabel != (UVAL) 0) {
	    g_immed2 (op_sub, tp, (IVAL) min_caselabel, ap);
	}
	g_immed (op_cmp, tp, (IVAL) max_caselabel, ap);
    }
    g_cbranch (op_bhi, sw->deflab);
#ifdef RELOC_BUG
    ilen = IL4;
    ap = g_extend (ap, tp, tp_long);
    ap2 = mk_immed (2L);
    ap2 = g_shft (op_asl, ilen, ap2, ap);
    freeop (ap2);
    freeop (ap1);
    ap1 = copy_addr (ap1, am_indx4);
    ap1->sreg = ap->preg;
    ap1->u.offset = mk_const (0L);
    ap2 = address_register ();
    g_move (IL4, ap1, ap2);
    freeop (ap2);
    ap2 = copy_addr (ap2, am_ind);
    g_code (op_jmp, IL0, ap2, NIL_ADDRESS);
#else
    /* jump table contains 2 byte offset of case branches */
    if ((max_caselabel > (UVAL) (65536L / 2L)) || is_coldfire ()) {
	/* will we ever get this many case labels?      */
	ilen = IL4;
	ap = g_extend (ap, tp, tp_long);
    } else {
	ilen = IL2;
	ap = g_extend (ap, tp, tp_short);
    }
    g_add (ilen, ap, ap);
    if (IandD_option) {
	g_add (ilen, ap, ap1);
	freeop (ap1);
	ap2 = copy_addr (ap1, am_ind);
	ap1 = data_register ();
	g_move (ilen, ap2, ap1);
	freeop (ap1);

	ap2 = address_register ();
	g_code (op_lea, IL0, mk_label (sw->beglab), ap2);
	g_add (ilen, ap1, ap2);
	freeop (ap2);
	ap2 = copy_addr (ap2, am_ind);
    } else {
	freeop (ap1);
	ap2 = copy_addr (ap1, (ilen == 2 ? am_indx4 : am_indx2));
	ap2->sreg = ap->preg;
	ap2->u.offset = mk_const ((IVAL) 0L);
	g_add (ilen, ap2, ap1);
	ap2 = copy_addr (ap2, am_ind);
    }
    g_code (op_jmp, IL0, ap2, NIL_ADDRESS);
    if (IandD_option) {
	g_label (sw->beglab);
    }
#endif
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
    IVAL    min, max, val;
    IVAL    min_value = 0L;
    LABEL   label;
    ADDRESS *ap;
    TYP    *tp = ep->etp;

#ifdef COLDFIRE
    if (is_coldfire ()) {
	tp = is_unsigned_type (tp) ? tp_ulong : tp_long;
    }
#endif /* COLDFIRE */

    initstack ();
    ap = g_expr (ep, (FLAGS) (F_DREG | F_VOL | F_COLD));
    if (tp->size == 8L) {
	ADDRESS *ap2;

	freeop (ap);
	ap2 = data_register ();
	g_code (op_move, IL4, mk_high (ap), ap2);
	ap = ap2;
	tp = tp_long;
    }

/* TODO:  proper handling of 8 byte values */

    while (stmt != NIL_STMT) {
	if (stmt->stype != st_default) {
	    for (min = stmt->v2.i; stmt != NIL_STMT; stmt = stmt->s1) {
		max = stmt->v2.i;
		stmt->v2.l = label = nextlabel++;
		if ((stmt->s1 == NIL_STMT) ||
		    (stmt->s1->stype == st_default) ||
		    (stmt->s1 != stmt->v1.s) || (stmt->s1->v2.i != max + 1L)) {
		    stmt = stmt->s1;
		    break;
		}
	    }
	    if ((min - min_value) < 0L) {
		g_immed2 (op_add, tp, min_value - min, ap);
	    } else {
		g_immed2 (op_sub, tp, min - min_value, ap);
	    }
	    min_value = min;
	    val = max - min;
	    switch (val) {
	    case 1L:
		g_cbranch (op_beq, label);
		g_immed2 (op_sub, tp, (IVAL) 1L, ap);
		min_value++;
		/*lint -fallthrough */
	    case 0L:
		g_cbranch (op_beq, label);
		break;
	    default:
#ifdef NLONGLONG
		if ((val > (IVAL) 2147483647L) || (val < (IVAL) -2147483648L))
		    CANNOT_REACH_HERE ();
#endif /* LONGLONG */
		g_immed2 (op_cmp, tp, max - min_value, ap);
		g_cbranch (op_bls, label);
		break;
	    }
	} else {
	    stmt = stmt->s1;
	}
    }
    freeop (ap);
    checkstack ();
}

/*
 *	 Generate the code for setting up any local variables and
 *	 the saving of any registers used.	  This code is actually
 *	 generated at the end of the function when the amount of
 *	 stackspace actually required is known .... the peephole
 *	 optimiser will move it back to the start of the function.
 */
PRIVATE void g_entry P1 (SIZE, frame_size)
{
#ifdef PROBES
    if (probe_option) {
	SIZE    size = frame_size + max_stack_adjust + 32L;
	ADDRESS *ap2, *ap = mk_reg (STACKPTR);

	if (size < 32768L) {
	    ap->mode = am_indx;
	    ap->u.offset = mk_const (-size);
	    g_code (op_tst, IL2, ap, NIL_ADDRESS);
	} else {
	    ap2 = data_register ();
	    ap->mode = am_indx2;
	    ap->sreg = ap->preg;
	    ap->u.offset = mk_const (0L);
	    g_move (IL4, mk_immed (-size), ap2);
	    g_code (op_tst, IL2, ap, NIL_ADDRESS);
	    freeop (ap2);
	}
    }
#endif /* PROBES */

#ifdef STACK_CHECK
    if (stackcheck_option) {
	g_move (IL4, mk_immed ((IVAL) (frame_size + max_stack_adjust)),
		&push);
	call_library (SUP_STACKCHECK);
    }
#endif /* STACK_CHECK */

    if (frame_size < 32768L || target_option >= target_68020) {
	g_code (op_link, IL0, mk_reg (regframe),
		mk_immed ((IVAL) -frame_size));
    } else {
	g_code (op_link, IL0, mk_reg (regframe), mk_immed ((IVAL) -32768L));
	g_sub (IL4, mk_immed ((IVAL) (frame_size - 32768L)),
	       mk_reg (STACKPTR));
    }

    /* Save any registers that were used */
    if (restore_mask != (REGMASK) 0) {
	g_movem (mk_smask (restore_mask), &push);
    }
#ifdef FLOAT_IEEE
    if (restore_fmask != (REGMASK) 0) {
	g_fmovem (mk_smask (restore_fmask), &push);
    }
#endif /* FLOAT_IEEE */

    max_stack_adjust = 0L;
}

/*
 *	 Generate the code for a "return" statement.  This ensures
 *	 that any returned result is loaded into the appropriate
 *	 register.
 */
PRIVATE void g_return P2 (const EXPR *, stmtexp, TYP *, tp)
{
    EXPR   *ep, *ep1;
    ADDRESS *ap;

    initstack ();
    switch (tp->type) {
    case bt_floatcomplex:
    case bt_doublecomplex:
    case bt_longdoublecomplex:
    case bt_struct:
    case bt_union:
	/* assign structure */
	ep = mk_autocon ((SIZE) 8);
	ep = mk_ref (ep, tp_pointer);
	ep1 = mk_ref (ep, tp);
	ep1 = mk_node (en_assign, ep1, copynode (stmtexp), tp);
	VOIDCAST g_expr (ep1, (FLAGS) (F_ALL | F_NOVALUE));

	ap = g_expr (ep, F_ALL);
	g_move (IL4, ap, mk_reg (reg_usage->result->reg[0]));
	freeop (ap);
	break;

#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	ap = g_expr (stmtexp, F_MEM);
#ifdef MOVEMBUG
	g_move (IL4, mk_low (ap), mk_reg (reg_usage->result->reg[0]));
	g_move (IL4, mk_high (ap), mk_reg (reg_usage->result->reg[1]));
#else
	g_movem (ap, mk_rmask ((REGMASK)
			       REGBIT (reg_usage->result->reg[0]) |
			       REGBIT (reg_usage->result->reg[1])));
#endif /* MOVEMBUG */
	break;
#endif /* LONGLONG_SUPPORT */

#ifdef FLOAT_IEEE
    case bt_longdouble:
	if (fpu_option && fpu_return_option) {
	    ap = g_expr (stmtexp, F_FREG);
	    g_fcode (op_fmove, (ILEN) tp->size, ap, mk_reg (FP0));
	} else {
	    ap = g_expr (stmtexp, F_MEM);
#ifdef MOVEMBUG
	    g_move (IL4, mk_low (ap), mk_reg (reg_usage->result->reg[0]));
	    g_move (IL4, mk_high (ap), mk_reg (reg_usage->result->reg[1]));
	    g_move (IL4, mk_top (ap), mk_reg (reg_usage->result->reg[2]));
#else
	    g_movem (ap, mk_rmask ((REGMASK)
				   REGBIT (reg_usage->result->reg[0]) |
				   REGBIT (reg_usage->result->reg[1]) |
				   REGBIT (reg_usage->result->reg[2])));
#endif /* MOVEMBUG */
	}
	freeop (ap);
	break;

    case bt_double:
	if (fpu_option && fpu_return_option) {
	    ap = g_expr (stmtexp, F_FREG);
	    g_fcode (op_fmove, (ILEN) tp->size, ap, mk_reg (FP0));
	} else {
	    ap = g_expr (stmtexp, F_MEM);
#ifdef MOVEMBUG
	    g_move (IL4, mk_low (ap), mk_reg (reg_usage->result->reg[0]));
	    g_move (IL4, mk_high (ap), mk_reg (reg_usage->result->reg[1]));
#else
	    g_movem (ap, mk_rmask ((REGMASK)
				   REGBIT (reg_usage->result->reg[0]) |
				   REGBIT (reg_usage->result->reg[1])));
#endif /* MOVEMBUG */
	}
	freeop (ap);
	break;
    case bt_float:
	if (fpu_option && fpu_return_option) {
	    ap = g_expr (stmtexp, F_FREG);
	    g_fcode (op_fmove, (ILEN) tp->size, ap, mk_reg (FP0));
	    freeop (ap);
	    break;
	}
	/*lint -fallthrough */
#endif /* FLOAT_IEEE */

#ifdef FLOAT_MFFP
    case bt_longdouble:
    case bt_double:
    case bt_float:
#endif /* FLOAT_MFFP */

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
	ap = g_expr (stmtexp, (FLAGS) (F_ALL & ~F_FREG));
	g_move ((ILEN) stmtexp->etp->size, ap,
		mk_reg (reg_usage->result->reg[0]));
	freeop (ap);
	break;
    default:
	FATAL ((__FILE__, "g_return", "illegal type %d", tp->type));
    }
    checkstack ();
}

/*
 *	 Generate the code required at the end of a function to
 *	 restore any used registers and the return instruction itself.
 */
PRIVATE void g_epilogue P0 (void)
{
    ADDRESS *ap;
    SIZE    fstackoffset, stackoffset;

    if (regs_used > 0 || fregs_used > 0) {
	if (is_leaf_function) {
	    ap = &pop;
#ifdef FLOAT_IEEE
	    if (restore_fmask) {
		g_fmovem (ap, mk_rmask (restore_fmask));
	    }
#endif /* FLOAT_IEEE */
	    if (restore_mask) {
		g_movem (ap, mk_rmask (restore_mask));
	    }
	} else {
	    if (lc_auto > lc_auto_max) {
		lc_auto_max = lc_auto;
	    }
	    fstackoffset =
		lc_auto_max + max_scratch + (12L * (SIZE) fregs_used);
	    stackoffset = fstackoffset + (4L * (SIZE) regs_used);
	    if (stackoffset <= 32768L) {
#ifdef FLOAT_IEEE
		if (restore_fmask) {
		    ap =
			mk_indirect (regframe,
				     mk_const ((IVAL) -fstackoffset));
		    g_fmovem (ap, mk_rmask (restore_fmask));
		}
#endif /* FLOAT_IEEE */
		if (restore_mask) {
		    ap =
			mk_indirect (regframe,
				     mk_const ((IVAL) -stackoffset));
		    g_movem (ap, mk_rmask (restore_mask));
		}
	    } else {
		ap = mk_reg (A1);
		g_move (IL4, mk_reg (regframe), ap);
		g_sub (IL4, mk_immed ((IVAL) fstackoffset), ap);
		ap = mk_amode (am_ainc);
		ap->preg = A1;
#ifdef FLOAT_IEEE
		if (restore_fmask) {
		    g_fmovem (ap, mk_rmask (restore_fmask));
		}
#endif /* FLOAT_IEEE */
		if (restore_mask) {
		    g_movem (ap, mk_rmask (restore_mask));
		}
	    }
	}
    }
    g_code (op_unlk, IL0, mk_reg (regframe), NIL_ADDRESS);
    g_code ((interrupt_option ? op_rte : op_rts), IL0, NIL_ADDRESS,
	    NIL_ADDRESS);
}

/*
 * allocate will allocate registers for the expressions that have a high
 * enough desirability.
 */
PRIVATE void g_allocate P1 (CSE *, olist)
{
    CSE    *csp;
    REG     reg;
    REGMASK mask = (REGMASK) 0;
    TYP    *tp;
    EXPR   *ep;
    unsigned value;
    USES    uses;
    REGMASK regmask = (REGMASK) regtemp_option |
	(REGMASK) regunused_option | REGBIT (regframe_option) | REGBIT (A7);

#ifdef FLOAT_IEEE
    REGMASK fmask = (REGMASK) 0;

    fregs_used = 0;
#endif /* FLOAT_IEEE */

    if (datamodel_option != model_absolute) {
	regmask |= REGBIT (regdata_option);
    }
    regs_used = 0;
    for (value = (unsigned) 3; value <= (unsigned) 5; value++) {
	for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	    if (csp->reg != NO_REG) {
		continue;	/* register already allocated */
	    }
	    uses = desire (csp);
	    if (uses < (USES) 3 || (!reg_option && uses < (USES) 5000)) {
		continue;	/* not wanted in register */
	    }
	    ep = csp->exp;
	    tp = ep->etp;
	    if (
		   /*
		    * integer constants may have different types
		    */
		   (csp->exp->nodetype != en_icon)
		   /*
		    * only those items which have sufficient
		    * dereferences are put into address registers
		    */
		   && (csp->duses > (USES) ((unsigned) csp->uses / value))
		   /*
		      * only allow pointer types
		    */
		   && (tp->type == bt_pointer32 || is_func (tp))) {
		reg = find_register (A_REG, regmask);
		if (reg != NO_REG) {
		    csp->reg = reg;
		    regmask |= REGBIT (reg);
		    mask |= REGBIT (reg);
		    regs_used++;
		    continue;
		}
	    }
	    if (
		   /*
		    * integer constants may have different types
		    */
		   (csp->exp->nodetype != en_icon)
		   /*
		    * only those items which have sufficient
		    * dereferences are put into address registers
		    */
		   && (csp->duses >
		       (USES) ((unsigned) csp->uses / (unsigned) 3))
		   /*
		    * the types which are fine in address registers.
		    * Allow only 32-bit integral and signed 16-bit
		    * integral types
		    */
		   && (tp->type == bt_short ||
		       tp->type == bt_int16 ||
		       tp->type == bt_int32 ||
		       tp->type == bt_uint32 ||
		       tp->type == bt_long ||
		       tp->type == bt_ulong ||
		       tp->type == bt_pointer32 || is_func (tp))) {
		reg = find_register (A_REG, regmask);
		if (reg != NO_REG) {
		    csp->reg = reg;
		    regmask |= REGBIT (reg);
		    mask |= REGBIT (reg);
		    regs_used++;
		    continue;
		}
	    }
	    if ((value == (unsigned) 5)
		/*
		 * the types which are fine in data registers.
		 * Allow all types except for 'float', 'double'
		 * and 'long double'
		 */
		&& (tp->type != bt_longdouble)
		&& (tp->type != bt_double)
		&& (tp->type != bt_float)
		&& (tp->type != bt_longlong)
		&& (tp->type != bt_ulonglong)
		) {
		reg = find_register (D_REG, regmask);
		if (reg != NO_REG) {
		    csp->reg = reg;
		    regmask |= REGBIT (reg);
		    mask |= REGBIT (reg);
		    regs_used++;
		    continue;
		}
	    }
#ifdef FLOAT_IEEE
	    if (fpu_option && is_real_floating_type (tp)) {
		reg = find_register (F_REG, regmask);
		if (reg != NO_REG) {
		    csp->reg = reg;
		    regmask |= REGBIT (reg);
		    fmask |= REGBIT (reg);
		    fregs_used++;
		    continue;
		}
	    }
#endif /* FLOAT_IEEE */
	}
    }

    /*
     *               Now take into account which registers must be saved by the
     *               function.
     */
    restore_mask = mask & reglist_to_mask (reg_usage->save);
#ifdef FLOAT_IEEE
    restore_fmask = fmask & reglist_to_mask (reg_usage->save);
#endif /* FLOAT_IEEE */
}


/*
 *	 Go through the common sub-expression tree and check to see if
 *	 any registers must be loaded with a value.
 */

PRIVATE void g_preload P1 (CSE *, olist)
{
    CSE * csp;
    EXPR *  ep;
    ADDRESS *ap, *ap2;

    for     (csp = olist; csp != NIL_CSE; csp = csp->next) {
	if (csp->reg != NO_REG) {	/* see if preload needed */
	    ep = csp->exp;
	    if (!is_lvalue (ep) || (ep->v.p[0]->v.i > 0L)) {
		initstack ();
		ap = g_expr (ep, F_ALL);
		ap2 = mk_reg (csp->reg);
		g_move ((ILEN) ep->etp->size, ap, ap2);
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
    SIZE  default_alignment = AL_DEFAULT;

    if      (lc_auto_max % default_alignment != 0L) {
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
 *	 Some special cases of multiplication are handled by inlining
 *	 the code.	 This routine detects the inlining conditions so
 *	 that the expression will not be replaced by a support routine
 *	 call.
 */
static BOOL is_inlined_multiply P1 (const EXPR *, ep)
{
    if (tst_ushort (ep->v.p[0]) && tst_ushort (ep->v.p[1])) {
	return TRUE;		/* use mulu instruction */
    }
    if      (tst_short (ep->v.p[0]) && tst_short (ep->v.p[1])) {
	return TRUE;		/* use muls instruction */
    }
    if (is_icon (ep->v.p[1])) {
	IVAL    value, i = ep->v.p[1]->v.i;

	if (i < 0L) {
	    i = -i;
	}
	for (value = 1L; value != 0L && value < i; value <<= 1L) {
	    if (pwrof2 (i - value) != -1) {
		return TRUE;
	    }
	    if (pwrof2 (i + value) != -1) {
		return TRUE;
	    }
	}
    }
    return FALSE;
}

/*
 *	 This routine does any code generator specific transformations
 *	 on the expression tree.
 *
 *	 For example it can replace operator nodes with calls to runtime
 *	 routines.	 This allows the global optimiser to perform optimisations
 *	 on such calls which wouldn't be possible if the calls were
 *	 generated in the code generator routines themselves.
 */

PRIVATE EXPR *g_transform P1 (EXPR *, ep)
{
    TYP * tp;

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
    case en_global:
    case en_sym:
    case en_register:
    case en_str:
	break;
    case en_add:
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_SFADD);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_DFADD);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_LFADD);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_SCADD);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_DCADD);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_LCADD);
	    return ep;
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    return transform_binary (ep, SUP_FPADD);
#endif /* FLOAT_MFFP */
	default:
	    return ep;
	}

    case en_sub:
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_SFSUB);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_DFSUB);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_LFSUB);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_SCSUB);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_DCSUB);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_LCSUB);
	    return ep;
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    return transform_binary (ep, SUP_FPSUB);
#endif /* FLOAT_MFFP */
	default:
	    return ep;
	}

    case en_div:
	switch (tp->type) {
	case bt_char:
	case bt_schar:
	case bt_int16:
	case bt_short:
#ifdef COLDFIRE
	    if (is_coldfire ()) {
		ep = transform_binary (ep, SUP_SDIV);
	    }
#endif /* COLDFIRE */
	    return ep;
	case bt_uchar:
	case bt_charu:
	case bt_uint16:
	case bt_ushort:
#ifdef COLDFIRE
	    if (is_coldfire ()) {
		ep = transform_binary (ep, SUP_USDIV);
	    }
#endif /* COLDFIRE */
	    return ep;
	case bt_int32:
	case bt_long:
	    if (target_option < target_68020) {
		ep = transform_binary (ep, SUP_LDIV);
	    }
	    return ep;
	case bt_uint32:
	case bt_ulong:
	    if (target_option < target_68020) {
		ep = transform_binary (ep, SUP_ULDIV);
	    }
	    return ep;
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	    ep = transform_binary (ep, SUP_LLDIV);
	    return ep;
	case bt_ulonglong:
	    ep = transform_binary (ep, SUP_ULLDIV);
	    return ep;
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_SFDIV);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_DFDIV);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_LFDIV);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_SCDIV);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_DCDIV);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_LCDIV);
	    return ep;
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    return transform_binary (ep, SUP_FPDIV);
#endif /* FLOAT_MFFP */
	default:
	    return ep;
	}

    case en_mod:
	switch (tp->type) {
	case bt_int32:
	case bt_long:
	    ep = transform_binary (ep, SUP_LREM);
	    return ep;
	case bt_uint32:
	case bt_ulong:
	    ep = transform_binary (ep, SUP_ULREM);
	    return ep;
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	    ep = transform_binary (ep, SUP_LLREM);
	    return ep;
	case bt_ulonglong:
	    ep = transform_binary (ep, SUP_ULLREM);
	    return ep;
#endif /* LONGLONG_SUPPORT */
	default:
	    return ep;
	}

    case en_mul:
	switch (tp->type) {
	case bt_int32:
	case bt_long:
	    if (is_inlined_multiply (ep)) {
		return ep;	/* special case handled later */
	    }
	    if (target_option < target_68020) {
		ep = transform_binary (ep, SUP_LMUL);
	    }
	    return ep;
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    if (is_inlined_multiply (ep)) {
		return ep;	/* special case handled later */
	    }
	    if (target_option < target_68020) {
		ep = transform_binary (ep, SUP_ULMUL);
	    }
	    return ep;
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	    ep = transform_binary (ep, SUP_LLMUL);
	    return ep;
	case bt_ulonglong:
	    ep = transform_binary (ep, SUP_ULLMUL);
	    return ep;
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_SFMUL);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_DFMUL);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_binary (ep, SUP_LFMUL);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_SCMUL);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_DCMUL);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_LCMUL);
	    return ep;
#endif /* FLOAT_MFFP */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ep = transform_binary (ep, SUP_FPMUL);
	    return ep;
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_asadd:
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASSFADD);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASDFADD);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASLFADD);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_ASSCADD);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_ASDCADD);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_ASLCADD);
	    return ep;
#endif /* FLOAT_MFFP */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ep = transform_binary_ref (ep, SUP_ASFPADD);
	    return ep;
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_assub:
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASSFSUB);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASDFSUB);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASLFSUB);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_ASSCSUB);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_ASDCSUB);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_ASLCSUB);
	    return ep;
#endif /* FLOAT_MFFP */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ep = transform_binary_ref (ep, SUP_ASFPSUB);
	    return ep;
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_asdiv:
	switch (tp->type) {
#ifdef COLDFIRE
	case bt_char:
	case bt_schar:
	    if (is_coldfire ()) {
		ep = transform_assign (ep, SUP_ASCDIV, SUP_LDIV, SUP_ASOPL);
	    }
	    return ep;
	case bt_uchar:
	case bt_charu:
	    if (is_coldfire ()) {
		ep = transform_assign (ep, SUP_ASUCDIV, SUP_LDIV, SUP_ASOPL);
	    }
	    return ep;
	case bt_int16:
	case bt_short:
	    if (is_coldfire ()) {
		ep = transform_assign (ep, SUP_ASSDIV, SUP_LDIV, SUP_ASOPL);
	    }
	    return ep;
	case bt_uint16:
	case bt_ushort:
	    if (is_coldfire ()) {
		ep = transform_assign (ep, SUP_ASUSDIV, SUP_LDIV, SUP_ASOPL);
	    }
	    return ep;
#endif /* COLDFIRE */
	case bt_int32:
	case bt_long:
	    if (target_option < target_68020) {
		ep = transform_assign (ep, SUP_ASLDIV, SUP_LDIV, SUP_ASOPL);
	    }
	    return ep;
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    if (target_option < target_68020) {
		ep = transform_assign (ep, SUP_ASULDIV, SUP_ULDIV, SUP_ASOPL);
	    }
	    return ep;
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	    ep = transform_assign (ep, SUP_ASLLDIV, SUP_LLDIV, SUP_ASOPL);
	    return ep;
	case bt_ulonglong:
	    ep = transform_assign (ep, SUP_ASULLDIV, SUP_ULLDIV, SUP_ASOPL);
	    return ep;
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASSFDIV);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASDFDIV);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASLFDIV);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_ASSCDIV);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_ASDCDIV);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_ASLCDIV);
	    return ep;
#endif /* FLOAT_MFFP */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ep = transform_binary_ref (ep, SUP_ASFPDIV);
	    return ep;
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}
    case en_asmod:
	switch (tp->type) {
	case bt_int32:
	case bt_long:
	    ep = transform_assign (ep, SUP_ASLREM, SUP_LREM, SUP_ASOPL);
	    return ep;
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    ep = transform_assign (ep, SUP_ASULREM, SUP_ULREM, SUP_ASOPL);
	    return ep;
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	    ep = transform_assign (ep, SUP_ASLLREM, SUP_LLREM, SUP_ASOPL);
	    return ep;
	case bt_ulonglong:
	    ep = transform_assign (ep, SUP_ASULLREM, SUP_ULLREM, SUP_ASOPL);
	    return ep;
#endif /* LONGLONG_SUPPORT */
	default:
	    return ep;
	}

    case en_asmul:
	switch (tp->type) {
	case bt_int32:
	case bt_long:
	    if (target_option < target_68020) {
		ep = transform_assign (ep, SUP_ASLMUL, SUP_LMUL, SUP_ASOPL);
	    }
	    return ep;
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    if (target_option < target_68020) {
		ep = transform_assign (ep, SUP_ASULMUL, SUP_ULMUL, SUP_ASOPL);
	    }
	    return ep;
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	    ep = transform_assign (ep, SUP_ASLLMUL, SUP_LLMUL, SUP_ASOPL);
	    return ep;
	case bt_ulonglong:
	    ep = transform_assign (ep, SUP_ASULLMUL, SUP_ULLMUL, SUP_ASOPL);
	    return ep;
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASSFMUL);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASDFMUL);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_binary_ref (ep, SUP_ASLFMUL);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_ASSCMUL);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_ASDCMUL);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_ASLCMUL);
	    return ep;
#endif /* FLOAT_MFFP */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ep = transform_binary_ref (ep, SUP_ASFPMUL);
	    return ep;
#endif /* FLOAT_IEEE */
	default:
	    return ep;
	}

    case en_aslsh:
	switch (tp->type) {
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	    ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_long);
	    ep = transform_assign (ep, SUP_ASLLLSH, SUP_LLLSH, NULL);
	    return ep;
	case bt_ulonglong:
	    ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_long);
	    ep = transform_assign (ep, SUP_ASULLLSH, SUP_ULLLSH, NULL);
	    return ep;
#endif /* LONGLONG_SUPPORT */
	default:
	    return ep;
	}
    case en_asrsh:
	switch (tp->type) {
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
	    ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_long);
	    ep = transform_assign (ep, SUP_ASLLRSH, SUP_LLRSH, NULL);
	    return ep;
	case bt_ulonglong:
	    ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_ulong);
	    ep = transform_assign (ep, SUP_ASULLRSH, SUP_ULLRSH, NULL);
	    return ep;
#endif /* LONGLONG_SUPPORT */
	default:
	    return ep;
	}
#ifdef LONGLONG_SUPPORT
    case en_lsh:
	switch (tp->type) {
	case bt_longlong:
	    ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_long);
	    ep = transform_binary (ep, SUP_LLLSH);
	    return ep;
	case bt_ulonglong:
	    ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_ulong);
	    ep = transform_binary (ep, SUP_ULLLSH);
	    return ep;
	default:
	    switch (ep->v.p[1]->etp->type) {
	    case bt_longlong:
		ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_long);
		break;
	    case bt_ulonglong:
		ep->v.p[1] =
		    mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_ulong);
		break;
	    default:
		break;
	    }
	    return ep;
	}

    case en_rsh:
	switch (tp->type) {
	case bt_longlong:
	    ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_long);
	    ep = transform_binary (ep, SUP_LLRSH);
	    return ep;
	case bt_ulonglong:
	    ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_long);
	    ep = transform_binary (ep, SUP_ULLRSH);
	    return ep;
	default:
	    switch (ep->v.p[1]->etp->type) {
	    case bt_longlong:
		ep->v.p[1] = mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_long);
		break;
	    case bt_ulonglong:
		ep->v.p[1] =
		    mk_node (en_cast, ep->v.p[1], NIL_EXPR, tp_ulong);
		break;
	    default:
		break;
	    }
	    return ep;
	}
#else
    case en_lsh:
    case en_rsh:
	break;
#endif /* LONGLONG_SUPPORT */

    case en_ainc:
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_unary_ref (ep, SUP_SFINC);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_unary_ref (ep, SUP_DFINC);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_unary_ref (ep, SUP_LFINC);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_SCINC);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_DCINC);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_LCINC);
	    return ep;
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ep = transform_unary_ref (ep, SUP_FPINC);
	    return ep;
#endif /* FLOAT_MFFP */
	default:
	    return ep;
	}

    case en_adec:
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_unary_ref (ep, SUP_SFDEC);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_unary_ref (ep, SUP_DFDEC);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_unary_ref (ep, SUP_LFDEC);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_SCINC);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_DCINC);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_LCINC);
	    return ep;
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ep = transform_unary_ref (ep, SUP_FPDEC);
	    return ep;
#endif /* FLOAT_MFFP */
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
	    switch (ep->v.p[0]->etp->type) {
#ifdef FLOAT_IEEE
	    case bt_float:
		return transform_unary (ep, SUP_SFTOL);
	    case bt_double:
		return transform_unary (ep, SUP_DFTOL);
	    case bt_longdouble:
		return transform_unary (ep, SUP_LFTOL);
	    case bt_floatcomplex:
		return transform_unary (ep, SUP_SCTOL);
	    case bt_doublecomplex:
		return transform_unary (ep, SUP_DCTOL);
	    case bt_longdoublecomplex:
		return transform_unary (ep, SUP_LCTOL);
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
	    case bt_float:
	    case bt_double:
	    case bt_longdouble:
		ep = transform_unary (ep, SUP_FPTOL);
		return ep;
#endif /* FLOAT_MFFP */
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
	    switch (ep->v.p[0]->etp->type) {
#ifdef FLOAT_IEEE
	    case bt_float:
		return transform_unary (ep, SUP_SFTOUL);
	    case bt_double:
		return transform_unary (ep, SUP_DFTOUL);
	    case bt_longdouble:
		return transform_unary (ep, SUP_LFTOUL);
	    case bt_floatcomplex:
		return transform_unary (ep, SUP_SCTOUL);
	    case bt_doublecomplex:
		return transform_unary (ep, SUP_DCTOUL);
	    case bt_longdoublecomplex:
		return transform_unary (ep, SUP_LCTOUL);
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
	    case bt_float:
	    case bt_double:
	    case bt_longdouble:
		ep = transform_unary (ep, SUP_FPTOUL);
		return ep;
#endif /* FLOAT_MFFP */
	    default:
		return ep;
	    }
#ifdef LONGLONG_SUPPORT
	case bt_longlong:
#ifdef FLOAT_IEEE
	    switch (ep->v.p[0]->etp->type) {
	    case bt_float:
		return transform_unary (ep, SUP_SFTOLL);
	    case bt_double:
		return transform_unary (ep, SUP_DFTOLL);
	    case bt_longdouble:
		return transform_unary (ep, SUP_LFTOLL);
	    case bt_floatcomplex:
		return transform_unary (ep, SUP_SCTOLL);
	    case bt_doublecomplex:
		return transform_unary (ep, SUP_DCTOLL);
	    case bt_longdoublecomplex:
		return transform_unary (ep, SUP_LCTOLL);
	    default:
		break;
	    }
#endif /* FLOAT_IEEE */
	    break;
	case bt_ulonglong:
#ifdef FLOAT_IEEE
	    switch (ep->v.p[0]->etp->type) {
	    case bt_float:
		return transform_unary (ep, SUP_SFTOULL);
	    case bt_double:
		return transform_unary (ep, SUP_DFTOULL);
	    case bt_longdouble:
		return transform_unary (ep, SUP_LFTOULL);
	    case bt_floatcomplex:
		return transform_unary (ep, SUP_SCTOULL);
	    case bt_doublecomplex:
		return transform_unary (ep, SUP_DCTOULL);
	    case bt_longdoublecomplex:
		return transform_unary (ep, SUP_LCTOULL);
	    default:
		break;
	    }
#endif /* FLOAT_IEEE */
	    break;
#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_SUPPORT
	case bt_float:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_char:
	    case bt_schar:
	    case bt_short:
	    case bt_int16:
		if (!fpu_option) {
		    /*lint -fallthrough*/
		case bt_charu:
		case bt_uchar:
		case bt_ushort:
		case bt_uint16:
		    ep->v.p[0] =
			mk_node (en_cast, ep->v.p[0], NIL_EXPR, tp_long);
		}
		/*lint -fallthrough */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_SFLTOSF);
		}
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		ep = transform_unary (ep, SUP_SFUTOSF);
		return ep;
#ifdef LONGLONG_SUPPORT
	    case bt_longlong:
		ep = transform_unary (ep, SUP_SFLLTOSF);
		return ep;
	    case bt_ulonglong:
		ep = transform_unary (ep, SUP_SFULLTOSF);
		return ep;
#endif /* LONGLONG_SUPPORT */
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
	    case bt_floatcomplex:
		ep = transform_unary (ep, SUP_SCTOSF);
		return ep;
	    case bt_doublecomplex:
		ep = transform_unary (ep, SUP_DCTOSF);
		return ep;
	    case bt_longdoublecomplex:
		ep = transform_unary (ep, SUP_LCTOSF);
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
		if (!fpu_option) {
		    /*lint -fallthrough*/
		case bt_charu:
		case bt_uchar:
		case bt_ushort:
		case bt_uint16:
		    ep->v.p[0] =
			mk_node (en_cast, ep->v.p[0], NIL_EXPR, tp_long);
		}
		/*lint -fallthrough */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_DFLTODF);
		}
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		ep = transform_unary (ep, SUP_DFUTODF);
		return ep;
#ifdef LONGLONG_SUPPORT
	    case bt_longlong:
		ep = transform_unary (ep, SUP_DFLLTODF);
		return ep;
	    case bt_ulonglong:
		ep = transform_unary (ep, SUP_DFULLTODF);
		return ep;
#endif /* LONGLONG_SUPPORT */
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
	    case bt_floatcomplex:
		ep = transform_unary (ep, SUP_SCTODF);
		return ep;
	    case bt_doublecomplex:
		ep = transform_unary (ep, SUP_DCTODF);
		return ep;
	    case bt_longdoublecomplex:
		ep = transform_unary (ep, SUP_LCTODF);
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
		/*lint -fallthrough */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		if (!fpu_option) {
		    ep = transform_unary (ep, SUP_LFLTOLF);
		}
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		ep = transform_unary (ep, SUP_LFUTOLF);
		return ep;
#ifdef LONGLONG_SUPPORT
	    case bt_longlong:
		ep = transform_unary (ep, SUP_LFLLTOLF);
		return ep;
	    case bt_ulonglong:
		ep = transform_unary (ep, SUP_LFULLTOLF);
		return ep;
#endif /* LONGLONG_SUPPORT */
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
	    case bt_floatcomplex:
		ep = transform_unary (ep, SUP_SCTOLF);
		return ep;
	    case bt_doublecomplex:
		ep = transform_unary (ep, SUP_DCTOLF);
		return ep;
	    case bt_longdoublecomplex:
		ep = transform_unary (ep, SUP_LCTOLF);
		return ep;
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
	case bt_floatcomplex:
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
		/*lint -fallthrough */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		ep = transform_unary (ep, SUP_SCLTOSC);
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		ep = transform_unary (ep, SUP_SCUTOSC);
		return ep;
#ifdef LONGLONG_SUPPORT
	    case bt_longlong:
		ep = transform_unary (ep, SUP_SCLLTOSC);
		return ep;
	    case bt_ulonglong:
		ep = transform_unary (ep, SUP_SCULLTOSC);
		return ep;
#endif /* LONGLONG_SUPPORT */
	    case bt_float:
		ep = transform_unary (ep, SUP_SFTOSC);
		return ep;
	    case bt_double:
		ep = transform_unary (ep, SUP_DFTOSC);
		return ep;
	    case bt_longdouble:
		ep = transform_unary (ep, SUP_LFTOSC);
		return ep;
	    case bt_doublecomplex:
		ep = transform_unary (ep, SUP_DCTOSC);
		return ep;
	    case bt_longdoublecomplex:
		ep = transform_unary (ep, SUP_LCTOSC);
		return ep;
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
	case bt_doublecomplex:
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
		/*lint -fallthrough */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		ep = transform_unary (ep, SUP_DCLTODC);
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		ep = transform_unary (ep, SUP_DCUTODC);
		return ep;
#ifdef LONGLONG_SUPPORT
	    case bt_longlong:
		ep = transform_unary (ep, SUP_DCLLTODC);
		return ep;
	    case bt_ulonglong:
		ep = transform_unary (ep, SUP_DCULLTODC);
		return ep;
#endif /* LONGONG_SUPPORT */
	    case bt_float:
		ep = transform_unary (ep, SUP_SFTODC);
		return ep;
	    case bt_double:
		ep = transform_unary (ep, SUP_DFTODC);
		return ep;
	    case bt_longdouble:
		ep = transform_unary (ep, SUP_LFTODC);
		return ep;
	    case bt_floatcomplex:
		ep = transform_unary (ep, SUP_SCTODC);
		return ep;
	    case bt_longdoublecomplex:
		ep = transform_unary (ep, SUP_LCTODC);
		return ep;
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
	case bt_longdoublecomplex:
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
		/*lint -fallthrough */
#ifdef FLOAT_IEEE
	    case bt_int32:
	    case bt_long:
		ep = transform_unary (ep, SUP_LCLTOLC);
		return ep;
	    case bt_uint32:
	    case bt_ulong:
	    case bt_pointer32:
		ep = transform_unary (ep, SUP_LCUTOLC);
		return ep;
#ifdef LONGLONG_SUPPORT
	    case bt_longlong:
		ep = transform_unary (ep, SUP_LCLLTOLC);
		return ep;
	    case bt_ulonglong:
		ep = transform_unary (ep, SUP_LCULLTOLC);
		return ep;
#endif /* LONGLONG_SUPPORT */
	    case bt_float:
		ep = transform_unary (ep, SUP_SFTOLC);
		return ep;
	    case bt_double:
		ep = transform_unary (ep, SUP_DFTOLC);
		return ep;
	    case bt_floatcomplex:
		ep = transform_unary (ep, SUP_SCTOLC);
		return ep;
	    case bt_longdoublecomplex:
		ep = transform_unary (ep, SUP_DCTOLC);
		return ep;
#endif /* FLOAT_IEEE */
	    default:
		return ep;
	    }
#endif /* FLOAT_SUPPORT */
	default:
	    return ep;
	}
	break;

    case en_eq:
    case en_ne:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
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
    case en_fcall:
    case en_call:
    case en_usercall:
    case en_assign:
    case en_ref:
    case en_not:
    case en_test:
    case en_compl:
    case en_deref:
    case en_fieldref:
    case en_literal:
    case en_litval:
	return ep;
    case en_uminus:
	switch (tp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    if (!fpu_option) {
		ep = transform_unary (ep, SUP_SFNEG);
	    }
	    return ep;
	case bt_double:
	    if (!fpu_option) {
		ep = transform_unary (ep, SUP_DFNEG);
	    }
	    return ep;
	case bt_longdouble:
	    if (!fpu_option) {
		ep = transform_unary (ep, SUP_LFNEG);
	    }
	    return ep;
	case bt_floatcomplex:
	    ep = transform_binary (ep, SUP_SCNEG);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_binary (ep, SUP_DCNEG);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_binary (ep, SUP_LCNEG);
	    return ep;
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    ep = transform_unary (ep, SUP_FPNEG);
	    return ep;
#endif /* FLOAT_MFFP */
	default:
	    return ep;
	}

    case en_asmul2:
	switch (ep->v.p[1]->etp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    ep = transform_assign2 (ep, SUP_ASMULSF);
	    return ep;
	case bt_double:
	    ep = transform_assign2 (ep, SUP_ASMULDF);
	    return ep;
	case bt_longdouble:
	    ep = transform_assign2 (ep, SUP_ASMULLF);
	    return ep;
	case bt_floatcomplex:
	    ep = transform_assign2 (ep, SUP_ASMULSC);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_assign2 (ep, SUP_ASMULDC);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_assign2 (ep, SUP_ASMULLC);
	    return ep;
#endif /* FLOAT_IEEE */
	default:
	    CANNOT_REACH_HERE ();
	}
	break;

    case en_asdiv2:
	switch (ep->v.p[1]->etp->type) {
#ifdef FLOAT_IEEE
	case bt_float:
	    ep = transform_assign2 (ep, SUP_ASDIVSF);
	    return ep;
	case bt_double:
	    ep = transform_assign2 (ep, SUP_ASDIVDF);
	    return ep;
	case bt_longdouble:
	    ep = transform_assign2 (ep, SUP_ASDIVLF);
	    return ep;
	case bt_floatcomplex:
	    ep = transform_assign2 (ep, SUP_ASDIVSC);
	    return ep;
	case bt_doublecomplex:
	    ep = transform_assign2 (ep, SUP_ASDIVDC);
	    return ep;
	case bt_longdoublecomplex:
	    ep = transform_assign2 (ep, SUP_ASDIVLC);
	    return ep;
#endif /* FLOAT_IEEE */
	default:
	    CANNOT_REACH_HERE ();
	}
	break;

    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return ep;
}

/*
 *	 This routine is called after the global optimizer has done it's
 *	 work re-organizing the expression tree.  This allows a code
 *	 generator to make code generator specific changes to the expression
 *	 tree which will result in better code generation.
 */

PRIVATE EXPR *g_order P1 (EXPR *, ep)
{
    EXPR *ep0 = ep->v.p[0];
    EXPR *  ep1 = ep->v.p[1];

    switch (ep->nodetype) {
    case en_mul:
	if (is_icon (ep0)) {
	    swap_nodes (ep);
	}
	break;

    case en_lt:
    case en_gt:
    case en_le:
    case en_ge:
    case en_eq:
    case en_ne:
	switch (ep0->nodetype) {
	case en_register:
	case en_icon:
	    swap_nodes (ep);
	    ep0 = ep->v.p[0];
	    ep1 = ep->v.p[1];
	    break;
	default:
	    break;
	}
	switch (ep1->nodetype) {
	case en_register:
	    break;
	case en_icon:
	    if ((ep1->v.i == 0L) || !is_byte (ep1) ||
		(ep0->etp->size < 4L) || (ep0->etp->size >= 8L)) {
		swap_nodes (ep);
	    }
	    break;
	default:
	    swap_nodes (ep);
	    break;
	}
	break;
    default:
	break;
    }
    return ep;
}

/*
 *	 This routine is called when the compiler is initializing, i.e.
 *	 before it even starts scanning tokens.
 */
PRIVATE void g_initialize P0 (void)
{
    REG   reg;
    unsigned long temp;

    if (!optimize_option) {
	stackopt_option = 0;
    }
    if (stackopt_option) {
	is_parameter = FALSE;
    }
    volatile_found = lang_option == LANG_KANDR;
    regframe = (REG) regframe_option;
    regdata = (REG) regdata_option;
    for (reg = D0, temp = regtemp_option; temp; reg++, (temp >>= 1)) {
	if (temp & (unsigned long) 1) {
	    set_temporary_register (reg);
	}
    }
}

/*
 *	 This routine is called when the compiler is closing down.
 */
PRIVATE void g_terminate P0 (void)
{
}

/*
 *	 Returns the current register usage.
 */
PRIVATE REGUSAGE *g_regusage P1 (TYP *, tp)
{
    UNUSEDARG (tp);
    return reg_usage;
}

/*
 *	 Command line options specific to the 68K code generator
 */
static OPTENUM model_opts[] =
{
    { (const CHAR *) "absolute", (int) model_absolute },
    { (const CHAR *) "small", (int) model_small },
    { (const CHAR *) "large", (int) model_large },
    { (const CHAR *) NULL, 0 }
};

static OPTENUM target_opts[] =
{
    { (const CHAR *) "68000", (int) target_68000 },
    { (const CHAR *) "68010", (int) target_68010 },
    { (const CHAR *) "68020", (int) target_68020 },
    { (const CHAR *) "68030", (int) target_68030 },
    { (const CHAR *) "68040", (int) target_68040 },
#ifdef COLDFIRE
    { (const CHAR *) "coldfire", (int) target_coldfire },
#endif /* COLDFIRE */
    { (const CHAR *) NULL, 0 }
};

static OPTSET reg68kset[] =
{
    { (const CHAR *) "d0", MEMBER (D0) },
    { (const CHAR *) "d1", MEMBER (D1) },
    { (const CHAR *) "d2", MEMBER (D2) },
    { (const CHAR *) "d3", MEMBER (D3) },
    { (const CHAR *) "d4", MEMBER (D4) },
    { (const CHAR *) "d5", MEMBER (D5) },
    { (const CHAR *) "d6", MEMBER (D6) },
    { (const CHAR *) "d7", MEMBER (D7) },
    { (const CHAR *) "a0", MEMBER (A0) },
    { (const CHAR *) "a1", MEMBER (A1) },
    { (const CHAR *) "a2", MEMBER (A2) },
    { (const CHAR *) "a3", MEMBER (A3) },
    { (const CHAR *) "a4", MEMBER (A4) },
    { (const CHAR *) "a5", MEMBER (A5) },
    { (const CHAR *) "a6", MEMBER (A6) },
    { (const CHAR *) "a7", MEMBER (A7) },
#ifdef FLOAT_IEEE
    { (const CHAR *) "fp0", MEMBER (FP0) },
    { (const CHAR *) "fp1", MEMBER (FP1) },
    { (const CHAR *) "fp2", MEMBER (FP2) },
    { (const CHAR *) "fp3", MEMBER (FP3) },
    { (const CHAR *) "fp4", MEMBER (FP4) },
    { (const CHAR *) "fp5", MEMBER (FP5) },
    { (const CHAR *) "fp6", MEMBER (FP6) },
    { (const CHAR *) "fp7", MEMBER (FP7) },
#endif /* FLOAT_IEEE */
    { (const CHAR *) NULL, EMPTY_SET }
};

static OPTENUM areg68k[] =
{
    { (const CHAR *) "a0", (int) A0 },
    { (const CHAR *) "a1", (int) A1 },
    { (const CHAR *) "a2", (int) A2 },
    { (const CHAR *) "a3", (int) A3 },
    { (const CHAR *) "a4", (int) A4 },
    { (const CHAR *) "a5", (int) A5 },
    { (const CHAR *) "a6", (int) A6 },
    { (const CHAR *) "a7", (int) A7 },
    { (const CHAR *) NULL, 0 }
};

static OPTENUM yesnoopts[] =
{
    { (const CHAR *) "yes", 1 },
    { (const CHAR *) "no", 0 },
    { (const CHAR *) NULL, 0 }
};

static OPTSET peepset[] =
{
    { (const CHAR *) "none", EMPTY_SET },
    { (const CHAR *) "instruction", MEMBER (PEEP_INSTRUCTION) },
    { (const CHAR *) "jumps", MEMBER (PEEP_JUMPS) },
#ifdef PEEP_FLOW
    { (const CHAR *) "flow", MEMBER (PEEP_FLOW) },
#endif /* PEEP_FLOW */
    { (const CHAR *) "all", ~EMPTY_SET },
    { (const CHAR *) NULL, EMPTY_SET }
};

static OPTENUM stackoptions[] =
{
    { (const CHAR *) "safest", OPT_SAFE },
    { (const CHAR *) "minimum", OPT_MINIMUM },
    { (const CHAR *) "average", OPT_AVERAGE },
    { (const CHAR *) "maximum", OPT_MAXIMUM },
    { (const CHAR *) NULL, 0 }
};

#ifdef MULTIPLE_PROCESSORS
#define MC68K_FUNCS ((void *)&mc68k_funcs)
#else
#define MC68K_FUNCS ((void *)NULL)
#endif /* MULTIPLE_PROCESSORS */

static OPTION opts[] =
{
    {
	(const CHAR *) "target=", enumeration_option,
	{&target_option},		/*lint !e708*/	/* union initialization */
	{&target_opts[0]}		/*lint !e708*/	/* union initialization */
    },
#ifdef ASMCALL
    {
	(const CHAR *) "asmcall=", string_option,
	{&asmcall_option},			/*lint !e708*/	/* union initialization */
	{NULL}				/*lint !e708*/	/* union initialization */
    },
#endif /* ASMCALL */
    {
	(const CHAR *) "codemodel=", enumeration_option,
	{&codemodel_option},		/*lint !e708*/	/* union initialization */
	{&model_opts[0]}		/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "datamodel=", enumeration_option,
	{&datamodel_option},		/*lint !e708*/	/* union initialization */
	{&model_opts[0]}		/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "fpu=", enumeration_option,
	{&fpu_option},			/*lint !e708*/	/* union initialization */
	{&yesnoopts[0]}			/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "fpureturn=", enumeration_option,
	{&fpu_return_option},		/*lint !e708*/	/* union initialization */
	{&yesnoopts[0]}			/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "interrupt=", enumeration_option,
	{&interrupt_option},		/*lint !e708*/	/* union initialization */
	{&yesnoopts[0]}			/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "peep=", set_option,
	{&peep_option},			/*lint !e708*/	/* union initialization */
	{(void *)&peepset[0]}		/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "prefix=", string_option,
	{(void *) &external_prefix},	/*lint !e708*/	/* union initialization */
	{NULL}				/*lint !e708*/	/* union initialization */
    },
#ifdef PROBES
    {
	(const CHAR *) "probe=", enumeration_option,
	{&probe_option},		/*lint !e708*/	/* union initialization */
	{&yesnoopts[0]}			/*lint !e708*/	/* union initialization */
    },
#endif /* PROBES */
    {
	(const CHAR *) "reg=", enumeration_option,
	{&reg_option},			/*lint !e708*/	/* union initialization */
	{&yesnoopts[0]}			/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "regframe=", enumeration_option,
	{&regframe_option},		/*lint !e708*/	/* union initialization */
	{&areg68k[0]}			/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "regdata=", enumeration_option,
	{&regdata_option},		/*lint !e708*/	/* union initialization */
	{&areg68k[0]}			/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "regreturn=", list_option,
	{&regreturn_list},		/*lint !e708*/	/* union initialization */
	{NULL}				/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "regtemp=", set_option,
	{&regtemp_option},		/*lint !e708*/	/* union initialization */
	{(void *)&reg68kset[0]}	/*lint !e708*/	/* union initialization */
    },
    {
	(const CHAR *) "regunused=", set_option,
	{&regunused_option},		/*lint !e708*/	/* union initialization */
	{(void *)&reg68kset[0]}	/*lint !e708*/	/* union initialization */
    },
#ifdef STACK_CHECK
    {
	(const CHAR *) "stackcheck=", enumeration_option,
	{&stackcheck_option},		/*lint !e708*/	/* union initialization */
	{&yesnoopts[0]}			/*lint !e708*/	/* union initialization */
    },
#endif /* STACK_CHECK */
    {
	(const CHAR *) "stackopt=", enumeration_option,
	{&stackopt_option},		/*lint !e708*/	/* union initialization */
	{&stackoptions[0]}		/*lint !e708*/	/* union initialization */
    },
#ifdef TRANSLATE
    {
	(const CHAR *) "trans=", enumeration_option,
	{&trans_option},		/*lint !e708*/	/* union initialization */
	{&yesnoopts[0]}			/*lint !e708*/	/* union initialization */
    },
#endif /* TRANSLATE */
#ifdef MULTIPLE_ASSEMBLERS
#ifdef TARGET_ACK
    {
	(const CHAR *) "ack68k", chip_option,
	{&ack68k_funcs},		/*lint !e708*/	/* union initialization */
	{MC68K_FUNCS}			/*lint !e708*/	/* union initialization */
    },
#endif /* TARGET_ACK */
#ifdef TARGET_CPM
    {
	(const CHAR *) "cpm68k", chip_option,
	{&cpm68k_funcs},		/*lint !e708*/	/* union initialization */
	{MC68K_FUNCS}			/*lint !e708*/	/* union initialization */
    },
#endif /* TARGET_CPM */
#ifdef TARGET_GAS
    {
	(const CHAR *) "gas68k", chip_option,
	{&gas68k_funcs},		/*lint !e708*/	/* union initialization */
	{MC68K_FUNCS}			/*lint !e708*/	/* union initialization */
    },
#endif /* TARGET_GAS */
#ifdef TARGET_QMAC
    {
	(const CHAR *) "qmc68k", chip_option,
	{&qmac68k_funcs},		/*lint !e708*/	/* union initialization */
	{MC68K_FUNCS}			/*lint !e708*/	/* union initialization */
    },
#endif /* TARGET_QMAC */
#else
#ifdef TARGET_ACK
    {
	(const CHAR *) "ack68k", chip_option,
	{NULL},				/*lint !e708*/	/* union initialization */
	{MC68K_FUNCS}			/*lint !e708*/	/* union initialization */
    },
#endif /* TARGET_ACK */
#ifdef TARGET_CPM
    {
	(const CHAR *) "cpm68k", chip_option,
	{NULL},				/*lint !e708*/	/* union initialization */
	{MC68K_FUNCS}			/*lint !e708*/	/* union initialization */
    },
#endif /* TARGET_CPM */
#ifdef TARGET_GAS
    {
	(const CHAR *) "gas68k", chip_option,
	{NULL},				/*lint !e708*/	/* union initialization */
	{MC68K_FUNCS}			/*lint !e708*/	/* union initialization */
    },
#endif /* TARGET_GAS */
#ifdef TARGET_QMAC
    {
	(const CHAR *) "qmc68k", chip_option,
	{NULL},				/*lint !e708*/	/* union initialization */
	{MC68K_FUNCS}			/*lint !e708*/	/* union initialization */
    },
#endif /* TARGET_QMAC */
#endif /* MULTIPLE_ASSEMBLERS */
    {
	(const CHAR *) NULL, NULL,
	{NULL},				/*lint !e708*/	/* union initialization */
	{NULL}				/*lint !e708*/	/* union initialization */
    }
};

OPTIONS opts68k =
{
    (const CHAR *) "68k ", opts
};

#ifdef MULTIPLE_PROCESSORS
struct genfuncs mc68k_funcs =
{
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
#endif /*DEBUGOPT */
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
    &alignments_68000[0]
};

#undef GEN_MODULE
#endif /* MULTIPLE_PROCESSORS */
#endif /* MC680X0 */
