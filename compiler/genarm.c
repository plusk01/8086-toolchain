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

/******************************************************************************
 *
 * this module contains all of the code generation routines for evaluating
 * expressions and conditions.
 *
 *****************************************************************************/

#include "config.h"

#ifdef ARM

#define GEN_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genarm.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define AL_DEFAULT	(g_alignments[bt_ellipsis])

/*********************************************** Static Function Definitions */

static ADDRESS *mk_legal P_ ((ADDRESS *, FLAGS));
static BOOL g_compare P_ ((const EXPR *));
static ADDRESS *g_cast P_ ((ADDRESS *, TYP *, TYP *, FLAGS));
static ADDRESS *g_expr P_ ((const EXPR *, FLAGS));
static void g_falsejp P_ ((const EXPR *, LABEL));
static void g_truejp P_ ((const EXPR *, LABEL));

#ifdef MULTIPLE_PROCESSORS
PRIVATE void g_expression P_ ((const EXPR *));
PRIVATE void g_jfalse P_ ((const EXPR *, LABEL));
PRIVATE void g_jtrue P_ ((const EXPR *, LABEL));
PRIVATE void g_stack P_ ((SIZE));
PRIVATE void g_switch_table P_ ((const EXPR *, const SWITCH *, UVAL, UVAL));
PRIVATE void g_switch_compare P_ ((const EXPR *, STMT *));
PRIVATE void g_entry P_ ((SIZE));
PRIVATE void g_return P_ ((const EXPR *, TYP *));
PRIVATE void g_epilogue P_ ((void));
PRIVATE void g_allocate P_ ((CSE *));
PRIVATE void g_flush P_ ((SYM *));
PRIVATE void g_auto_align P_ ((void));
PRIVATE BOOL g_is_bigendian P_ ((void));
PRIVATE BOOL g_is_ascending_stack P_ ((void));
PRIVATE void g_initialize P_ ((void));

#endif /* MULTIPLE_PROCESSORS */

/********************************************************** Static Variables */

static unsigned peep_option = PEEP_ALL;	/* peephole optimisations */
static int stackopt_option = 1L;	/* Use lazy stack optimisation */

static BOOL regs_used = 0;	/* number of register variables allocated */
static SIZE max_stack_adjust = 0L;	/* largest amount stack is altered */
static REG frameptr = FRAMEPTR;
static REGMASK restore_mask;	/* list of registers used by function */

/*
 *	 The following tables specify the alignment requirements of the
 *	 basic types depending on the processor type.
 */
static SIZE alignments_arm[] = {
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
    4L,				/* bt_int32     */
    4L,				/* bt_uint32    */
    4L,				/* bt_long              */
    4L,				/* bt_ulong     */
    4L,				/* bt_longlong  */
    4L,				/* bt_ulonglong */
    4L,				/* bt_float     */
    4L,				/* bt_double    */
    4L,				/* bt_longdouble */
    4L,				/* bt_floatcomplex */
    4L,				/* bt_doublecomplex */
    4L,				/* bt_longdoublecomplex */
    4L,				/* bt_float     */
    4L,				/* bt_double    */
    4L,				/* bt_longdouble */
    4L,				/* bt_pointer16 */
    4L,				/* bt_pointer32 */
    4L,				/* bt_struct    */
    4L,				/* bt_union     */
    4L,				/* bt_func              */
    4L,				/* bt_bitfield  */
    4L,				/* bt_ubitfield */
    1L,				/* bt_bbitfield */
    4L				/* bt_ellipsis - used for alignment suitable for all types */
};

#ifndef MULTIPLE_PROCESSORS
PRIVATE SIZE *g_alignments = &alignments_arm[0];

#endif /* MULTIPLE_PROCESSORS */

/*****************************************************************************/

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

    if (ap == NIL_ADDRESS) {
	FATAL ((__FILE__, "copy_addr", "ap == 0"));
    }
    newap = (ADDRESS *) xalloc (sizeof (ADDRESS));

    *newap = *ap;
    newap->mode = mode;
    return newap;
}

/*
 * make a ep to reference an immediate value i.
 */
static ADDRESS *mk_immed P1 (IVAL, i)
{
    ADDRESS *ap;

    ap = mk_amode (am_immed);
    ap->offset = mk_const (i);
    return ap;
}

/*
 * construct a reference node for an internal label number.
 */
static ADDRESS *mk_label P1 (LABEL, lab)
{
    ADDRESS *ap;

    ap = mk_amode (am_direct);
    ap->offset = mk_lcon (lab);
    return ap;
}

/*
 * make a node to reference a line number.
 */
static ADDRESS *mk_line P1 (LINE, i)
{
    ADDRESS *ap;

    ap = mk_amode (am_line);
    ap->offset = mk_const ((IVAL) i);
    return ap;
}

#if 0
/*
 * generate a direct reference to a string label.
 */
ADDRESS *mk_strlab P1 (CHAR *, s)
{
    ADDRESS *ap;

    ap = mk_amode (am_direct);
    ap->offset = mk_node (en_nacon, NIL_EXPR, NIL_EXPR, tp_void);
    ap->offset->v.sp = s;
    return ap;
}
#endif

/*
 * make an address reference to a register.
 */
ADDRESS *mk_reg P1 (REG, r)
{
    ADDRESS *ap;

    ap = mk_amode (am_reg);
    ap->preg = r;
    return ap;
}

/*
 * make an address reference to a register mask list.
 */
static ADDRESS *mk_mask P1 (REGMASK, mask)
{
    ADDRESS *ap;

    ap = mk_amode (am_mask);
    ap->offset = mk_const ((IVAL) mask);
    return ap;
}

static ADDRESS *mk_direct P1 (EXPR *, ep)
{
    ADDRESS *ap;

    ap = mk_amode (am_direct);
    ap->offset = ep;
    return ap;
}

static ADDRESS *mk_address P3 (AMODE, mode, REG, reg, SIZE, off)
{
    ADDRESS *ap;

    ap = mk_amode (mode);
    ap->preg = reg;
    ap->offset = mk_const (off);
    return ap;
}

static ADDRESS *mk_legal P2 (ADDRESS *, ap, FLAGS, flags)
{
    ADDRESS *ap2;

    if (flags & F_NOVALUE) {
	freeop (ap);
	return NIL_ADDRESS;
    }
    switch (ap->mode) {
    case am_immed:
	if (flags & F_IMMED) {
	    return ap;
	}
	break;
    case am_reg:
	if (flags & F_REG) {
	    return ap;
	}
	break;
    default:
	break;
    }
    if (flags & F_REG) {
	freeop (ap);
	ap2 = data_register ();
	g_code (op_mov, cc_al, ap2, ap, NIL_ADDRESS);
	return ap2;
    }
    FATAL ((__FILE__, "mk_legal", "mode = %d, flags = %d", ap->mode, flags));
    return NIL_ADDRESS;
}

/*
 * add a compiler generated label to the peep list.
 */
PRIVATE void g_label P1 (LABEL, labno)
{
    sync_stack ();
    g_code (op_label, cc_al, mk_label (labno), NIL_ADDRESS, NIL_ADDRESS);
}

#ifdef DEBUGOPT
/*
 * add a source line number to the peep list.
 */
PRIVATE void g_line P2 (LINE, line, const CHAR *, str)
{
    (void) str;
    g_code (op_line, cc_al, mk_line (line), NIL_ADDRESS, NIL_ADDRESS);
}

#endif /*DEBUGOPT */

/*
 * add a conditional branch instruction to the peep list.
 */
static void g_cbranch P2 (CONDITION, cc, LABEL, labno)
{
    sync_stack ();
    g_code (op_b, cc, mk_label (labno), NIL_ADDRESS, NIL_ADDRESS);
}

/*
 * add a branch instruction to the peep list.
 */
PRIVATE void g_branch P1 (LABEL, labno)
{
    g_code (op_b, cc_al, mk_label (labno), NIL_ADDRESS, NIL_ADDRESS);
}

/*
 * adjust the stack by "bytes" bytes.
 */
PRIVATE void g_stack P1 (SIZE, bytes)
{
    if (bytes != 0L) {
	/* adjust stack pointer */
	stack_offset -= bytes;
	if (max_stack_adjust < bytes) {
	    max_stack_adjust = bytes;
	}
    }
}

/*
 * generate the code to access an object.
 */
static ADDRESS *g_deref P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1;

    (void) flags;
    switch (ep->nodetype) {
    case en_autocon:
	ap1 = mk_amode (am_pre);
	ap1->preg = frameptr;
	ap1->offset = mk_const (ep->v.i);
	return ap1;
    default:
	ap1 = g_expr (ep, (FLAGS) (F_REG | F_IMMED));
	if (ap1->mode == am_immed) {
	    return copy_addr (ap1, am_pre);
	}
	return copy_addr (ap1, am_direct);
    }
}

/*
 * generate the code for a unary minus
 */
static ADDRESS *g_uminus P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;

    switch (ep->etp->type) {
    case bt_char:
    case bt_schar:
    case bt_uchar:
    case bt_charu:
    case bt_short:
    case bt_int16:
    case bt_ushort:
    case bt_uint16:
    case bt_long:
    case bt_ulong:
    case bt_int32:
    case bt_uint32:
    case bt_pointer32:
	ap2 = g_expr (ep->v.p[0], F_REG);
	freeop (ap2);
	ap1 = data_register ();
	g_code (op_rsb, cc_al, ap1, ap2, mk_immed (0L));
	return mk_legal (ap1, flags);
    default:
	FATAL ((__FILE__, "g_uminus", "type = %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate the code for a negate
 */
static ADDRESS *g_negate P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;

    switch (ep->etp->type) {
    case bt_char:
    case bt_schar:
    case bt_uchar:
    case bt_charu:
    case bt_short:
    case bt_int16:
    case bt_ushort:
    case bt_uint16:
    case bt_long:
    case bt_ulong:
    case bt_int32:
    case bt_uint32:
    case bt_pointer32:
	ap2 = g_expr (ep->v.p[0], F_REG);
	freeop (ap2);
	ap1 = data_register ();
	g_code (op_mvn, cc_al, ap1, ap2, NIL_ADDRESS);
	return mk_legal (ap1, flags);
    default:
	FATAL ((__FILE__, "g_negate", "type = %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate code to evaluate a and/or/xor node and return the addressing
 * node of the result
 */
static ADDRESS *g_logic P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;

    switch (ep->etp->type) {
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap1 = g_expr (ep->v.p[0], F_REG);
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_REG | F_IMMED));
	validate (ap1);
	freeop (ap2);
	g_code (op, cc_al, ap1, ap1, ap2);
	return mk_legal (ap1, flags);
    default:
	FATAL ((__FILE__, "g_logic", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

static ADDRESS *g_aslogic P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    (void) flags;
    (void) op;
    FATAL ((__FILE__, "g_aslogic", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}

/*
 * generate code to evaluate an add/subtract node and return the addressing
 * node of the result
 */
static ADDRESS *g_add P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2;

    switch (ep->etp->type) {
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap1 = g_expr (ep->v.p[0], F_REG);
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_REG | F_IMMED));
	validate (ap1);
	freeop (ap2);
	g_code (op, cc_al, ap1, ap1, ap2);
	return mk_legal (ap1, flags);
    default:
	FATAL ((__FILE__, "g_add", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

static ADDRESS *g_asadd P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    (void) flags;
    (void) op;
    FATAL ((__FILE__, "g_asadd", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}

static ADDRESS *g_mul P2 (const EXPR *, ep, FLAGS, flags)
{
    (void) flags;
    (void) ep;
    FATAL ((__FILE__, "g_mul", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}

static ADDRESS *g_asmul P2 (const EXPR *, ep, FLAGS, flags)
{
    (void) flags;
    FATAL ((__FILE__, "g_asmul", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}


static ADDRESS *g_div P2 (const EXPR *, ep, FLAGS, flags)
{
    (void) flags;
    FATAL ((__FILE__, "g_div", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}

static ADDRESS *g_asdiv P2 (const EXPR *, ep, FLAGS, flags)
{
    (void) flags;
    FATAL ((__FILE__, "g_asdiv", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}

static ADDRESS *g_mod P2 (const EXPR *, ep, FLAGS, flags)
{
    (void) flags;
    FATAL ((__FILE__, "g_mod", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}

static ADDRESS *g_asmod P2 (const EXPR *, ep, FLAGS, flags)
{
    (void) flags;
    FATAL ((__FILE__, "g_asmod", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}

static ADDRESS *g_lshift P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2, *ap3;

    switch (ep->etp->type) {
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	ap1 = g_expr (ep->v.p[0], F_REG);
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_REG | F_IMMED));
	validate (ap1);
	freeop (ap2);
	if (ap2->mode == am_immed) {
	    ap3 = copy_addr (ap1, am_lsl);
	    ap3->offset = ap2->offset;
	} else {
	    ap3 = copy_addr (ap1, am_lslr);
	    ap3->sreg = ap2->preg;
	}
	g_code (op_mov, cc_al, ap1, ap3, NIL_ADDRESS);
	return mk_legal (ap1, flags);
    default:
	FATAL ((__FILE__, "g_lshift", "typ = %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

#if 0
static ADDRESS *g_aslshift P2 (const EXPR *, ep, FLAGS, flags)
{
    FATAL ((__FILE__, "g_aslshift", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}
#endif

static ADDRESS *g_rshift P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2, *ap3;
    BOOL    sign = FALSE;

    switch (ep->etp->type) {
    case bt_int32:
    case bt_long:
	sign = TRUE;
	/*FALLTHRU */
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	ap1 = g_expr (ep->v.p[0], F_REG);
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_REG | F_IMMED));
	validate (ap1);
	freeop (ap2);
	if (ap2->mode == am_immed) {
	    ap3 = copy_addr (ap1, sign ? am_asr : am_lsr);
	    ap3->offset = ap2->offset;
	} else {
	    ap3 = copy_addr (ap1, sign ? am_asrr : am_lsrr);
	    ap3->sreg = ap2->preg;
	}
	g_code (op_mov, cc_al, ap1, ap3, NIL_ADDRESS);
	return mk_legal (ap1, flags);
    default:
	FATAL ((__FILE__, "g_rshift", "typ = %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

#if 0
static ADDRESS *g_asrshift P2 (const EXPR *, ep, FLAGS, flags)
{
    FATAL ((__FILE__, "g_asrshift", "typ = %d", ep->etp->type));
    return NIL_ADDRESS;
}
#endif

/*
 * Generate the code to extend the value in the register described by
 * 'ap' from type 'tp1' to the type 'tp2'.
 */
static ADDRESS *g_extend P3 (ADDRESS *, ap, TYP *, tp1, TYP *, tp2)
{
    ADDRESS *ap1;
    AMODE   mode;

    assert (ap->mode == am_reg);
    switch (tp2->type) {
    case bt_int16:
    case bt_uint16:
    case bt_short:
    case bt_ushort:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	switch (tp1->type) {
	case bt_char:
	case bt_schar:
	case bt_short:
	case bt_int16:
	    mode = am_asr;
	    ap1 = mk_address (am_lsl, ap->preg, (4L - tp1->size) * 8L);
	    g_code (op_mov, cc_al, ap, ap1, NIL_ADDRESS);
	    ap1 = mk_address (mode, ap->preg, (4L - tp1->size) * 8L);
	    g_code (op_mov, cc_al, ap, ap1, NIL_ADDRESS);
	    return ap;
	case bt_uchar:
	case bt_charu:
	case bt_ushort:
	case bt_uint16:
	    mode = am_lsr;
	    ap1 = mk_address (am_lsl, ap->preg, (4L - tp1->size) * 8L);
	    g_code (op_mov, cc_al, ap, ap1, NIL_ADDRESS);
	    ap1 = mk_address (mode, ap->preg, (4L - tp1->size) * 8L);
	    g_code (op_mov, cc_al, ap, ap1, NIL_ADDRESS);
	    return ap;
	case bt_int32:
	case bt_long:
	case bt_uint32:
	case bt_ulong:
	case bt_pointer32:
	    return ap;
	default:
	    break;
	}
	break;
    default:
	break;
    }
    FATAL (
	   (__FILE__, "g_extend", "typ1 = %d, typ2 = %d", tp1->type,
	    tp2->type));
    return NIL_ADDRESS;
}

static ADDRESS *g_cast P4 (ADDRESS *, ap, TYP *, tp1, TYP *, tp2, FLAGS,
			   flags)
{
    if (flags & F_NOVALUE) {
	freeop (ap);
	return NIL_ADDRESS;
    }
    switch (tp2->type) {
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
	ap = mk_legal (ap, (FLAGS) (F_REG | F_VOL));
	ap = g_extend (ap, tp1, tp2);
	return mk_legal (ap, flags);
    case bt_short:
    case bt_int16:
    case bt_ushort:
    case bt_uint16:
	ap = mk_legal (ap, (FLAGS) (F_REG | F_VOL));
	ap = g_extend (ap, tp1, tp2);
	return mk_legal (ap, flags);
    case bt_int32:
    case bt_long:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
    case bt_func:
	ap = mk_legal (ap, (FLAGS) (F_REG | F_VOL));
	ap = g_extend (ap, tp1, tp2);
	return mk_legal (ap, flags);
    default:
	FATAL (
	       (__FILE__, "g_cast", "typ1 = %d, typ2 = %d", tp1->type,
		tp2->type));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate code for the assignment expression
 */
static ADDRESS *g_assign P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1;
    ADDRESS *ap2;

    switch (ep->v.p[0]->nodetype) {
    case en_register:
	ap1 = g_expr (ep->v.p[0], F_ALL);
	ap2 = g_expr (ep->v.p[1], F_REG);
	validate (ap1);
	freeop (ap2);
	return mk_legal (ap1, flags);
    case en_ref:
	ap1 = g_expr (ep->v.p[0], F_ALL);
	ap2 = g_expr (ep->v.p[1], F_REG);
	validate (ap1);
	g_code (op_str, cc_al, ap1, ap2, NIL_ADDRESS);
	freeop (ap2);
	return mk_legal (ap1, flags);
    default:
	FATAL ((__FILE__, "g_assign", "nodetype = %d", ep->v.p[0]->nodetype));
	break;
    }
    return NIL_ADDRESS;
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

    switch (tp->type) {
    case bt_void:
	flagx = (FLAGS) (F_ALL | F_NOVALUE);
	break;
    case bt_struct:
    case bt_union:
	tp = tp_pointer;
	/*FALLTHROUGH */
    default:
	flagx = (FLAGS) (F_REG | F_VOL);
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
    freeop (ap1);

    /* all registers are void */

    g_branch (end_label);
    g_label (false_label);

    ap2 = g_expr (ep->v.p[1], flagx);

    g_label (end_label);

    g_stack (stack_offset);
    stack_offset = offset;
    return mk_legal (ap2, flags);
}

/*--------------------------------------------------------------------------*/

static SIZE push_param P1 (const EXPR *, ep)
{
    (void) ep;
    return 0L;
}

/*
 * generate the function return addressing mode
 */
static ADDRESS *func_result P3 (FLAGS, flags, SIZE, size, const EXPR *, ep)
{
    ADDRESS *ap;

    (void) size;
    (void) ep;
    if (flags & F_NOVALUE) {
	return NIL_ADDRESS;
    }
    ap = data_register ();
    return ap;
}

/*
 * generate the parameters for a function call
 */
static SIZE g_parms P1 (const EXPR *, ep)
{
    SIZE    size;

    for (size = 0L; ep != NIL_EXPR; ep = ep->v.p[1]) {
	size += push_param (ep->v.p[0]);
    }
    return size;
}

/*
 * generate a function call node and return the addressing mode of the result
 */
static ADDRESS *g_fcall P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap;
    SIZE    size;
    EXPR   *ep0 = ep->v.p[0];

    if (!is_parameter) {
	switch (stackopt_option) {
	case 1:
	    /*
	     *               "Safe" stack optimisation.  Perform a stack optimisation
	     *               unless:
	     *               1.  The alloca() routine is called
	     *               2.  The function call is via a variable
	     *               3.  The function starts with an underscore character
	     */
	    if ((ep0->nodetype != en_nacon) ||
		(ep0->v.str[0] == (CHAR) '_') || (ep0->v.str == alloca_name)) {
		g_stack (stack_offset);
	    }
	    break;
	case 2:
	    /*
	     *               "Forced" stack optimisation.   This will not suppress
	     *               the optimisation on encountering calls to functions
	     *               whose names begin with underscore.
	     */
	    if ((ep0->nodetype != en_nacon) || (ep0->v.str == alloca_name)) {
		g_stack (stack_offset);
	    }
	    break;
	default:
	case 0:
	    /*
	     *               no stack optimisation
	     */
	    g_stack (stack_offset);
	    break;
	}
    }
    temp_inv ();		/* push any used temporary registers */
    size = g_parms (ep0);	/* generate parameters */
    switch (ep0->nodetype) {
    case en_nacon:
    case en_labcon:
	ap = mk_direct (ep0);
	g_code (op_bl, cc_al, ap, NIL_ADDRESS, NIL_ADDRESS);
	break;
    default:
	ap = g_expr (ep0, F_REG);
	g_code (op_mov, cc_al, mk_reg (R15), ap, NIL_ADDRESS);
	break;
    }
    return func_result (flags, size, ep);
}

#ifdef ASM
static ADDRESS *g_asm P1 (const EXPR *, ep)
{
    ADDRESS *ap = mk_amode (am_str);

    ap->offset = copynode (ep);
    g_code (op_asm, cc_al, ap, NIL_ADDRESS, NIL_ADDRESS);
    return NIL_ADDRESS;
}
#endif /* ASM */

/*--------------------------------------------------------------------------*/

/*
 * general expression evaluation.  returns the addressing mode of the result
 */
static ADDRESS *g_expr P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1;
    LABEL   lab0, lab1;
    CONDITION cc;
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

    if (tst_const (ep)) {
	ap1 = mk_amode (am_immed);
	ap1->offset = copynode (ep);
	return mk_legal (ap1, flags);
    }
    switch (ep->nodetype) {
    case en_register:
	ap1 = mk_reg (ep->v.r);
	return mk_legal (ap1, flags);
    case en_ref:
	ap1 = g_deref (ep->v.p[0], flags);
	return mk_legal (ap1, flags);
    case en_uminus:
	return g_uminus (ep, flags);
    case en_compl:
	return g_negate (ep, flags);
    case en_add:
	return g_add (ep, flags, op_add);
    case en_sub:
	return g_add (ep, flags, op_sub);
    case en_mul:
	return g_mul (ep, flags);
    case en_div:
	return g_div (ep, flags);
    case en_mod:
	return g_mod (ep, flags);
    case en_and:
	return g_logic (ep, flags, op_and);
    case en_or:
	return g_logic (ep, flags, op_orr);
    case en_xor:
	return g_logic (ep, flags, op_eor);
    case en_lsh:
	return g_lshift (ep, flags);
    case en_rsh:
	return g_rshift (ep, flags);
    case en_asadd:
	return g_asadd (ep, flags, op_add);
    case en_assub:
	return g_asadd (ep, flags, op_sub);
    case en_asmul:
	return g_asmul (ep, flags);
    case en_asdiv:
	return g_asdiv (ep, flags);
    case en_asmod:
	return g_asmod (ep, flags);
    case en_asand:
	return g_aslogic (ep, flags, op_and);
    case en_asor:
	return g_aslogic (ep, flags, op_orr);
    case en_asxor:
	return g_aslogic (ep, flags, op_eor);
    case en_aslsh:
	return g_lshift (ep, flags);
    case en_asrsh:
	return g_rshift (ep, flags);
    case en_assign:
	return g_assign (ep, flags);
    case en_comma:
	freeop (g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_NOVALUE)));
	return g_expr (ep->v.p[1], flags);
    case en_cast:
	return g_cast (g_expr (ep->v.p[0], F_ALL),
		       ep->v.p[0]->etp, ep->etp, flags);
    case en_eq:
	cc = cc_eq;
	VOIDCAST g_compare (ep);

	goto cont1;
    case en_ne:
	cc = cc_eq;
	VOIDCAST g_compare (ep);

	goto cont1;
    case en_lt:
	cc = g_compare (ep) ? cc_cc : cc_lt;
	goto cont1;
    case en_le:
	cc = g_compare (ep) ? cc_ls : cc_le;
	goto cont1;
    case en_gt:
	cc = g_compare (ep) ? cc_hi : cc_gt;
	goto cont1;
    case en_ge:
	cc = g_compare (ep) ? cc_cs : cc_ge;
      cont1:
	ap1 = data_register ();
	g_code (op_mov, cc, ap1, mk_immed (1L), NIL_ADDRESS);
	g_code (op_mov, reverse_cc[cc], ap1, mk_immed (0L), NIL_ADDRESS);
	return mk_legal (ap1, flags);
    case en_land:
    case en_lor:
    case en_not:
	lab0 = nextlabel++;
	lab1 = nextlabel++;
	g_falsejp (ep, lab0);
	ap1 = data_register ();
	g_code (op_mov, cc_al, ap1, mk_immed (1L), NIL_ADDRESS);
	g_branch (lab1);
	g_code (op_mov, cc_al, ap1, mk_immed (0L), NIL_ADDRESS);
	g_label (lab1);
	return mk_legal (ap1, flags);
    case en_cond:
	return g_hook (ep, flags);
    case en_fcall:
    case en_call:
    case en_usercall:
	return g_fcall (ep, flags);
#ifdef ASM
    case en_str:
	return g_asm (ep);
#endif /* ASM */
    default:
	FATAL ((__FILE__, "g_expr", "uncoded ep %d", ep->nodetype));
	break;
    }
    return NIL_ADDRESS;
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
    ADDRESS *ap1, *ap2;
    BOOL    sign = FALSE;

    switch (ep->v.p[0]->etp->type) {
    case bt_char:
    case bt_schar:
    case bt_short:
    case bt_int16:
    case bt_int32:
    case bt_long:
	sign = TRUE;
	/*FALLTHRU */
    case bt_uchar:
    case bt_charu:
    case bt_ushort:
    case bt_uint16:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	ap2 = g_expr (ep->v.p[1], F_ALL);
	ap1 = g_expr (ep->v.p[0], F_REG);
	validate (ap2);
	g_code (op_cmps, cc_al, ap1, ap2, NIL_ADDRESS);
	freeop (ap1);
	freeop (ap2);
	return sign;
    default:
	FATAL ((__FILE__, "g_compare", "typ = %d", ep->etp->type));
	break;
    }
    return FALSE;
}

/*
 * Test the expression and set the condition codes accordingly
 */
static void g_test P1 (const EXPR *, ep)
{
    ADDRESS *ap;

    switch (ep->etp->type) {
    case bt_char:
    case bt_schar:
    case bt_short:
    case bt_int16:
    case bt_int32:
    case bt_long:
    case bt_uchar:
    case bt_charu:
    case bt_ushort:
    case bt_uint16:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	ap = g_expr (ep, F_REG);
	g_code (op_movs, cc_al, ap, ap, NIL_ADDRESS);
	freeop (ap);
	return;
    default:
	FATAL ((__FILE__, "g_test", "typ = %d\n", ep->etp->type));
	break;
    }
}

/*
 * generate a jump to label if the node passed evaluates to a true condition.
 */
static void g_truejp P2 (const EXPR *, ep, LABEL, label)
{
    CONDITION cc;
    LABEL   lab0;

    if (is_icon (ep)) {
	if (ep->v.i) {
	    g_branch (label);
	}
	return;
    }
    switch (ep->nodetype) {
    case en_eq:
	VOIDCAST g_compare (ep);
	cc = cc_eq;
	goto cont1;
    case en_ne:
	VOIDCAST g_compare (ep);
	cc = cc_ne;
	goto cont1;
    case en_lt:
	cc = g_compare (ep) ? cc_cc : cc_lt;
	goto cont1;
    case en_le:
	cc = g_compare (ep) ? cc_ls : cc_le;
	goto cont1;
    case en_gt:
	cc = g_compare (ep) ? cc_hi : cc_gt;
	goto cont1;
    case en_ge:
	cc = g_compare (ep) ? cc_cs : cc_ge;
      cont1:
	g_cbranch (cc, label);
	return;
    case en_land:
	lab0 = nextlabel++;
	g_falsejp (ep->v.p[0], lab0);
	g_truejp (ep->v.p[1], label);
	g_label (lab0);
	return;
    case en_lor:
	g_truejp (ep->v.p[0], label);
	g_truejp (ep->v.p[1], label);
	return;
    case en_not:
	g_falsejp (ep->v.p[0], label);
	return;
    default:
	g_test (ep);
	g_cbranch (cc_ne, label);
	return;
    }
}

/*
 * generate code to execute a jump to label if the expression passed is
 * false.
 */
static void g_falsejp P2 (const EXPR *, ep, LABEL, label)
{
    CONDITION cc;
    LABEL   lab0;

    if (is_icon (ep)) {
	if (ep->v.i) {
	    g_branch (label);
	}
	return;
    }
    switch (ep->nodetype) {
    case en_eq:
	VOIDCAST g_compare (ep);
	cc = cc_ne;
	goto cont1;
    case en_ne:
	VOIDCAST g_compare (ep);
	cc = cc_eq;
	goto cont1;
    case en_lt:
	cc = g_compare (ep) ? cc_cs : cc_ge;
	goto cont1;
    case en_le:
	cc = g_compare (ep) ? cc_hi : cc_gt;
	goto cont1;
    case en_gt:
	cc = g_compare (ep) ? cc_ls : cc_le;
	goto cont1;
    case en_ge:
	cc = g_compare (ep) ? cc_cc : cc_lt;
      cont1:
	g_cbranch (cc, label);
	return;
    case en_land:
	g_falsejp (ep->v.p[0], label);
	g_falsejp (ep->v.p[1], label);
	return;
    case en_lor:
	lab0 = nextlabel++;
	g_truejp (ep->v.p[0], lab0);
	g_falsejp (ep->v.p[1], label);
	g_label (lab0);
	return;
    case en_not:
	g_truejp (ep->v.p[0], label);
	return;
    default:
	g_test (ep);
	g_cbranch (cc_eq, label);
	return;
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
    (void) ep;
    (void) sw;
    (void) min_caselabel;
    (void) max_caselabel;
    initstack ();
    checkstack ();
}

/*
 * Generate the body of a switch statement by comparing each case value
 * in turn.   The comparison is in fact done by using subtraction as this
 * actually generates more efficient code (and would work best if the
 * labels were sorted!)
 */
PRIVATE void g_switch_compare P2 (const EXPR *, ep, STMT *, stmt)
{
    (void) ep;
    (void) stmt;
    initstack ();
    checkstack ();
}

PRIVATE void g_entry P1 (SIZE, frame_size)
{
    (void) frame_size;

#if 0
    SYM    *sp;

    if (framesize > 0L) {
	sp = runtime_symbol (SYM_STACK);
	g_code (op_bl, cc_al, mk_strlab (nameof (sp)), NIL_ADDRESS,
		NIL_ADDRESS);
    }
#endif
}

PRIVATE void g_return P2 (const EXPR *, stmtexp, TYP *, tp)
{
    ADDRESS *ap;

    (void) tp;
    initstack ();
    ap = g_expr (stmtexp, F_ALL);
    g_code (op_mov, cc_al, mk_reg (reg_usage->result->reg[0]), ap,
	    NIL_ADDRESS);
    freeop (ap);
    checkstack ();
}

PRIVATE void g_epilogue P0 (void)
{
    if (restore_mask) {
	g_code (op_ldmea, cc_al, mk_reg (FRAMEPTR), mk_mask (restore_mask),
		NIL_ADDRESS);
    }
    g_code (op_mov, cc_al, mk_reg (R14), mk_reg (R15), NIL_ADDRESS);
}

/*
 * allocate will allocate registers for the expressions that have a high
 * enough desirability.
 */
PRIVATE void g_allocate P1 (CSE *, olist)
{
    CSE    *csp;
    REG     reg = (REG) ((int) max_reg + 1);
    REGMASK mask = (REGMASK) 0;
    TYP    *tp;

    regs_used = 0;
    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	tp = csp->exp->etp;
	if (!reg_option && desire (csp) < (USES) 5000) {
	    csp->reg = NO_REG;
	} else if (reg < frameptr
		   && csp->exp->nodetype != en_icon
		   && csp->exp->nodetype != en_nacon
		   && (tp->type == bt_pointer32 ||
		       tp->type == bt_long ||
		       tp->type == bt_ulong ||
		       tp->type == bt_int16 ||
		       tp->type == bt_uint16 ||
		       tp->type == bt_int32 ||
		       tp->type == bt_uint32 ||
		       tp->type == bt_short ||
		       tp->type == bt_ushort ||
		       tp->type == bt_char ||
		       tp->type == bt_schar ||
		       tp->type == bt_uchar || tp->type == bt_charu)) {
	    csp->reg = reg++;
	    mask |= REGBIT (csp->reg);
	    regs_used++;
	} else {
	    csp->reg = NO_REG;
	}
    }

    if (mask != (REGMASK) 0) {
	g_code (op_stmfd, cc_al, mk_reg (STACKPTR), mk_mask (mask),
		NIL_ADDRESS);
    }
    restore_mask = mask;
}

/*
 *	 Go through the common sub-expression tree and check to see if
 *	 any registers must be loaded with a value.
 */

PRIVATE void g_preload P1 (CSE *, olist)
{
    CSE    *csp;
    EXPR   *ep;
    ADDRESS *ap, *ap2;

    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	if (csp->reg != NO_REG) {
	    ep = csp->exp;
	    if ((!is_lvalue (ep)) || ep->v.p[0]->v.i > 0L) {
		initstack ();
		ap = g_expr (ep, F_ALL);
		ap2 = mk_reg (csp->reg);
		g_code (op_mov, cc_al, ap, ap2, NIL_ADDRESS);
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
    return TRUE;
}

PRIVATE BOOL g_is_ascending_stack P0 (void)
{
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
    if (ep == NIL_EXPR) {
	return ep;
    }
    switch (ep->nodetype) {
    case en_icon:
    case en_nacon:
    case en_labcon:
    case en_autocon:
    case en_sym:
    case en_ref:
    case en_fieldref:
    case en_register:
    case en_uminus:
    case en_test:
    case en_not:
    case en_compl:
    case en_ainc:
    case en_adec:
    case en_cast:
    case en_deref:
    case en_add:
    case en_sub:
    case en_div:
    case en_mod:
    case en_lsh:
    case en_rsh:
    case en_and:
    case en_or:
    case en_xor:
    case en_land:
    case en_lor:
    case en_eq:
    case en_ne:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
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
    case en_mul:
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
    return ep;
}

/*
 *	 This routine is called when the compiler is initialising, i.e.
 *	 before it even starts scanning tokens.
 */
PRIVATE void g_initialize P0 (void)
{
    if (!optimize_option) {
	stackopt_option = 0;
    }
    if (stackopt_option) {
	is_parameter = FALSE;
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
    (void) tp;
    return reg_usage;
}

#ifdef MULTIPLE_PROCESSORS
#define MCARM_FUNCS (void *)&mcarm_funcs
#else
#define MCARM_FUNCS (void *)NULL
#endif /* MULTIPLE_PROCESSORS */

static OPTSET peepset[] = {
    {(const CHAR *) "none", PEEP_NONE},
    {(const CHAR *) "flow", PEEP_FLOW},
    {(const CHAR *) "all", PEEP_ALL},
    {(const CHAR *) NULL, 0}
};

static OPTENUM yesnoopts[] = {
    {(const CHAR *) "yes", 1},
    {(const CHAR *) "no", 0},
    {(const CHAR *) NULL, 0}
};

static OPTION opts[] = {
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
#ifdef TRANSLATE
    {
     (const CHAR *) "trans=", enumeration_option,
     {&trans_option},
     {&yesnoopts[0]}
     },
#endif /* TRANSLATE */
#ifdef MULTIPLE_ASSEMBLERS
#ifdef TARGET_ARM
    {
     (const CHAR *) "armarm", chip_option,
     {&armarm_funcs},
     {MCARM_FUNCS}
     },
#endif /* TARGET_OBJ */
#ifdef TARGET_OBJ
    {
     (const CHAR *) "objarm", chip_option,
     {&armobj_funcs},
     {MCARM_FUNCS}
     },
#endif /* TARGET_OBJ */
#else
#ifdef TARGET_ARM
    {
     (const CHAR *) "armarm", chip_option,
     {NULL},
     {MCARM_FUNCS}
     },
#endif /* TARGET_OBJ */
#ifdef TARGET_OBJ
    {
     (const CHAR *) "objarm", chip_option,
     {NULL},
     {MCARM_FUNCS}
     },
#endif /* TARGET_OBJ */
#endif /* MULTIPLE_ASSEMBLERS */
    {
     (const CHAR *) NULL, NULL,
     {NULL},
     {NULL}
     }
};

OPTIONS optsarm = {
    (const CHAR *) "ARM ",
    opts
};

#ifdef MULTIPLE_PROCESSORS
struct genfuncs mcarm_funcs = {
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
    &alignments_arm[0]
};

#undef GEN_MODULE
#endif /* MULTIPLE_PROCESSORS */
#endif /* ARM */
