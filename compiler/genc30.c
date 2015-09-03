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
 *
 *
 * 1995   Ivo Oesch, Started in to write a codegenerator for the
 *        signalprocessor TMS320C30 (December)
 */

/******************************************************************************
 *
 * this module contains all of the code generation routines for evaluating
 * expressions and conditions.
 *
 *****************************************************************************/

#include "config.h"

#ifdef TMS320C30

#define	GEN_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genc30.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define NEW_BITFIELDSTUFF

#define best_flags(requested, possible) ((FLAGS)((requested&possible)!=0 ? requested&possible : possible))
#define isanyreg(AP) ((AP->mode==am_areg)||(AP->mode==am_dreg)||(AP->mode==am_ireg)||(AP->mode==am_sreg))
#define AL_DEFAULT      (g_alignments[bt_ellipsis])
#define is_op3_violated(ap) (  ((ap)->mode == am_ainc)  || ((ap)->mode == am_adec)   \
		             ||((ap)->mode == am_preinc)|| ((ap)->mode == am_predec) \
		             ||((ap)->mode == am_direct)|| ((ap)->mode == am_immed)  \
		             ||((ap)->mode == am_const_direct))


/********************************************************** Type Definitions */

typedef struct _itree
{
    LABEL   label;
    EXPR   *value;
    struct _itree *less;
    struct _itree *more;
}
ITREE;

/********************************************************** Static Variables */

static unsigned peep_option = PEEP_STANDARD;	/* peehole optimisations */
static int regs_used = 0;	/* number of register variable allocated */
static int address_reg_used = 0;	/* number of register variable allocated to addressregisters */

static REGMASK restore_mask;	/* register restore mask */
static REGMASK floatrestore_mask;	/* register restore mask */
static REGMASK interrupt_restore_mask = 0;	/* register restore mask */
static REGMASK interrupt_floatrestore_mask = 0;	/* register restore mask */
static REGMASK return_register_mask = 0;	/* used returnregister */
static SIZE max_stack_adjust = 0L;	/* largest amount stack is altered */

static REGMASK forced_save_mask = 0;

#if 0
static REGMASK forced_floatsave_mask = 0;

#else
#define forced_floatsave_mask (forced_save_mask & MASK_REG_DATA)
#endif
/*
 *  The following tables specify the alignment requirements of the
 *  basic types depending on the processor type.
 */
static SIZE alignments_c30[] = {
    1L,				/* bt_void      */
    1L,				/* bt_bool      */
    1L,				/* bt_char      */
    1L,				/* bt_charu     */
    1L,				/* bt_uchar     */
    1L,				/* bt_schar     */
    1L,				/* bt_short     */
    1L,				/* bt_ushort    */
    1L,				/* bt_int16     */
    1L,				/* bt_uint16    */
    1L,				/* bt_int32     */
    1L,				/* bt_uint32    */
    1L,				/* bt_long      */
    1L,				/* bt_ulong     */
    1L,				/* bt_longlong  */
    1L,				/* bt_ulonglong */
    1L,				/* bt_float     */
    1L,				/* bt_double    */
    1L,				/* bt_longdouble */
    1L,				/* bt_floatcomplex */
    1L,				/* bt_doublecomplex */
    1L,				/* bt_longdoublecomplex */
    1L,				/* bt_floatimaginary */
    1L,				/* bt_doubleimaginary */
    1L,				/* bt_longdoubleimaginary */
    1L,				/* bt_pointer16 */
    1L,				/* bt_pointer32 */
    1L,				/* bt_struct    */
    1L,				/* bt_union     */
    1L,				/* bt_func      */
    1L,				/* bt_bitfield  */
    1L,				/* bt_ubitfield */
    1L,				/* bt_bbitfield */
    1L				/* bt_ellipsis - used for alignment suitable for all types */
};


/*
 * support routines, define as pointers to be sure to have only
 * one copy in memory of them, else symsearch does not work
 * properly sinc it compares pointers and not contents of strings
 */

static const CHAR *psup_fpdiv = SUP_FPDIV;
static const CHAR *psup_fprem = SUP_FPREM;
static const CHAR *psup_ldiv = SUP_LDIV;
static const CHAR *psup_lrem = SUP_LREM;
static const CHAR *psup_uldiv = SUP_ULDIV;
static const CHAR *psup_ulrem = SUP_ULREM;

static int opt_const_in_ram = OPT_NO;
static int opt_true_long = OPT_NO;
static int stackopt_option = OPT_SAFE;
static int opt_shortfloat = 1;
static int interrupt_option = OPT_NO;
static int opt_traps = OPT_NO;

int     opt_branches = OPT_LEVEL1;
int     opt_delayed_branches = 2;
const CHAR *opt_peep_sequence = (const CHAR *) "";
int     opt_peep_test = 0;

REG     frameptr = FRAMEPTR;

#ifndef MULTIPLE_PROCESSORS
PRIVATE SIZE *g_alignments = &alignments_c30[0];

#endif /* MULTIPLE_PROCESSORS */

/*********************************************** Static Function Definitions */

static ADDRESS *as_fcall P_ ((const EXPR *, FLAGS, const CHAR *, ITYPE));
static ADDRESS *func_result P_ ((FLAGS, SIZE, TYP *));
static ADDRESS *g_addsub P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_aincdec P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asadd P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asbitfield P_ ((const EXPR *, FLAGS, OPCODE, BOOL));
static ADDRESS *g_asdiv P_ ((const EXPR *, FLAGS, BOOL));
static ADDRESS *g_aslogic P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_asmul P_ ((const EXPR *, FLAGS));
static ADDRESS *g_asshift P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_assign P_ ((const EXPR *, FLAGS));
static ADDRESS *g_cast P_ ((ADDRESS *, TYP *, TYP *, FLAGS));
static ADDRESS *g_conditionalload
P_ ((const EXPR *, ADDRESS *, ADDRESS *, ITYPE, FLAGS));
static ADDRESS *g_deref P_ ((const EXPR *, TYP *, FLAGS));
static ADDRESS *g_div P_ ((const EXPR *, FLAGS, BOOL));
static ADDRESS *g_expr P_ ((const EXPR *, FLAGS));
static ADDRESS *g_fcall P_ ((const EXPR *, FLAGS));
static ADDRESS *g_fderef P_ ((const EXPR *, FLAGS));
static ADDRESS *g_hook P_ ((const EXPR *, FLAGS));
static ADDRESS *g_index P_ ((const EXPR *));
static ADDRESS *g_logic P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_mul P_ ((const EXPR *, FLAGS));
static ADDRESS *g_shift P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *g_unary P_ ((const EXPR *, FLAGS, OPCODE));
static ADDRESS *mk_legal P_ ((ADDRESS *, FLAGS, ITYPE));
static ADDRESS *mk_ilabel P_ ((const EXPR *));
static ADDRESS *check_trap P_ ((const EXPR *));
static ADDRESS *mk_indirect P_ ((REG, EXPR *));

static BOOL g_compare P_ ((const EXPR *));
static BOOL tst_iconst P_ ((const EXPR *));
static EXPR *copy_iexpr P_ ((const EXPR *));
static SIZE g_parms P_ ((const EXPR *));
static SIZE push_param P_ ((const EXPR *));
static void call_library_r0_r1 P_ ((const CHAR *));
static void g_immed P_ ((OPCODE, IVAL, ADDRESS *));
static void g_rotate P_ ((ADDRESS *, int, TYP *, int));
static void g_test P_ ((const EXPR *));
static void structassign P_ ((ADDRESS *, ADDRESS *, SIZE));
static void g_truejp P_ ((const EXPR *, LABEL));
static void g_falsejp P_ ((const EXPR *, LABEL));
static void allocate_blockrepeat_registers P_ ((void));


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


/*****************************************************************************/

static ADDRESS *mk_amode P1 (AMODE, mode)
{
    ADDRESS *ap;
    ap = (ADDRESS *) xalloc ((size_t) sizeof (ADDRESS));

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
ADDRESS *copy_addr P2 (ADDRESS *, ap, AMODE, mode)
{
    ADDRESS *newap;

    assert (ap);
    newap = (ADDRESS *) xalloc ((size_t) sizeof (ADDRESS));

    *newap = *ap;
    newap->mode = mode;
    return newap;
}

/*
 * make a node to reference an signed immediate value i.
 */
static ADDRESS *mk_immed P1 (IVAL, i)
{
    if ((i <= 32767L) && (i >= -32768L))
	return mk_expr (am_immed, mk_const (i));
    else
	return mk_ilabel (mk_const (i));
}

/*
 * make a node to reference an unsigned immediate value i.
 */
static ADDRESS *mk_uimmed P1 (IVAL, i)
{
    if (i < 0)
	FATAL ((__FILE__, "mk_uimmed", "value is negative %ld", i));
    if (i <= 0xFFFFL)
	return mk_expr (am_immed, mk_const (i));
    else
	return mk_ilabel (mk_const (i));
}

/*
 * construct a reference node for an internal label number.
 */
ADDRESS *mk_label P1 (LABEL, lab)
{
    return mk_expr (am_const_direct, mk_lcon (lab));
}

static ADDRESS *mk_Jumplabel P1 (LABEL, lab)
{
    return mk_expr (am_immed, mk_lcon (lab));
}

/*
 * generate a direct reference to a string label.
 */
static ADDRESS *mk_strlab P1 (const CHAR *, s)
{
    EXPR   *ep;

    ep = mk_node (en_nacon, NIL_EXPR, NIL_EXPR, tp_void);
    ep->v.str = s;
    return mk_expr (am_immed, ep);
}

/*
 * generate a call to a library routine.
 * it is assumed that lib_name won''t be clobbered
 */
static void call_library_r0_r1 P1 (const CHAR *, lib_name)
{
    SYM    *sp;

    sp = internal_symbol (lib_name, tp_void);

    /* routines are using all temporary-registers, so 
     * intrruptroutines must save them
     */

    interrupt_restore_mask |=
	MEMBER (REG_R0) | MEMBER (REG_R1) | MEMBER (REG_R2) | MEMBER (REG_AR0)
	| MEMBER (REG_AR1);

    interrupt_floatrestore_mask |=
	MEMBER (REG_R0) | MEMBER (REG_R1) | MEMBER (REG_R2);

    /*
     *  op_xcall is a marker for the peephole-optimizer,
     *  it assumes, that only xcall uses R0 and R1 registers
     *  for parameterpassing.
     *  all other calls are assumed to use stack for
     *  parameterpassing, so values of r0 and r1 (and
     *  all other tempregs) may be invalidated/overwritten
     *  before functionscall.
     */

    g_code (op_xcall, OP_INT, mk_strlab (nameof (sp)), NIL_ADDRESS);
}


/*
 *  make an address reference to a register.
 */
ADDRESS *mk_reg P1 (REG, r)
{
    ADDRESS *ap;

    switch (r) {
    case REG_R0:
    case REG_R1:
    case REG_R2:
    case REG_R3:
    case REG_R4:
    case REG_R5:
    case REG_R6:
    case REG_R7:
	ap = mk_amode (am_dreg);
	ap->preg = r;
	break;
    case REG_AR0:
    case REG_AR1:
    case REG_AR2:
    case REG_AR3:
    case REG_AR4:
    case REG_AR5:
    case REG_AR6:
    case REG_AR7:
	ap = mk_amode (am_areg);
	ap->preg = r;
	break;
    case REG_IR0:
    case REG_IR1:
	ap = mk_amode (am_ireg);
	ap->preg = r;
	break;
    case REG_DP:
    case REG_BK:
    case REG_SP:
    case REG_ST:
    case REG_IE:
    case REG_IF:
    case REG_IOF:
    case REG_RS:
    case REG_RE:
    case REG_RC:
	ap = mk_amode (am_sreg);
	ap->preg = r;
	break;
    default:
	CANNOT_REACH_HERE ();
	ap = NIL_ADDRESS;
    }
    interrupt_restore_mask |= MEMBER (r);
    return ap;
}

/*
 * make an address reference to a floatregister.
 */
ADDRESS *mk_freg P1 (REG, r)
{
    ADDRESS *ap;

    assert (r <= REG_R7);
    ap = mk_amode (am_freg);
    ap->preg = r;
    interrupt_restore_mask |= MEMBER (r);
    interrupt_floatrestore_mask |= MEMBER (r);
    return ap;
}

/*
 * returns addressing mode of form offset(frameptr)
 * size is rounded up to AL_DEFAULT
 */
static ADDRESS *mk_scratch P1 (SIZE, size)
{
    ADDRESS *ap;
    SIZE    default_alignment = AL_DEFAULT;

    /* round up the request */
    if (size % default_alignment)
	size += default_alignment - (size % default_alignment);

    /* allocate the storage */
    act_scratch += size;

    /*
     * The next statement could be deferred and put into the
     * routine checkstack(), but this is just safer.
     */
    if (act_scratch > max_scratch) {
	max_scratch = act_scratch;
    }
    /* do not add act_scratch, because pointer must point to start of
     * scratch area
     */
    ap = mk_indirect (frameptr, mk_const ((lc_auto_max + 1)));
    return ap;
}

/*
 * push all register in the masks onto stack.
 */
static void push_registers P2 (REGMASK, mask, REGMASK, floatmask)
{
    REG     reg;

    for (reg = MAX_REG; reg >= REG_R0; reg--) {
	/* Check which registers are set in mask */
	if ((mask & MEMBER (reg)) != 0) {
	    g_code (op_pushnopeep, OP_INT, mk_reg (reg), NIL_ADDRESS);
	}
	if ((floatmask & MEMBER (reg)) != 0) {
	    g_code (op_pushfnopeep, OP_FLOAT, mk_reg (reg), NIL_ADDRESS);
	}
    }
}

/*
 * pop all registers in the masks from the stack.
 */
static void pop_registers P2 (REGMASK, mask, REGMASK, floatmask)
{
    REG     reg;

    for (reg = REG_R0; reg <= MAX_REG; reg++) {
	/* Check which registers are set in mask */
	if ((floatmask & MEMBER (reg)) != 0) {
	    g_code (op_popf, OP_FLOAT, NIL_ADDRESS, mk_reg (reg));
	}
	if ((mask & MEMBER (reg)) != 0) {
	    g_code (op_pop, OP_INT, NIL_ADDRESS, mk_reg (reg));
	}
    }
}

/*
 * reads all register in the mask from the stack
 * this version does not use pop, but indeed address based on framepointer
 * and offset of the pushed registers from framepointer
 * generates in some cases faster code than poped version
 * since sp must not be set correctly as in the pop-variant
 */
static void pop_fast_registers P3 (REGMASK, mask, REGMASK, floatmask, IVAL,
				   offset)
{
    REG     reg;
    ADDRESS *ap;

    for (reg = REG_R0; reg <= MAX_REG; reg++) {
	/* Check which registers are set in mask */
	if ((floatmask & MEMBER (reg)) != 0) {
	    ap = mk_amode (am_indx);
	    ap->preg = frameptr;	/* frame pointer */
	    ap->u.offset = mk_const (offset--);
	    g_code (op_popldf, OP_FLOAT, ap, mk_reg (reg));
	}
	if ((mask & MEMBER (reg)) != 0) {
	    ap = mk_amode (am_indx);
	    ap->preg = frameptr;	/* frame pointer */
	    ap->u.offset = mk_const (offset--);
	    g_code (op_popldi, OP_INT, ap, mk_reg (reg));
	}
    }
}

/*
 * make a direct reference to a node.
 */
static ADDRESS *mk_immediatelabel P1 (EXPR *, ep)
{
    return mk_expr (am_immed, ep);
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


#ifdef FLOAT_SUPPORT
/*
 * make a node to reference an immediate value f.
 */
static ADDRESS *mk_immedfloat P1 (RVAL, f)
{
    ADDRESS *ap;

    ap = mk_amode (am_immed);
    ap->u.offset = mk_fcon (&f, tp_double);
    return ap;
}
#endif /* FLOAT_SUPPORT */

static void allocate_blockrepeat_registers (void)
{
    interrupt_restore_mask |=
	MEMBER (REG_RC) | MEMBER (REG_RS) | MEMBER (REG_RE);
}


/*
 * Generate a label which points to an integer constant.  This routine
 * ensures that only one copy of the constant is generated.
 */
static ADDRESS *mk_ilabel P1 (const EXPR *, ep)
{
    ITREE  *p, *q;
    int     local_global = global_flag;
    LABEL   lab;
    EXPRTYPE type;

    static ITREE *iitree = NULL;	/* Tree for integer constants */
    static ITREE *ilabtree = NULL;	/* Tree for label   constants */
    static ITREE *inatree = NULL;	/* Tree for na      constants */
    static ITREE *iexprtree = NULL;	/* Tree for expr    constants */

    lab = nextlabel++;

    type = ep->nodetype;
    if (tst_iconst (ep)) {
	type = en_icon;
	/*
	 *FATAL((__FILE__,"mk_ilabel", "icon not supportet now"));
	 */
    }
    switch (type) {
    case en_icon:
	p = iitree;
	break;
    case en_nacon:
	p = inatree;
	break;
    case en_labcon:
	p = ilabtree;
	break;
    case en_add:
    case en_sub:
    case en_cast:
    case en_uminus:
	p = iexprtree;
	break;
    default:
	FATAL ((__FILE__, "mk_ilabel", "illegal nodetype %d", type));
    }
    /* in the moment we do not try to build a tree */
    for (q = p; p; p = p->more) {
	if (is_equalnode (p->value, ep)) {
	    return mk_label (p->label);
	}
	q = p;
    }
    global_flag = 1;
    p = (ITREE *) xalloc ((int) sizeof (ITREE));
    p->label = lab;
    p->value = copy_iexpr (ep);
    p->less = p->more = NULL;
    global_flag = local_global;
    if (q == NULL) {
	switch (type) {
	case en_icon:
	    iitree = p;
	    break;
	case en_nacon:
	    inatree = p;
	    break;
	case en_labcon:
	    ilabtree = p;
	    break;
	case en_add:
	case en_sub:
	case en_cast:
	case en_uminus:
	    iexprtree = p;
	    break;
	default:
	    break;
	}
    }
    /*else if (q->value < val) */
    /*  q->less = p;          */
    else
	q->more = p;
    put_kseg (alignment_of_type (tp_double));
    put_label (lab);
    put_pointer (ep);
    return mk_label (lab);
}

/*****************************************************************************/

static EXPR *copy_iexpr P1 (const EXPR *, ep)
{
    EXPR   *newep;

    if (ep == NIL_EXPR)
	return NIL_EXPR;
    newep = copynode (ep);

    switch (ep->nodetype) {
    case en_icon:
    case en_autocon:
    case en_labcon:
    case en_nacon:
	return newep;
    case en_add:
    case en_sub:
	newep->v.p[0] = copy_iexpr (ep->v.p[0]);
	newep->v.p[1] = copy_iexpr (ep->v.p[1]);
	return newep;
    case en_cast:
    case en_uminus:
	newep->v.p[0] = copy_iexpr (ep->v.p[0]);
	return newep;
    default:
	FATAL ((__FILE__, "copy_iexpr", "illegal nodetype %d", ep->nodetype));
	return NIL_EXPR;
    }
}

/**************************************************************************************/


/*
 * return true if the node passed can be used as offset in addressgeneration.
 */
static BOOL is_offset P1 (const EXPR *, ep)
{
    return is_icon (ep) && (ep->v.i >= -255L && ep->v.i <= 255L);
}

/*
 * return true if the node passed can be generated as a short offset.
 */
static BOOL is_short P1 (const EXPR *, ep)
{
    return is_icon (ep) && (ep->v.i >= -32768L && ep->v.i <= 32767L);
}

/*
 * return true if the node passed can be generated as a unsigned short
 * offset.
 */
static BOOL is_ushort P1 (const EXPR *, ep)
{
    return is_icon (ep) && (ep->v.i >= 0L && ep->v.i <= 0xFFFFL);
}

/*
 * delivers true, if node can ba evaluated to an addressingmode
 * suitable for an 3-operand instruction
 * (register, *an, *+an(1), *-an(1), *-an(irm), *+an(irm))
 *                                   ^^^^^^^^^^^^^^^^^^^^-not included yet
 *
 * Autoincrements are currently not accepted for 3-op-instructions
 * ->could give troubles when both operands are the same address-
 *   registers (*arn++(1), *arm++(1), r) arn == arm
 *    
 *
 * Attention!! do not forget to change makro is_op3_violated()
 *             if you add the auto-/pre-incre-/decre-ments 
 */
static BOOL is_op3_possible (EXPR *node)
{
    /* Look for register */
    if (node->nodetype == en_register) {
	return TRUE;
    }
    /* look for autocons *ar7+(1) or *ar7-(1) */
    if ((node->nodetype == en_autocon)
	&& (node->v.i >= -1)
	&& (node->v.i <= 1)) {
	return TRUE;
    }
    if (node->nodetype == en_ref) {
	switch (node->v.p[0]->nodetype) {
	    /* Look for *arn */
	case en_register:
	    if (is_address_register (node->v.p[0]->v.r)) {
		return TRUE;
	    }
	    break;
	    /* look for indexnode *arn+(1) or *arn-(1) */
	case en_add:
	    if ((node->v.p[0]->v.p[0]->nodetype == en_register)
		&& (is_address_register (node->v.p[0]->v.p[0]->v.r))
		&& is_icon (node->v.p[0]->v.p[1])
		&& (node->v.p[0]->v.p[1]->v.i >= -1)
		&& (node->v.p[0]->v.p[1]->v.i <= 1)) {
		return TRUE;
	    }
	    /* look for indexnode *arn+(1) or *arn-(1), nodes of add swapped */
	    if ((node->v.p[0]->v.p[1]->nodetype == en_register)
		&& (is_address_register (node->v.p[0]->v.p[1]->v.r))
		&& is_icon (node->v.p[0]->v.p[0])
		&& (node->v.p[0]->v.p[0]->v.i >= -1)
		&& (node->v.p[0]->v.p[0]->v.i <= 1)) {
		return TRUE;
	    }
	    break;
#if 0				/* currently not supported */
	case en_ainc:
	case en_adec:
	    /* look for autoincre/decrement *arn++(1) / *arn--(1) */
	    if (ep->v.p[1]->v.i == 1
		&& ep->v.p[0]->nodetype == en_register
		&& is_address_register (ep->v.p[0]->v.r)) {
		return TRUE;
	    }
	    break;
	case en_asadd:
	case en_assub:
	    /* look for preincre/decrement *++arn(1) / *--arn(1) */
	    if (is_icon (ep->v.p[1])
		&& ep->v.p[1]->v.i == 1
		&& ep->v.p[0]->nodetype == en_register
		&& is_address_register (ep->v.p[0]->v.r)) {
		/* *++An(1) */
		return TRUE;
	    }
	    break;
#endif
	default:
	    break;
	}
    }
    return FALSE;
}

#ifdef FLOAT_SUPPORT
BOOL    is_short_float (const RVAL f, BTYPE tp)
{
    int     i;
    RVAL    intpart;

    if ((f > MAX_POS_SHORT_FLOAT) || (f < MIN_NEG_SHORT_FLOAT))
	return (FALSE);
    if (f == 0.0)
	return (TRUE);
    if ((f > MAX_NEG_SHORT_FLOAT) && (f < MIN_POS_SHORT_FLOAT))
	return (FALSE);
    /*
     *  perhaps here should follow a check for the precision
     *  sinc shortfloats only offers a precision of 11 Mantissabits
     *  and sometimes we dont want to loose precision of constants
     *  but in the moment I dont know how to do this check
     *
     */
    /* We differ now in precission between float and (long-)double */

    /* we have 3 options (opt_shortfloat):
     * 0 we use shortfloats only if we are absolutely sure they
     *   wont bring any loss in precision
     *
     * 1 floats are allways represented in shortform if they are in
     *   range, double and longdouble only if there is no loss in
     *   precision
     *
     * 3 floats, doubles ans longdoubles  are allways represented
     *   in shortform if they are in range, there may be a loss in
     *   precision
     */

    if ((tp == bt_float) && (opt_shortfloat > 0))
	return (TRUE);

    if (opt_shortfloat > 1)
	return (TRUE);
    /* cut away the fractional part */
    i = (int) f;
    intpart = (RVAL) i;
    /* we use immediates for long and double only for
     * floats with no fractional part, then we can
     * be sure we dont loss any precission
     * It's not the best solution, but for now it has to do
     * it would be better to lock if the number has not more than
     * 12 significant mantissabit and if the exponent is between
     * +7 and -7 (base 2, of course), then the number can be represented
     * in the short form without any losses
     */
    if (intpart == f)
	return (TRUE);
#if 0
    /* New way to find out if a number can be represented with 11
     * mantissabits, needs still to be tested
     */
    /* shift mantissa, until first mantissabit has reached position
     * of bit 11 in an integer
     */
    if (f > 0.0) {
	while (f < ((double) (1UL << 11))) {
	    f = f + f;
	}
    } else {
	while (f > ((double) (-(1UL << 11)))) {
	    f = f + f;
	}
    }
    /* cut away the fractional part */
    i = (int) f;
    intpart = i;
    /* if there are no bits used in the fractionalpart we
     * may use the short integerformat
     */
    if (intpart == f)
	return (TRUE);

#endif
    return (FALSE);
}
#endif /* FLOAT_SUPPORT */

/*
 * tests if it is a labelfree constant node, that means either en_icon,
 * or sums or differences of such nodes
 */
static BOOL tst_iconst P1 (const EXPR *, ep)
{
    switch (ep->nodetype) {
    case en_icon:
	return TRUE;
    case en_add:
    case en_sub:
	return tst_iconst (ep->v.p[0]) && tst_iconst (ep->v.p[1]);
#ifndef RELOC_BUG
    case en_cast:
#endif
    case en_uminus:
	return tst_iconst (ep->v.p[0]);
    default:
	break;
    }
    return FALSE;
}


/*
 * Checks, if the given nacon-name is a valid
 * trapname, eg is __trap_00 to __trap_31.
 * also valid names are __trap_nn_xxxx, where
 * xxx is any valid sequence of identifiercharacters.
 * if so, returns an address containing the immediatevalue
 * of the trapnumber, otherwise returns NIL_ADDRESS
 * is used to convert functionscalls named __trap_nn to
 * trapu-opcodes, simplifies interface to operating-
 * systems.
 */

static ADDRESS *check_trap P1 (const EXPR *, ep)
{
    static const CHAR *trapname = (const CHAR *) "__trap_nn";
    const CHAR *p, *q;
    IVAL    i;

    /*
     *  Check if trapp option was enabled
     */
    if (opt_traps == OPT_NO) {
	return NIL_ADDRESS;
    }
    if (ep->nodetype == en_nacon) {
	p = ep->v.str;
	q = trapname;
	while (*q != 'n') {
	    if (*q++ != *p++) {
		return NIL_ADDRESS;
	    }
	}
	if ((*p > '3') || (*p < '0')) {
	    return NIL_ADDRESS;
	}
	i = (IVAL) (10 * ((int) *p++ - '0'));
	if ((*p > '9') || (*p < '0')) {
	    return NIL_ADDRESS;
	}
	i += (IVAL) ((int) *p++ - '0');
	/* Check if number is in range and for end of string */
	if ((i > 31) || ((*p != 0) && (*p != '_'))) {
	    return NIL_ADDRESS;
	}
	return (mk_immed (i));
    }
    return NIL_ADDRESS;
}


/*
 * mk_legal will coerce the addressing mode in ap1 into a mode that is
 * satisfactory for the flag word.
 */
static ADDRESS *mk_legal P3 (ADDRESS *, ap, FLAGS, flags, ITYPE, size)
{
    ADDRESS *ap2, *ap3;
    OPCODE  OpSto, OpLd;

    if (flags & F_NOVALUE) {
	freeop (ap);
	return NIL_ADDRESS;
    }
    if (ap == NIL_ADDRESS) {
	FATAL ((__FILE__, "mk_legal", "ap = 0"));
	return NIL_ADDRESS;
    }
    switch (size) {
    case OP_INT:
	OpSto = op_sti;
	OpLd = op_ldi;
	if (flags & F_FREG) {
	    FATAL ((__FILE__, "mk_legal", "op_int float mixed with integer"));
	}
	break;
    case OP_FLOAT:
	OpSto = op_stf;
	OpLd = op_ldf;
	if (flags & F_XREG) {
	    FATAL (
		   (__FILE__, "mk_legal",
		    "op_float float mixed with integer"));
	}
	break;
    default:
	FATAL ((__FILE__, "mk_legal", "illegal size0"));
	break;
    }

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
	if (flags & F_FREG) {
	    FATAL (
		   (__FILE__, "mk_legal",
		    "am_areg float mixed with integer"));
	}
	break;
    case am_ireg:
	if (flags & F_IREG
	    && (!(flags & F_VOL) || is_temporary_register (ap->preg))) {
	    return ap;
	}
	if (flags & F_FREG) {
	    FATAL (
		   (__FILE__, "mk_legal",
		    "am_ireg float mixed with integer"));
	}
	break;
    case am_dreg:
	if (flags & F_DREG
	    && (!(flags & F_VOL) || is_temporary_register (ap->preg))) {
	    return ap;
	}
	if (flags & F_FREG) {
	    FATAL (
		   (__FILE__, "mk_legal",
		    "am_dreg float mixed with integer"));
	}
	break;
    case am_freg:
	if (flags & F_FREG
	    && (!(flags & F_VOL) || is_temporary_register (ap->preg))) {
	    return ap;
	}
	if (flags & F_XREG) {
	    FATAL (
		   (__FILE__, "mk_legal",
		    "am_freg float mixed with integer"));
	}
	break;
    case am_ind:
    case am_const_ind:
    case am_indx:
    case am_indx2:
    case am_indxs:
    case am_direct:
    case am_const_direct:
    case am_adec:
    case am_ainc:
    case am_preinc:
    case am_predec:
	if (flags & F_MEM)
	    return ap;
	break;
    default:
	break;
    }
    if ((flags & F_XREG) != 0) {
	/* decide, which mode is better */
	if (is_free_data () && (flags & F_DREG)) {
	    freeop (ap);	/* maybe we can use it... */
	    ap2 = data_register (F_DREG);	/* allocate to dreg */
	    g_code (op_ldi, OP_INT, ap, ap2);
	    return ap2;
	}
	if (is_free_addr () && (flags & F_AREG)) {
	    freeop (ap);	/* maybe we can use it... */
	    ap2 = address_register ();	/* allocate to dreg */
	    g_code (op_ldi, OP_INT, ap, ap2);
	    return ap2;
	}
	if (is_free_ireg () && (flags & F_IREG)) {
	    freeop (ap);
	    ap2 = index_register ();
	    g_code (op_ldi, OP_INT, ap, ap2);
	    return ap2;
	}
    }
    if (flags & F_DREG) {
	freeop (ap);		/* maybe we can use it... */
	ap2 = data_register (F_DREG);	/* allocate to dreg */
	g_code (op_ldi, OP_INT, ap, ap2);
	return ap2;
    }
    if (flags & F_FREG) {
	freeop (ap);		/* maybe we can use it... */
	ap2 = data_register (F_FREG);	/* allocate to dreg */
	g_code (op_ldf, OP_FLOAT, ap, ap2);
	return ap2;
    }
    if (flags & F_AREG) {
	freeop (ap);
	ap2 = address_register ();
	g_code (op_ldi, OP_INT, ap, ap2);
	return ap2;
    }
    if (flags & F_IREG) {
	freeop (ap);
	ap2 = index_register ();
	g_code (op_ldi, OP_INT, ap, ap2);
	return ap2;
    }
    if (flags & F_MEM) {
	if (ap->mode == am_immed) {
	    /* we can not store immediates directly in memory */
	    freeop (ap);
	    ap3 = data_register ((size == OP_INT) ? F_DREG : F_FREG);
	    g_code (OpLd, size, ap, ap3);
	    ap = ap3;
	}
	freeop (ap);
	ap2 = mk_scratch (1L);
	/* copy value into memory */
	g_code (OpSto, size, ap, ap2);
	return ap2;
    }
    FATAL ((__FILE__, "mk_legal", ""));
    return NIL_ADDRESS;
}

/*****************************************************************************/


/*
 * add a compiler generated label to the peep list.
 */
PRIVATE void g_label P1 (LABEL, labno)
{
    sync_stack ();
    g_code (op_label, OP_INT, mk_label (labno), NIL_ADDRESS);
}

#ifdef DEBUGOPT
/*
 * add a source line number to the peep list.
 */
PRIVATE void g_line P2 (LINE, line, const CHAR *, linetxt)
{
    g_code3 (op_line, OP_INT, mk_line (line), mk_linetxt (linetxt),
	     NIL_ADDRESS);
}

#endif /*DEBUGOPT */
/*
 * add a conditional branch instruction to the peep list.
 */
static void g_cbranch P2 (OPCODE, op, LABEL, labno)
{
    sync_stack ();
    g_code (op, OP_INT, mk_Jumplabel (labno), NIL_ADDRESS);
}

/*
 * add a branch instruction to the peep list.
 */
PRIVATE void g_branch P1 (LABEL, labno)
{
    g_cbranch (op_br, labno);
}

/*
 * adjust the stack by "bytes" bytes.
 */
PRIVATE void g_stack P1 (SIZE, bytes)
{
    if (bytes != 0L) {
	/* adjust stack pointer */
	g_immed (op_subi, bytes, mk_reg (STACKPTR));
	stack_offset -= bytes;
	if (max_stack_adjust < bytes)
	    max_stack_adjust = bytes;
    }
}

/*
 * Generate an instruction which takes an immediate option with optimal
 * (for space) instruction(s).
 */
static void g_immed P3 (OPCODE, op, IVAL, i, ADDRESS *, ap)
{
    if (ap != NIL_ADDRESS) {
	if (!isanyreg (ap))
	    FATAL ((__FILE__, "g_immed", "immed-to-memory"));
    }
    if ((i >= -32768L && i <= 32767L) || (opt_const_in_ram != OPT_NO)) {
	g_code (op, OP_INT, mk_immed (i), ap);
    } else {
	/* bigger constants we must build by hand */
	/* in future it could be solved through storing constants */
	/* in datasegment and fetching them from there */
	ADDRESS *ap1;
	IVAL    low = (IVAL) ((UVAL) i & 0xFFFFL);
	IVAL    high = (IVAL) ((UVAL) i >> 16);

	if (op == op_ldi) {
	    ap1 = ap;
	} else {
	    ap1 = temporary_register (F_XREG);
	}
	g_code (op_ldi, OP_INT, mk_immed (high), ap1);
	if (high != 0) {
	    g_code (op_lsh, OP_INT, mk_immed (16), ap1);
	}
	if (low != 0) {
	    g_code (op_or, OP_INT, mk_uimmed (low), ap1);
	}
	if (op != op_ldi) {
	    g_code (op, OP_INT, ap1, ap);
	    freeop (ap1);
	}
    }
}

/*
 * Generate an instruction which takes an unsigned immediate option with
 * optimal (for space) instruction(s).
 */
static void g_uimmed P3 (OPCODE, op, IVAL, i, ADDRESS *, ap)
{
    if (ap != NIL_ADDRESS) {
	if (!isanyreg (ap))
	    FATAL ((__FILE__, "g_uimmed", "immed-to-memory"));
    }
    if ((i >= 0 && i <= 0xFFFFL) || (opt_const_in_ram != OPT_NO)) {
	g_code (op, OP_INT, mk_uimmed (i), ap);
    } else {
	/* bigger constants we must build by hand */
	/* in future it could be solved through storing constants */
	/* in datasegment and fetching them from there */
	ADDRESS *ap1 = temporary_register (F_XREG);
	IVAL    low = (IVAL) ((UVAL) i & 0xFFFFL);
	IVAL    high = (IVAL) ((UVAL) i >> 16);

	g_code (op_ldi, OP_INT, mk_immed (high), ap1);
	if (high != 0) {
	    g_code (op_lsh, OP_INT, mk_immed (16), ap1);
	}
	if (low != 0) {
	    g_code (op_or, OP_INT, mk_uimmed (low), ap1);
	}
	g_code (op, OP_INT, ap1, ap);
	freeop (ap1);
    }
}

/*----------------------------------------------------------------------------*/


/*
 * generate code to evaluate an index node and return the addressing
 * mode of the result.
 */
static ADDRESS *g_index P1 (const EXPR *, ep)
{
    ADDRESS *ap1, *ap2;
    EXPR   *ep0 = ep->v.p[0];
    EXPR   *ep1 = ep->v.p[1];
    AMODE   mode;

    /*
     *  Try and ensure that we evaluate address registers first ....
     *  this leads to better code.
     */
    switch (ep->nodetype) {
    case en_add:
	if (ep1->nodetype == en_register && is_address_register (ep1->v.r)) {
	    ep0 = ep->v.p[1];
	    ep1 = ep->v.p[0];
	}
	mode = am_indx2;
	break;
    case en_sub:
	mode = am_indxs;
	break;
    default:
	FATAL ((__FILE__, "g_index", "illegal nodetype %d ", ep->nodetype));

    }
    if (ep1->nodetype == en_register && ep0->nodetype == en_register) {
	if (is_address_register (ep0->v.r) || (ep->nodetype == en_sub)) {
	    /* first node is address register */
	    ap1 = g_expr (ep0, F_AREG);
	    ap1 = copy_addr (ap1, mode);
	    ap2 = g_expr (ep1, F_IREG);
	    ap1->sreg = ap2->preg;
	    ap1->deep = ap2->deep;
	    ap1->u.offset = mk_const (0L);
	    return ap1;
	} else if (is_address_register (ep1->v.r) && (ep->nodetype == en_add)) {
	    /* second node is address register */
	    ap1 = g_expr (ep1, F_AREG);
	    ap1 = copy_addr (ap1, am_indx2);	/* (ARx,IRx) */
	    ap2 = g_expr (ep0, F_IREG);
	    ap1->sreg = ap2->preg;
	    ap1->deep = ap2->deep;
	    ap1->u.offset = mk_const (0L);
	    return ap1;
	}
    }
    /*
     *   The general case (no register)
     */
    ap1 =
	g_expr (ep0,
		(ep->nodetype ==
		 en_add) ? (FLAGS) (F_AREG | F_IMMED) : F_AREG);
    switch (ap1->mode) {
    case am_areg:
	ap2 = g_expr (ep1, (FLAGS) (F_IREG | F_IMMED));
	validate (ap1);
	break;
    case am_immed:
	if (ep->nodetype == en_sub) {
	    FATAL ((__FILE__, "g_index", "illegal adressmodee"));
	}
	ap2 = ap1;
	ap1 = g_expr (ep1, (FLAGS) (F_AREG | F_IMMED));
	validate (ap2);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
    /*
     *  possible combinations:
     *
     *          F_IMMED +/- F_IMMED
     *          F_AREG  +/- F_IMMED
     *          F_AREG  +/- F_IREG
     */

    if (ap1->mode == am_areg) {
	/* 
	 *  watch out for:
	 *          register(addr) + index_register
	 *          register(addr) + data_register
	 */
	if (!is_temporary_register (ap1->preg)) {
	    /* ap1 = tempref address register */
	    ap1 = copy_addr (ap1, ap1->mode);
	    switch (ap2->mode) {
	    case am_ireg:
		/* 0(ARx,IRy) */
		ap1->mode = mode;
		ap1->sreg = ap2->preg;
		ap1->deep = ap2->deep;
		ap1->u.offset = mk_const (0L);
		return ap1;
	    case am_immed:
		if (!is_offset (ap2->u.offset))
		    /* we want to add to ap1 later... */
		    /* so copy it in scratchregister */
		    ap1 = mk_legal (ap1, (FLAGS) (F_AREG | F_VOL), OP_INT);
		break;
	    default:
		break;
	    }

/* needs some changes in the registermanagement before this can be done */
/* we cannot correctly restore second temp if it has been pushed        */
/* (we have only one depth-information per ap and so can only keep      */
/* of one tempregister                                                  */
#if 0
	error, not suported now} else {
	    /* ap1 is temporaryregister */
	    switch (ap2->mode) {
	    case am_ireg:
		/* 0(ARx,IRy) */
		ap1 = copy_addr (ap1, ap1->mode);
		ap1->mode = am_indx2;
		ap1->sreg = ap2->preg;
		ap1->u.offset = mk_const (0L);
		return ap1;
	    default:
		break;
	    }
#endif
	}
    }
    if (ap2->mode == am_immed) {
	if (ap1->mode == am_immed) {
	    ap1 = copy_addr (ap1, am_direct);
	    if (ep->nodetype == en_add) {
		ap1->u.offset = mk_add (ap1->u.offset, ap2->u.offset);
	    } else {
		ap1->u.offset =
		    mk_node (en_sub, ap1->u.offset, ap2->u.offset, tp_void);
	    }
	    return ap1;
	}
	if (is_offset (ap2->u.offset)) {
	    ap1 = mk_legal (ap1, F_AREG, OP_INT);
	    ap1 = copy_addr (ap1, am_indx);
	    if (ep->nodetype == en_add) {
		ap1->u.offset = ap2->u.offset;
	    } else {
		ap1->u.offset = mk_const (-ap2->u.offset->v.i);
	    }
	    return ap1;
	}
    }
    /* just to be sure, ap1 should allready be volatile here... */
    if (!is_temporary_register (ap1->preg)) {
	/* ap1 is not volatile ... */
	ap1 = mk_legal (ap1, (FLAGS) (F_AREG | F_VOL), OP_INT);
    }
    if (ep->nodetype == en_add) {
	g_code (op_addi, OP_INT, ap2, ap1);	/* add left to address reg */
    } else {
	g_code (op_subi, OP_INT, ap2, ap1);	/* sub left to address reg */
    }
    ap1 = copy_addr (ap1, am_ind);
    freeop (ap2);		/* release any temps in ap2 */
    return ap1;			/* return indirect */
}

/*
 * return the addressing mode of a dereferenced node.
 */
static ADDRESS *g_deref P3 (const EXPR *, ep, TYP *, tp, FLAGS, flags)
{
    ADDRESS *ap1;

    /*
     * If a reference to a struct/union is required, return a pointer to the
     * struct instead
     */
    if (is_structure_type (tp) || is_array_assignment (tp)) {
	return g_expr (ep, F_IALL);
    }
    switch (ep->nodetype) {
#ifdef DEBUG
    case en_sub:
#endif
    case en_add:
	return g_index (ep);
    case en_autocon:
	if (ep->v.i >= -255L && ep->v.i < 255L) {
	    ap1 = mk_indirect (frameptr, mk_const (ep->v.i));
	} else {
	    ap1 = address_register ();
	    g_immed (op_ldi, ep->v.i, ap1);
	    g_code (op_addi, OP_INT, mk_reg (frameptr), ap1);
	    ap1 = copy_addr (ap1, am_ind);
	}
	return ap1;
    case en_ainc:
	/* special TMS320C30 instructions */
	if (ep->v.p[1]->v.i <= 255L	/* if size less than max autoincrement */
	    && ep->v.p[1]->v.i >= 0L	/* and if size positive */
	    && ep->v.p[0]->nodetype == en_register
	    && is_address_register (ep->v.p[0]->v.r)
	    && !(flags & F_USES)) {
	    /* (An)+ */
	    ap1 = mk_amode (am_ainc);
	    ap1->preg = ep->v.p[0]->v.r;
	    ap1->u.offset = mk_const (ep->v.p[1]->v.i);
	    return ap1;
	}
	break;
    case en_adec:
	/* special TMS320C30 instructions */
	if (ep->v.p[1]->v.i <= 255L	/* if size less than max autoincrement */
	    && ep->v.p[1]->v.i >= 0L	/* and if size positive */
	    && ep->v.p[0]->nodetype == en_register
	    && is_address_register (ep->v.p[0]->v.r)
	    && !(flags & F_USES)) {
	    /* (An)- */
	    ap1 = mk_amode (am_adec);
	    ap1->preg = ep->v.p[0]->v.r;
	    ap1->u.offset = mk_const (ep->v.p[1]->v.i);
	    return ap1;
	}
	break;
    case en_asadd:
	/* special TMS320C30 instructions */
	if (is_icon (ep->v.p[1])
	    && ep->v.p[1]->v.i <= 255L	/* if size less than max autoincrement */
	    && ep->v.p[1]->v.i >= 0L	/* and if size positive */
	    && ep->v.p[0]->nodetype == en_register
	    && is_address_register (ep->v.p[0]->v.r)
	    && !(flags & F_USES)) {
	    /* ++(An) */
	    ap1 = mk_amode (am_preinc);
	    ap1->preg = ep->v.p[0]->v.r;
	    ap1->u.offset = mk_const (ep->v.p[1]->v.i);
	    return ap1;
	}
	break;
    case en_assub:
	/* special TMS320C30 instructions */
	if (is_icon (ep->v.p[1])
	    && ep->v.p[1]->v.i <= 255L	/* if size less than max autoincrement */
	    && ep->v.p[1]->v.i >= 0L	/* and if size positive */
	    && ep->v.p[0]->nodetype == en_register
	    && is_address_register (ep->v.p[0]->v.r)
	    && !(flags & F_USES)) {
	    /* --(An) */
	    ap1 = mk_amode (am_predec);
	    ap1->preg = ep->v.p[0]->v.r;
	    ap1->u.offset = mk_const (ep->v.p[1]->v.i);
	    return ap1;
	}
	break;
    default:
	break;
    }
    /*
     * F_DIRECT Supresses to make a doubleindirection for
     * immediates, sinc it will be converted in an direct-reference
     */
    ap1 = g_expr (ep, (FLAGS) (F_AREG | F_IMMED | F_DIRECT));	/* generate address */
    if (ap1->mode == am_areg) {
	return copy_addr (ap1, am_ind);
    }
    return copy_addr (ap1, am_direct);
}

/*
 * get a bitfield value
 */
static ADDRESS *g_fderef P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap;

    ap = g_deref (ep->v.p[0], ep->etp, (FLAGS) (F_IALL));
    ap = mk_legal (ap, (FLAGS) (F_XREG | F_VOL), OP_INT);
    g_rotate (ap, (int) ep->v.bit.offset, ep->etp, (int) ep->v.bit.width);
    return mk_legal (ap, flags, OP_INT);
}

/*============================================================================*/

/*
 * generate code to evaluate a unary minus or complement. float: unary minus
 * calls a library function
 */
static ADDRESS *g_unary P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap, *ap2;

    switch (ep->etp->type) {
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
	ap =
	    g_expr (ep->v.p[0],
		    (FLAGS) (op == op_not ? F_IALL | F_UNSIGNED : F_IALL));

	/* maybe we can use ap... */
	freeop (ap);

	ap2 = temporary_register (best_flags (flags, F_XREG));
	g_code (op, OP_INT, ap, ap2);
	return mk_legal (ap2, flags, OP_INT);
    case bt_float:
    case bt_double:
    case bt_longdouble:
	if (op == op_negi) {
	    ap = g_expr (ep->v.p[0], (FLAGS) (F_FALL));

	    /* maybe we can use ap... */
	    freeop (ap);

	    ap2 = temporary_register (F_FREG);
	    g_code (op_negf, OP_FLOAT, ap, ap2);
	    return mk_legal (ap2, flags, OP_FLOAT);
	}
	/* Fall through */
    default:
	FATAL (
	       (__FILE__, "g_unary", "illegal type %d or operation %d",
		ep->etp->type, op));
	break;
    }
    return NIL_ADDRESS;
}

/*============================================================================*/

/*
 * generate an auto increment or decrement node. op should be either op_add
 * (for increment) or op_sub (for decrement).
 */
static ADDRESS *g_aincdec P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2, *ap3;

    switch (ep->etp->type) {
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
	if (ep->v.p[0]->nodetype == en_fieldref)
	    return g_asbitfield (ep, flags, op, TRUE);
	if (flags & F_NOVALUE) {	/* dont need result */
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_IALL));
	    /* if ap1 is in register we can modify it directly */
	    if (isanyreg (ap1)) {
		g_immed (op, ep->v.p[1]->v.i, ap1);
	    } else {
		/* else we must get ap1 in a register, modify it and store it again */
		ap2 = temporary_register (F_XREG);
		g_code (op_ldi, OP_INT, ap1, ap2);
		g_immed (op, ep->v.p[1]->v.i, ap2);
		g_code (op_sti, OP_INT, ap2, ap1);
		freeop (ap2);
	    }
	    return mk_legal (ap1, flags, OP_INT);
	}
	ap1 = temporary_register (best_flags (flags, F_XREG));
	ap2 = g_expr (ep->v.p[0], (FLAGS) (F_IALL | F_USES));
	validate (ap1);
	g_code (op_ldi, OP_INT, ap2, ap1);
	/* if ap2 is in register we can modify it directly */
	if (isanyreg (ap2)) {
	    g_immed (op, ep->v.p[1]->v.i, ap2);
	} else {
	    /* else we must get ap2 in a register, modify it and store it again */
	    ap3 = temporary_register (F_XREG);
	    g_code (op_ldi, OP_INT, ap1, ap3);
	    g_immed (op, ep->v.p[1]->v.i, ap3);
	    g_code (op_sti, OP_INT, ap3, ap2);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, OP_INT);
#ifdef FLOAT_SUPPORT
    case bt_float:
    case bt_double:
    case bt_longdouble:
	if (flags & F_NOVALUE) {	/* dont need result */
	    ap1 = g_expr (ep->v.p[0], F_FALL);
	    /* if ap1 is in register we can modify it directly */
	    if (ap1->mode == am_freg) {
		g_code (op, OP_FLOAT,
			mk_immedfloat ((double) ep->v.p[1]->v.f), ap1);
	    } else {
		/* else we must get ap1 in a register, modify it and store it again */
		ap2 = temporary_register (F_FREG);
		g_code (op_ldf, OP_FLOAT, ap1, ap2);
		g_code (op, OP_FLOAT,
			mk_immedfloat ((double) ep->v.p[1]->v.f), ap2);
		g_code (op_stf, OP_FLOAT, ap2, ap1);
		freeop (ap2);
	    }
	    return mk_legal (ap1, flags, OP_FLOAT);
	}
	ap1 = temporary_register (F_FREG);
	ap2 = g_expr (ep->v.p[0], (FLAGS) (F_FALL | F_USES));
	validate (ap1);
	g_code (op_ldf, OP_FLOAT, ap2, ap1);
	/* if ap2 is in register we can modify it directly */
	if (ap2->mode == am_freg) {
	    g_code (op, OP_FLOAT, mk_immedfloat ((double) ep->v.p[1]->v.f),
		    ap2);
	} else {
	    /* else we must get ap2 in a register, modify it and store it again */
	    ap3 = temporary_register (F_FREG);
	    g_code (op_ldf, OP_FLOAT, ap1, ap3);
	    g_code (op, OP_FLOAT, mk_immedfloat ((double) ep->v.p[1]->v.f),
		    ap3);
	    g_code (op_stf, OP_FLOAT, ap3, ap2);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, OP_FLOAT);
#endif /* FLOAT_SUPPORT */
    default:
	FATAL (
	       (__FILE__, "g_aincdec", "illegal type %d or float",
		ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*============================================================================*/

/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */
static ADDRESS *g_addsub P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2, *ap3;

    switch (ep->etp->type) {
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
	if (is_op3_possible (ep->v.p[0])
	    && is_op3_possible (ep->v.p[1])) {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_XREG | F_MEM));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_XREG | F_MEM));
	    /* Just to be sure... check again for correct op3-operands */
	    if (is_op3_violated (ap1) || is_op3_violated (ap2)) {
		FATAL ((__FILE__, "g_addsub", "inconsistency int "));
	    }
	    validate (ap1);
	    freeop (ap2);
	    freeop (ap1);
	    ap3 = temporary_register (best_flags (flags, F_XREG));
	    g_code3 ((op == op_addi ? op_addi3 : op_subi3), OP_INT, ap2, ap1,
		     ap3);
	    ap1 = ap3;
	} else {
	    if (tst_iconst (ep->v.p[0])) {
		ap1 =
		    g_expr (ep->v.p[1],
			    (FLAGS) (F_VOL | best_flags (flags, F_XREG)));
		ap2 = g_expr (ep->v.p[0], F_IALL);
		if (op == op_subi)
		    op = op_subri;
	    } else {
		ap1 =
		    g_expr (ep->v.p[0],
			    (FLAGS) (F_VOL | best_flags (flags, F_XREG)));
		ap2 = g_expr (ep->v.p[1], F_IALL);
	    }
	    validate (ap1);	/* in case push occurred */
	    g_code (op, OP_INT, ap2, ap1);
	    freeop (ap2);
	}
	return mk_legal (ap1, flags, OP_INT);
    case bt_longdouble:
    case bt_double:
    case bt_float:
	op = (op == op_addi) ? op_addf : op_subf;
	if (is_op3_possible (ep->v.p[0])
	    && is_op3_possible (ep->v.p[1])) {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_FREG | F_MEM));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_FREG | F_MEM));
	    /* Just to be sure... check again for correct op3-operands */
	    if (is_op3_violated (ap1) || is_op3_violated (ap2)) {
		FATAL ((__FILE__, "g_addsub", "inconsistency float "));
	    }
	    validate (ap1);
	    freeop (ap2);
	    freeop (ap1);
	    ap3 = temporary_register (F_FREG);
	    g_code3 ((op == op_addf ? op_addf3 : op_subf3), OP_FLOAT, ap2,
		     ap1, ap3);
	    ap1 = ap3;
	} else {
	    if (ep->v.p[0]->nodetype == en_fcon) {
		ap1 = g_expr (ep->v.p[1], (FLAGS) (F_VOL | F_FREG));
		ap2 = g_expr (ep->v.p[0], F_FALL);
		if (op == op_subf)
		    op = op_subrf;
	    } else {
		ap1 = g_expr (ep->v.p[0], (FLAGS) (F_VOL | F_FREG));
		ap2 = g_expr (ep->v.p[1], F_FALL);
	    }
	    validate (ap1);	/* in case push occurred */
	    g_code (op, OP_FLOAT, ap2, ap1);
	    freeop (ap2);
	}
	return mk_legal (ap1, flags, OP_FLOAT);
    default:
	FATAL ((__FILE__, "g_addsub", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate a plus equal or a minus equal node.
 */
static ADDRESS *g_asadd P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    FLAGS   flagx;
    ADDRESS *ap1, *ap2, *ap3;

    switch (ep->etp->type) {
    case bt_char:
    case bt_charu:
    case bt_schar:
    case bt_uchar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	if (ep->v.p[0]->nodetype == en_fieldref)
	    return g_asbitfield (ep, flags, op, FALSE);
	if (flags & F_NOVALUE)
	    flagx = F_IALL;
	else
	    flagx = (FLAGS) (F_IALL | F_USES);
	ap1 = g_expr (ep->v.p[0], flagx);

	if (isanyreg (ap1)) {
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_IALL));
	    validate (ap1);
	    g_code (op, OP_INT, ap2, ap1);
	} else {
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_IALL));
	    validate (ap1);
	    ap3 = temporary_register (F_XREG);
	    g_code (op_ldi, OP_INT, ap1, ap3);
	    g_code (op, OP_INT, ap2, ap3);
	    g_code (op_sti, OP_INT, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, OP_INT);
    case bt_float:
    case bt_double:
    case bt_longdouble:
	op = (op == op_addi) ? op_addf : op_subf;
	flagx = (flags & F_NOVALUE) ? F_FALL : (FLAGS) (F_FALL | F_USES);
	ap1 = g_expr (ep->v.p[0], flagx);
	if (ap1->mode == am_freg) {
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_FREG | F_MEM | F_IMMED));
	    validate (ap1);
	    g_code (op, OP_FLOAT, ap2, ap1);
	} else {
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_FREG | F_MEM | F_IMMED));
	    validate (ap1);
	    ap3 = temporary_register (F_FREG);
	    g_code (op_ldf, OP_FLOAT, ap1, ap3);
	    g_code (op, OP_FLOAT, ap2, ap3);
	    g_code (op_stf, OP_FLOAT, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, OP_FLOAT);
    default:
	FATAL ((__FILE__, "asadd", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*============================================================================*/

/*
 * generate code to evaluate a restricted binary node and return the
 * addressing mode of the result.
 */
static ADDRESS *g_logic P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2, *ap3;

    if (is_op3_possible (ep->v.p[0])
	&& is_op3_possible (ep->v.p[1])) {
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_XREG | F_MEM));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_XREG | F_MEM));
	validate (ap1);
	/* Just to be sure... check again for correct op3-operands */
	if (is_op3_violated (ap1) || is_op3_violated (ap2)) {
	    FATAL ((__FILE__, "g_logic", "inconsistency int "));
	}
	switch (op) {
	case op_and:
	    op = op_and3;
	    break;
	case op_andn:
	    op = op_andn3;
	    break;
	case op_or:
	    op = op_or3;
	    break;
	case op_xor:
	    op = op_xor3;
	    break;
	default:
	    FATAL ((__FILE__, "g_logic", "illegal opcode %d", op));
	    break;
	}
	freeop (ap2);
	freeop (ap1);
	ap3 = temporary_register (best_flags (flags, F_XREG));
	g_code3 (op, OP_INT, ap2, ap1, ap3);
	ap1 = ap3;
    } else {
	if (tst_iconst (ep->v.p[0])) {
	    ap1 =
		g_expr (ep->v.p[1],
			(FLAGS) (F_VOL | best_flags (flags, F_XREG)));
	    ap2 = g_expr (ep->v.p[0], (FLAGS) (F_IALL | F_UNSIGNED));
	} else {
	    ap1 =
		g_expr (ep->v.p[0],
			(FLAGS) (F_VOL | best_flags (flags, F_XREG)));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_IALL | F_UNSIGNED));
	}
	validate (ap1);		/* in case push occurred */
	g_code (op, OP_INT, ap2, ap1);
	freeop (ap2);
    }
    return mk_legal (ap1, flags, OP_INT);
}

/*
 * generate a &= or a |= node.
 */
static ADDRESS *g_aslogic P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    FLAGS   flagx;
    ADDRESS *ap1, *ap2;

    if (ep->v.p[0]->nodetype == en_fieldref)
	return g_asbitfield (ep, flags, op, FALSE);
    flagx = (flags & F_NOVALUE) ? F_IALL : (FLAGS) (F_IALL | F_USES);
    ap1 = g_expr (ep->v.p[0], flagx);
    if (isanyreg (ap1)) {
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_IALL | F_UNSIGNED));
    } else {
	ap2 = g_expr (ep->v.p[1], F_XREG);
    }
    validate (ap1);
    if (isanyreg (ap1)) {
	g_code (op, OP_INT, ap2, ap1);
    } else {
	ap2 = mk_legal (ap2, (FLAGS) (F_VOL | F_XREG), OP_INT);
	g_code (op, OP_INT, ap1, ap2);
	g_code (op_sti, OP_INT, ap2, ap1);
    }
    freeop (ap2);
    return mk_legal (ap1, flags, OP_INT);
}

/*============================================================================*/

/*
 * generate code to evaluate a shift node and return the address mode of the
 * result.
 * Note: right shifts will have already been converted to negative left
 * shifts.
 */
static ADDRESS *g_shift P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    ADDRESS *ap1, *ap2, *ap3;

    if (is_op3_possible (ep->v.p[0])
	&& is_op3_possible (ep->v.p[1])) {
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_XREG | F_MEM));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_XREG | F_MEM));
	validate (ap1);
	/* Just to be sure... check again for correct op3-operands */
	if (is_op3_violated (ap1) || is_op3_violated (ap2)) {
	    FATAL ((__FILE__, "g_shift", "inconsistency int "));
	}
	switch (op) {
	case op_lsh:
	    op = op_lsh3;
	    break;
	case op_ash:
	    op = op_ash3;
	    break;
	default:
	    FATAL ((__FILE__, "g_shift", "illegal opcode %d", op));
	    break;
	}
	freeop (ap2);
	freeop (ap1);
	ap3 = temporary_register (best_flags (flags, F_XREG));
	g_code3 (op, OP_INT, ap2, ap1, ap3);
	ap1 = ap3;
    } else {
	ap1 =
	    g_expr (ep->v.p[0], (FLAGS) (F_VOL | best_flags (flags, F_XREG)));
	ap2 = g_expr (ep->v.p[1], F_IALL);

	validate (ap1);
	g_code (op, OP_INT, ap2, ap1);
	freeop (ap2);
    }
    return mk_legal (ap1, flags, OP_INT);
}

/*
 * generate shift equals operators.
 */
static ADDRESS *g_asshift P3 (const EXPR *, ep, FLAGS, flags, OPCODE, op)
{
    FLAGS   flagx;
    ADDRESS *ap1, *ap2, *ap3;

    switch (ep->etp->type) {
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
	if (ep->v.p[0]->nodetype == en_fieldref)
	    return g_asbitfield (ep, flags, op, FALSE);
	flagx = (flags & F_NOVALUE) ? F_IALL : (FLAGS) (F_IALL | F_USES);
	ap1 = g_expr (ep->v.p[0], flagx);
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_IALL));
	validate (ap1);
	if (isanyreg (ap1)) {
	    g_code (op, OP_INT, ap2, ap1);
	} else {
	    ap3 = temporary_register (F_XREG);
	    g_code (op_ldi, OP_INT, ap1, ap3);
	    g_code (op, OP_INT, ap2, ap3);
	    g_code (op_sti, OP_INT, ap3, ap1);
	    freeop (ap3);
	}
	freeop (ap2);
	return mk_legal (ap1, flags, OP_INT);
    default:
	FATAL ((__FILE__, "g_asshift", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*============================================================================*/

/*
 * generate code to evaluate a divide operator
 */
static ADDRESS *g_div P3 (const EXPR *, ep, FLAGS, flags, BOOL, mod)
{

    BOOL    is_signed = FALSE;
    ADDRESS *ap1, *ap2;

    switch (ep->etp->type) {
    case bt_char:
    case bt_schar:
    case bt_short:
    case bt_int16:
    case bt_int32:
    case bt_long:
	is_signed = TRUE;
	/*FALLTHRU */
    case bt_uchar:
    case bt_charu:
    case bt_ushort:
    case bt_uint16:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	temp_inv ();
	ap1 = g_expr (ep->v.p[0], F_IALL);
	ap2 = g_expr (ep->v.p[1], F_IALL);
	validate (ap1);
	freeop (ap2);
	freeop (ap1);
	if (ap2->preg == REG_R0) {
	    /* bad luck, parameter is using space for ap1 */
	    if (ap1->preg == REG_R1) {
		/* its worse, a2 is in r0 and a1 is in r1 */
		g_code (op_ldi, OP_INT, ap2, mk_reg (REG_AR0));
		g_code (op_ldi, OP_INT, ap1, mk_reg (REG_R0));
		g_code (op_ldi, OP_INT, mk_reg (REG_AR0), mk_reg (REG_R1));
	    } else {
		g_code (op_ldi, OP_INT, ap2, mk_reg (REG_R1));
		g_code (op_ldi, OP_INT, ap1, mk_reg (REG_R0));
	    }
	} else {
	    g_code (op_ldi, OP_INT, ap1, mk_reg (REG_R0));
	    g_code (op_ldi, OP_INT, ap2, mk_reg (REG_R1));
	}
	if (is_signed == FALSE) {
	    call_library_r0_r1 (mod ? psup_ulrem : psup_uldiv);
	} else {
	    call_library_r0_r1 (mod ? psup_lrem : psup_ldiv);
	}
	return func_result (flags, 0L, ep->etp);
    case bt_float:
    case bt_double:
    case bt_longdouble:
	temp_inv ();
	ap1 = g_expr (ep->v.p[0], F_FALL);
	ap2 = g_expr (ep->v.p[1], F_FALL);
	validate (ap1);
	freeop (ap2);
	freeop (ap1);
	if (ap2->preg == REG_R0) {
	    /* bad luck, parameter is using space for ap1 */
	    if (ap1->preg == REG_R1) {
		/* its worse, a2 is in r0 and a1 is in r1 */
		/* could be solved with parallel addressing */
		g_code (op_ldf, OP_INT, ap2, mk_freg (REG_R2));
		g_code (op_ldf, OP_INT, ap1, mk_freg (REG_R0));
		g_code (op_ldf, OP_INT, mk_freg (REG_R2), mk_freg (REG_R1));
	    } else {
		g_code (op_ldf, OP_INT, ap2, mk_freg (REG_R1));
		g_code (op_ldf, OP_INT, ap1, mk_freg (REG_R0));
	    }
	} else {
	    g_code (op_ldf, OP_INT, ap1, mk_freg (REG_R0));
	    g_code (op_ldf, OP_INT, ap2, mk_freg (REG_R1));
	}
	call_library_r0_r1 (mod ? psup_lrem : psup_fpdiv);
	return func_result (flags, 0L, ep->etp);
    default:
	FATAL (
	       (__FILE__, "g_div", "%d: illegal type %d", mod,
		ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate /= and %= nodes.
 */
static ADDRESS *g_asdiv P3 (const EXPR *, ep, FLAGS, flags, BOOL, mod)
{
    switch (ep->etp->type) {
    case bt_char:
    case bt_schar:
    case bt_short:
    case bt_int16:
    case bt_int32:
    case bt_long:
	if (mod) {
	    return as_fcall (ep, flags, psup_lrem, OP_INT);
	} else {
	    return as_fcall (ep, flags, psup_ldiv, OP_INT);
	}
    case bt_charu:
    case bt_uchar:
    case bt_ushort:
    case bt_uint16:
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	if (mod) {
	    return as_fcall (ep, flags, psup_ulrem, OP_INT);
	} else {
	    return as_fcall (ep, flags, psup_uldiv, OP_INT);
	}
    case bt_float:
    case bt_double:
    case bt_longdouble:
	if (mod) {
	    return as_fcall (ep, flags, psup_fprem, OP_FLOAT);
	} else {
	    return as_fcall (ep, flags, psup_fpdiv, OP_FLOAT);
	}
    default:
	FATAL (
	       (__FILE__, "asdiv", "%d: illegal type %d", mod,
		ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*============================================================================*/


static void g_inline_multiply P3 (ADDRESS *, ap1, ADDRESS *, ap2, int,
				  is_signed)
{
    ADDRESS *ap3, *ap4, *ap5;

    ap3 = temporary_register (F_XREG);
    ap4 = temporary_register (F_XREG);
    if (is_signed == FALSE) {
	/* get signum */
	ap5 = temporary_register (F_XREG);
	/*
	 * validates are not nescessary, since there must be enough
	 * registers availlable, if not, this is a compilererror
	 * and the validates will catch it and produce a fatal-error 
	 */
	validate (ap2);
	validate (ap1);
	g_code3 (op_xor3, OP_INT, ap2, ap1, ap5);
	g_code (op_absi, OP_INT, ap1, ap1);
	g_code (op_absi, OP_INT, ap2, ap2);
    }
    g_code (op_ldi, OP_INT, mk_immed (-16), ap4);
    g_code3 (op_lsh3, OP_INT, ap4, ap1, ap3);
    g_code3 (op_lsh3, OP_INT, ap4, ap2, ap4);
    g_code (op_and, OP_INT, mk_uimmed (0xFFFFL), ap1);
    g_code (op_and, OP_INT, mk_uimmed (0XFFFFL), ap2);
    g_code (op_mpyi, OP_INT, ap1, ap4);
    g_code (op_mpyi, OP_INT, ap2, ap3);
    g_code (op_mpyi, OP_INT, ap1, ap2);
    g_code (op_addi, OP_INT, ap4, ap3);
    g_code (op_lsh, OP_INT, mk_immed (16), ap3);
    g_code3 (op_addi3, OP_INT, ap3, ap2, ap1);
    if (is_signed == FALSE) {
	/* get signum */
	g_code (op_negi, OP_INT, ap1, ap2);
	g_code3 (op_tstb3, OP_INT, ap5, ap5, NIL_ADDRESS);
	g_code (op_ldin, OP_INT, ap2, ap1);
	freeop (ap5);
    }
    freeop (ap4);
    freeop (ap3);
}


static ADDRESS *g_mul P2 (const EXPR *, ep, FLAGS, flags)
{
    int     is_signed = TRUE;

    ADDRESS *ap1, *ap2, *ap3;

    switch (ep->etp->type) {
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	is_signed = FALSE;
	/*FALLTHRU */
    case bt_long:
    case bt_int32:
	if (opt_true_long != OPT_NO) {
	    ap1 =
		g_expr (ep->v.p[0],
			(FLAGS) (F_VOL | best_flags (flags, F_XREG)));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_VOL | F_XREG));
	    validate (ap1);
	    g_inline_multiply (ap1, ap2, is_signed);
	    freeop (ap2);
	    return mk_legal (ap1, flags, OP_INT);
	}
	/*FALLTHRU */
    case bt_char:
    case bt_schar:
    case bt_charu:
    case bt_uchar:
    case bt_short:
    case bt_int16:
    case bt_ushort:
    case bt_uint16:
	if (is_op3_possible (ep->v.p[0])
	    && is_op3_possible (ep->v.p[1])) {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_XREG | F_MEM));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_XREG | F_MEM));
	    validate (ap1);
	    /* Just to be sure... check again for correct op3-operands */
	    if (is_op3_violated (ap1) || is_op3_violated (ap2)) {
		FATAL ((__FILE__, "g_mul", "inconsistency int "));
	    }
	    freeop (ap2);
	    freeop (ap1);
	    ap3 = temporary_register (best_flags (flags, F_XREG));
	    g_code3 (op_mpyi3, OP_INT, ap2, ap1, ap3);
	    ap1 = ap3;
	} else {
	    if (tst_iconst (ep->v.p[0])) {
		ap1 =
		    g_expr (ep->v.p[1],
			    (FLAGS) (F_VOL | best_flags (flags, F_XREG)));
		ap2 = g_expr (ep->v.p[0], F_IALL);
	    } else {
		ap1 =
		    g_expr (ep->v.p[0],
			    (FLAGS) (F_VOL | best_flags (flags, F_XREG)));
		ap2 = g_expr (ep->v.p[1], F_IALL);
	    }
	    validate (ap1);	/* in case push occurred */
	    g_code (op_mpyi, OP_INT, ap2, ap1);
	    freeop (ap2);
	}
	return mk_legal (ap1, flags, OP_INT);
    case bt_float:
    case bt_double:
    case bt_longdouble:

	if (is_op3_possible (ep->v.p[0])
	    && is_op3_possible (ep->v.p[1])) {
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_FREG | F_MEM));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_FREG | F_MEM));
	    validate (ap1);
	    /* Just to be sure... check again for correct op3-operands */
	    if (is_op3_violated (ap1) || is_op3_violated (ap2)) {
		FATAL ((__FILE__, "g_mul", "inconsistency float "));
	    }
	    freeop (ap2);
	    freeop (ap1);
	    ap3 = temporary_register (F_FREG);
	    g_code3 (op_mpyf3, OP_FLOAT, ap2, ap1, ap3);
	    ap1 = ap3;
	} else {
	    if (ep->v.p[0]->nodetype == en_fcon) {
		ap1 = g_expr (ep->v.p[1], (FLAGS) (F_VOL | F_FREG));
		ap2 = g_expr (ep->v.p[0], F_FALL);
	    } else {
		ap1 = g_expr (ep->v.p[0], (FLAGS) (F_VOL | F_FREG));
		ap2 = g_expr (ep->v.p[1], F_FALL);
	    }
	    validate (ap1);	/* in case push occurred */
	    g_code (op_mpyf, OP_FLOAT, ap2, ap1);
	    freeop (ap2);
	}
	return mk_legal (ap1, flags, OP_FLOAT);
    default:
	FATAL ((__FILE__, "g_mul", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*
 * generate a *= node.
 */
static ADDRESS *g_asmul P2 (const EXPR *, ep, FLAGS, flags)
{
    int     is_signed = TRUE;
    ADDRESS *ap1, *ap2, *ap3, *ap6;

    switch (ep->etp->type) {
    case bt_int32:
    case bt_long:
	is_signed = FALSE;
	/*FALLTHRU */
    case bt_uint32:
    case bt_ulong:
    case bt_pointer32:
	if (opt_true_long != OPT_NO) {
	    if (ep->v.p[0]->nodetype == en_fieldref)
		return g_asbitfield (ep, flags, op_mpyi, FALSE);
	    ap1 = g_expr (ep->v.p[0], (FLAGS) (F_IALL | F_USES));
	    ap2 = g_expr (ep->v.p[1], (FLAGS) (F_XREG | F_VOL));
	    if (isanyreg (ap1)) {
		validate (ap1);
		ap6 = NULL;
	    } else {
		ap6 = ap1;
		ap1 = temporary_register (F_XREG);
		validate (ap6);
		g_code (op_ldi, OP_INT, ap6, ap1);
	    }
	    validate (ap2);
	    g_inline_multiply (ap1, ap2, is_signed);
	    if (ap6 != NULL) {
		freeop (ap1);
		validate (ap6);
		g_code (op_sti, OP_INT, ap1, ap6);
		ap1 = ap6;
	    }
	    freeop (ap2);
	    return mk_legal (ap1, flags, OP_INT);
	}
	/*FALLTHRU */
    case bt_char:
    case bt_schar:
    case bt_charu:
    case bt_uchar:
    case bt_short:
    case bt_int16:
    case bt_ushort:
    case bt_uint16:
	if (ep->v.p[0]->nodetype == en_fieldref)
	    return g_asbitfield (ep, flags, op_mpyi, FALSE);
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_IALL | F_USES));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_IALL));
	if (isanyreg (ap1)) {
	    validate (ap1);
	    g_code (op_mpyi, OP_INT, ap2, ap1);
	    freeop (ap2);
	} else {
	    ap3 = temporary_register (F_XREG);
	    validate (ap1);
	    g_code (op_ldi, OP_INT, ap1, ap3);
	    g_code (op_mpyi, OP_INT, ap2, ap3);
	    g_code (op_sti, OP_INT, ap3, ap1);
	    freeop (ap3);
	    freeop (ap2);
	}
	return mk_legal (ap1, flags, OP_INT);
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap1 =
	    g_expr (ep->v.p[0], (FLAGS) (F_FREG | F_MEM | F_IMMED | F_USES));
	ap2 = g_expr (ep->v.p[1], (FLAGS) (F_FREG | F_MEM | F_IMMED));
	if ((ap1->mode != am_freg)) {
	    ap3 = data_register (F_FREG);
	    validate (ap1);
	    g_code (op_ldf, OP_FLOAT, ap1, ap3);
	    g_code (op_mpyf, OP_FLOAT, ap2, ap3);
	    g_code (op_stf, OP_FLOAT, ap3, ap1);
	    freeop (ap3);
	    freeop (ap2);
	} else {
	    validate (ap1);
	    g_code (op_mpyf, OP_FLOAT, ap2, ap1);
	    freeop (ap2);
	}
	return mk_legal (ap1, flags, OP_FLOAT);
    default:
	FATAL ((__FILE__, "asmul", "illegal type %d", ep->etp->type));
	break;
    }
    return NIL_ADDRESS;
}

/*============================================================================*/

/*
 * generate code to evaluate a condition operator node (?:)
 */
static ADDRESS *g_hook P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2;
    EXPRTYPE typ1, typ2;
    FLAGS   flagx;
    BOOL    result_is_void = FALSE;
    ITYPE   type = OP_INT;
    SIZE    offset;
    LABEL   false_label = nextlabel++;
    LABEL   end_label = nextlabel++;

    switch (ep->etp->type) {
    case bt_void:
	result_is_void = TRUE;
	flagx = (FLAGS) (F_ALL | F_NOVALUE);
	break;
    case bt_float:
    case bt_double:
    case bt_longdouble:
	flagx = (FLAGS) (F_FREG | F_VOL);
	type = OP_FLOAT;
	break;
    default:
	flagx = (FLAGS) (best_flags (flags, F_XREG) | F_VOL);
	break;
    }

    typ1 = ep->v.p[1]->v.p[0]->nodetype;
    typ2 = ep->v.p[1]->v.p[1]->nodetype;
    if (((typ1 == en_autocon)
	 || (typ1 == en_fcon) || (typ1 == en_register)
	 || (is_short (ep->v.p[1]->v.p[0]))
	 || ((opt_const_in_ram != OPT_NO) && (typ1 == en_icon)))
	&& ((typ2 == en_autocon)
	    || (typ2 == en_fcon) || (typ2 == en_register)
	    || (is_short (ep->v.p[1]->v.p[1]))
	    || ((opt_const_in_ram != OPT_NO) && (typ2 == en_icon)))
	&& (ep->v.p[0]->nodetype != en_land)
	&& (ep->v.p[0]->nodetype != en_lor)) {

	/* Sequence of ap1 and ap2 is important, validate is used in */
	/* routine g_conditionalload                                 */

	ap1 = g_expr (ep->v.p[1]->v.p[0], (type == OP_INT) ? F_IALL : F_FALL);
	ap2 = g_expr (ep->v.p[1]->v.p[1], (type == OP_INT) ? F_IALL : F_FALL);
	if (((ap1->mode == am_dreg) && is_temporary_register (ap1->preg))
	    || ((ap2->mode == am_dreg) && is_temporary_register (ap2->preg))) {
	    FATAL ((__FILE__, "g_hook", "INCONSISTENCY Dreg"));
	}
	ap2 = g_conditionalload (ep->v.p[0], ap1, ap2, type, flagx);
    } else {
	temp_inv ();		/* I do not think I can avoid that */
	offset = stack_offset;
	stack_offset = 0L;

	/* all scratch registers are void */

	g_falsejp (ep->v.p[0], false_label);
	ep = ep->v.p[1];

	/* all scratch registers are void */

	ap1 = g_expr (ep->v.p[0], flagx);
	freeop (ap1);

	/* all scratch registers are void */

	g_branch (end_label);
	g_label (false_label);

	ap2 = g_expr (ep->v.p[1], flagx);
	if (!result_is_void && !is_equal_address (ap1, ap2))
	    FATAL ((__FILE__, "g_hook", "INCONSISTENCY"));

	g_label (end_label);

	g_stack (stack_offset);
	stack_offset = offset;
    }
    return mk_legal (ap2, flags, type);
}

/*
 * rotate a bitfield into the required position (assumes ap is in a register)
 * Offset determines direction of shift, >= 0 bitfield is isolated and shiftet
 * to left, signum is maintained through combination lsh <<, ash >>
 *
 * Offset < 0 Bitfield is shifted to right, extrabits are not masked out
 */
static void g_rotate P4 (ADDRESS *, ap, int, offset, TYP *, tp, int, width)
{
    int     w;

    if (offset >= 0) {
	switch (tp->type) {
	case bt_char:
	case bt_schar:
	case bt_short:
	case bt_int16:
	case bt_int32:
	case bt_long:
	    /* signed bitfield */
	    w = (int) (32 - offset - width);
	    if (w != 0) {
		g_immed (op_lsh, (long) w, ap);
	    }
	    w = (int) (32 - width);
	    if (w != 0) {
		g_immed (op_ash, (long) -w, ap);
	    }
	    break;

	default:
	    /* unsigned bitfield */
	    /* shift has same costs as and, thanks barrelshifter */
	    if ((offset == 0) && (width <= 16)) {
		g_uimmed (op_and, (IVAL) bitmask ((BITSIZE) width), ap);
	    } else {
		w = (int) (32 - offset - width);
		if (w != 0) {
		    g_immed (op_lsh, (long) w, ap);
		}
		w = (int) (32 - width);
		if (w != 0) {
		    g_immed (op_lsh, (long) -w, ap);
		}
	    }
	    break;
	}
    } else {
	FATAL ((__FILE__, "g_rotate", "negative offset"));
	g_immed (op_lsh, (long) offset, ap);
    }
}


/*
 * generate the code for assign operators in bitfield
 * Swap determines 1 by divide if modulo or div is wished,
 * by ++/-- if posttype or pre-type (returns modified or standardvalue)
 */
static ADDRESS *g_asbitfield P4 (const EXPR *, ep, FLAGS, flags, OPCODE, op,
				 BOOL, postoperation)
{
#ifdef NEW_BITFIELDSTUFF
    ADDRESS *ap5;

#endif
    ADDRESS *ap1, *ap2, *ap3, *ap4;
    EXPR   *ep1, *lnode = ep->v.p[0];
    int     width = (int) lnode->v.bit.width;
    int     offset = (int) lnode->v.bit.offset;
    UVAL    mask;

#ifdef NEW_BITFIELDSTUFF
    if ((flags & F_NOVALUE) == F_NONE) {
	ap5 = temporary_register (best_flags (flags, F_XREG));
    }
#endif
    ap4 = temporary_register (F_XREG);
    /* Evaluate the address of the LHS */
    ep1 = mk_ref (lnode->v.bit.p, tp_pointer);
    ap2 = g_expr (ep1, F_MEM);


    /* Now get the value of the LHS, rotate and mask out unwanted bits */
    ap1 = temporary_register (F_XREG);

    g_code (op_ldi, OP_INT, ap2, ap1);
    /* Save the readed value for write back */

    validate (ap4);
    g_code (op_ldi, OP_INT, ap1, ap4);

    g_rotate (ap1, offset, lnode->etp, width);
#ifdef NEW_BITFIELDSTUFF
    if (((flags & F_NOVALUE) == F_NONE) && (postoperation)) {
	validate (ap5);
	g_code (op_ldi, OP_INT, ap1, ap5);
    }
#endif
    /* evaluate the RHS */
    ap3 = g_expr (ep->v.p[1], (FLAGS) (F_XREG | F_IMMED));
    validate (ap1);
    validate (ap2);

    /* now do the operation, masking the result back to the required size */
    switch (op) {
    case op_divs:
    case op_divu:
	FATAL ((__FILE__, "g_asbitfield", "div is illegal"));
	break;
    default:
	g_code (op, OP_INT, ap3, ap1);
	break;
    }
    freeop (ap3);
#ifdef NEW_BITFIELDSTUFF
    if ((width + offset) <= 15) {
	ap3 = mk_immed ((IVAL) bitmask ((BITSIZE) width));
    } else {
#endif
	ap3 = temporary_register (F_XREG);
	if (width <= 15) {
	    /* must be less than 16 bit sinc ldi immed is signed */
	    mask = bitmask ((BITSIZE) width);
	    g_immed (op_ldi, (IVAL) mask, ap3);
	} else {
	    g_code (op_ldi, OP_INT, mk_immed (-1L), ap3);
	    g_immed (op_lsh, (IVAL) -(32 - width), ap3);
	}
#ifdef NEW_BITFIELDSTUFF
    }
#endif

    validate (ap4);
#ifdef NEW_BITFIELDSTUFF
    if (((flags & F_NOVALUE) == F_NONE) && (!postoperation)) {
	/* 
	 * mask out the wanted bits and shift to correct position 
	 * and make a copy with correct sign as returnvalue (ap5)
	 */
	validate (ap5);
	g_immed (op_lsh, (IVAL) (32 - width), ap1);
	g_code (op_ldi, OP_INT, ap1, ap5);
	if (is_signed_type (lnode->etp)) {
	    g_immed (op_ash, (IVAL) -(32 - width), ap5);
	} else {
	    g_immed (op_lsh, (IVAL) -(32 - width), ap5);
	}
	g_immed (op_lsh, (IVAL) -(32 - width - offset), ap1);
	if (offset != 0) {
	    if ((width + offset) <= 15) {
		freeop (ap3);
		ap3 = mk_immed ((IVAL) (bitmask ((BITSIZE) width) << offset));
	    } else {
		g_immed (op_lsh, (IVAL) offset, ap3);
	    }
	}
    } else {
	/* mask out the wanted bits and shift to correct position */
	g_code (op_and, OP_INT, ap3, ap1);
	if (offset != 0) {
	    g_immed (op_lsh, (IVAL) offset, ap1);
	    if ((width + offset) <= 15) {
		freeop (ap3);
		ap3 = mk_immed ((IVAL) (bitmask ((BITSIZE) width) << offset));
	    } else {
		g_immed (op_lsh, (IVAL) offset, ap3);
	    }
	}
    }
#else
    g_code (op_and, OP_INT, ap3, ap1);
    if (offset != 0) {
	g_immed (op_lsh, offset, ap1);
	g_immed (op_lsh, offset, ap3);
    }
#endif
    validate (ap4);
    g_code (op_andn, OP_INT, ap3, ap4);
    g_code (op_or, OP_INT, ap1, ap4);
    g_code (op_sti, OP_INT, ap4, ap2);

    freeop (ap3);
    freeop (ap1);
    freeop (ap2);
    freeop (ap4);

#ifdef NEW_BITFIELDSTUFF
    if ((flags & F_NOVALUE) == F_NONE) {
	validate (ap5);
	ap1 = ap5;
    } else {
	ap1 = NIL_ADDRESS;
    }

#else
    /* return a result */
    ap2 = temporary_register (F_XREG);
    g_code (op_ldi, OP_INT, ap1, ap2);
    ap1 = ap2;

    if ((flags & F_NOVALUE) == F_NONE) {
	/* result value needed */
	/* get value with correct sign */
	g_rotate (ap1, offset, lnode->etp, width);
	if (postoperation) {
	    /* post increment/decrement restore original value */
	    switch (op) {
	    case op_addi:
		op = op_subi;
		g_code (op, OP_INT, mk_immed (1L), ap1);
		/* get value with correct sign and significant bits */
		g_rotate (ap1, 0, lnode->etp, width);
		break;
	    case op_subi:
		op = op_addi;
		g_code (op, OP_INT, mk_immed (1L), ap1);
		/* get value with correct sign and significant bits */
		g_rotate (ap1, 0, lnode->etp, width);
		break;
	    default:
		break;
	    }
	}
    }
#endif
    return mk_legal (ap1, flags, OP_INT);
}


/*
 * assign structure from ap1 to ap2
 * ap1, ap2 are scratch address registers
 */
static void structassign P3 (ADDRESS *, ap1, ADDRESS *, ap2, SIZE, size)
{
    SIZE    loop;
    ADDRESS *ap3;
    SIZE    i;

    if (size == 0L) {
	FATAL ((__FILE__, "structassign", "size = 0"));
    }
    loop = size;
    ap1 = copy_addr (ap1, am_ainc);
    ap1->u.offset = mk_const (1);
    ap2 = copy_addr (ap2, am_ainc);
    ap2->u.offset = mk_const (1);
    ap3 = data_register (F_DREG);

    /* Fetch first word to init prallelmode */
    g_code (op_ldi, OP_INT, ap1, ap3);

    /* Short loops we do manually */
    if (loop <= 3) {
	for (i = 1; i <= loop - 1; i++) {
	    g_code_parallel (op_ldi_sti, OP_INT, ap1, NIL_ADDRESS, ap3,
			     ap3, NIL_ADDRESS, ap2);
	}
    } else {
	/* check if we have to save the blockrepeatregisters */
#if 0
	save_blockrepeat ();	/* for dbra */
#endif
	allocate_blockrepeat_registers ();
	/* repeat copy instruction loop-2 times */
	g_uimmed (op_rpts, loop - 2, NIL_ADDRESS);
	g_code_parallel (op_ldi_sti, OP_INT, ap1, NIL_ADDRESS, ap3,
			 ap3, NIL_ADDRESS, ap2);
#if 0
	restore_blockrepeat ();
#endif
    }
    /* finish the last cycle */
    g_code (op_sti, OP_INT, ap3, ap2);
    freeop (ap3);
}

/*
 * generate code for an assignment node.
 */
static ADDRESS *g_assign P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1, *ap2, *ap3, *ap4;
    EXPR   *ep1;
    TYP    *tp = ep->etp;
    SIZE    size = ep->etp->size;
    ITYPE   type = OP_INT;
    UVAL    mask;
    FLAGS   flagf, flagi;

    flagf = (flags & F_NOVALUE) ? F_FALL : (FLAGS) (F_FALL | F_USES);
    flagi = (flags & F_NOVALUE) ? F_IALL : (FLAGS) (F_IALL | F_USES);
    switch (tp->type) {
    case bt_pointer32:
	if (!is_array_type (tp) && !is_array_assignment (tp)) {
	    goto common;
	}
	/*FALLTHRU */
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
	    structassign (ap1, ap2, ep->etp->size);
	    freeop (ap2);
	    freeop (ap1);
	    return NIL_ADDRESS;
	}
	ap3 = address_register ();
	g_code (op_ldi, OP_INT, ap1, ap3);
	structassign (ap3, ap2, size);
	freeop (ap3);
	freeop (ap2);
	validate (ap1);
	return mk_legal (ap1, flags, OP_INT);


    case bt_float:
    case bt_double:
    case bt_longdouble:
	switch (ep->v.p[0]->nodetype) {
	case en_fieldref:
	    FATAL ((__FILE__, "g_assign", "en_fieldref for float"));
	    /*NOTREACHED */

	    /*
	     * we want to pass the right hand side as the expression value.
	     * This can''t be done if the left side is a register variable
	     * on which the right hand side addressing mode depends. But if
	     * the left side IS a register variable, it is desirable to pass
	     * the left side, so no problem.
	     */

	case en_register:
	    /* pass the left side as expr. value */
	    ap1 = g_expr (ep->v.p[0], flagf);
	    ap2 = g_expr (ep->v.p[1], F_FALL);
	    validate (ap1);
	    switch (ap1->mode) {
	    case am_areg:
	    case am_dreg:
	    case am_ireg:
	    case am_sreg:
		FATAL ((__FILE__, "g_assign", "Integer mixed with float"));
		break;
	    case am_freg:
		g_code (op_ldf, OP_FLOAT, ap2, ap1);
		type = OP_FLOAT;
		break;
	    default:
		switch (ap2->mode) {
		case am_areg:
		case am_dreg:
		case am_ireg:
		case am_sreg:
		    FATAL (
			   (__FILE__, "g_assign",
			    "Integer mixed with float"));
		    break;
		case am_freg:
		    g_code (op_stf, OP_FLOAT, ap2, ap1);
		    type = OP_FLOAT;
		    break;
		default:
		    /* we can not store memory to memory directly */
		    ap3 = temporary_register (F_FREG);
		    g_code (op_ldf, OP_FLOAT, ap2, ap3);
		    g_code (op_stf, OP_FLOAT, ap3, ap1);
		    type = OP_FLOAT;
		    freeop (ap3);
		    break;
		}
		break;
	    }
	    freeop (ap2);
	    return mk_legal (ap1, flags, OP_FLOAT);

	default:
	    /* pass the right side as expr. value */
	    /* normally, this is more efficient */
	    ap1 = g_expr (ep->v.p[1], flagf);
	    ap2 = g_expr (ep->v.p[0], F_FALL);
	    validate (ap1);
	    switch (ap2->mode) {
	    case am_areg:
	    case am_dreg:
	    case am_ireg:
	    case am_sreg:
		FATAL ((__FILE__, "g_assign", "Integer mixed with float"));
		break;
	    case am_freg:
		g_code (op_ldf, OP_FLOAT, ap1, ap2);
		type = OP_FLOAT;
		break;
	    default:
		switch (ap1->mode) {
		case am_areg:
		case am_dreg:
		case am_ireg:
		case am_sreg:
		    FATAL (
			   (__FILE__, "g_assign",
			    "Integer mixed with float"));
		    break;
		case am_freg:
		    type = OP_FLOAT;
		    g_code (op_stf, OP_FLOAT, ap1, ap2);
		    break;
		default:
		    /* we can not store memory to memory directly */
		    ap3 = temporary_register (F_FREG);
		    g_code (op_ldf, OP_FLOAT, ap1, ap3);
		    g_code (op_stf, OP_FLOAT, ap3, ap2);
		    freeop (ap3);
		    break;
		}
		break;
	    }
	    freeop (ap2);
	    return mk_legal (ap1, flags, type);
	}

    default:
      common:
	switch (ep->v.p[0]->nodetype) {
	case en_fieldref:
	    /*
	     * Field assignment
	     */
	    /* get the value */
	    mask = bitmask (ep->v.p[0]->v.bit.width);
	    ap1 = g_expr (ep->v.p[1], (FLAGS) (F_IMMED | F_XREG | F_VOL));
	    if (ap1->mode == am_immed) {
		ap1->u.offset->v.i &= mask;
		ap3 =
		    mk_uimmed ((ap1->u.offset->v.i) << ep->v.p[0]->v.
			       bit.offset);
	    } else {
		if (flags & F_NOVALUE) {
		    g_uimmed (op_and, mask, ap1);
		    ap3 = ap1;
		    if (ep->v.p[0]->v.bit.offset != 0) {
			g_immed (op_lsh, ep->v.p[0]->v.bit.offset, ap3);
		    }
		} else {
		    /* 
		     * mask out the wanted bits and shift to correct position 
		     * and make a copy with correct sign as returnvalue (ap5)
		     */
		    g_immed (op_lsh, (32 - ep->v.p[0]->v.bit.width), ap1);
		    ap3 = temporary_register (F_XREG);
		    g_code (op_ldi, OP_INT, ap1, ap3);
		    if (is_signed_type (tp)) {
			g_immed (op_ash, -(32 - ep->v.p[0]->v.bit.width),
				 ap1);
		    } else {
			g_immed (op_lsh, -(32 - ep->v.p[0]->v.bit.width),
				 ap1);
		    }
		    g_immed (op_lsh,
			     -(32 - ep->v.p[0]->v.bit.width -
			       ep->v.p[0]->v.bit.offset), ap3);
		}
	    }
	    mask <<= ep->v.p[0]->v.bit.offset;
	    ep1 = mk_ref (ep->v.p[0]->v.bit.p, tp_pointer);
	    ap2 = g_expr (ep1, F_MEM);
	    validate (ap3);
	    ap4 = temporary_register (F_XREG);
	    g_code (op_ldi, OP_INT, ap2, ap4);
	    g_uimmed (op_andn, mask, ap4);
	    g_code (op_or, OP_INT, ap3, ap4);
	    g_code (op_sti, OP_INT, ap4, ap2);
	    freeop (ap4);
	    freeop (ap2);
	    if (!(flags & F_NOVALUE)) {
		freeop (ap3);
		validate (ap1);
	    }
	    return mk_legal (ap1, flags, OP_INT);

	    /*
	     * we want to pass the right hand side as the expression value.
	     * This can''t be done if the left side is a register variable
	     * on which the right hand side addressing mode depends. But if
	     * the left side IS a register variable, it is desirable to pass
	     * the left side, so no problem.
	     */
	case en_register:
	    /* pass the left side as expr. value */
	    ap1 = g_expr (ep->v.p[0], flagi);
	    ap2 = g_expr (ep->v.p[1], F_IALL);
	    validate (ap1);
	    switch (ap1->mode) {
	    case am_areg:
	    case am_dreg:
	    case am_ireg:
	    case am_sreg:
		g_code (op_ldi, OP_INT, ap2, ap1);
		break;
	    case am_freg:
		FATAL ((__FILE__, "g_assign", "Float mixed with integer"));
		break;
	    default:
		switch (ap2->mode) {
		case am_areg:
		case am_dreg:
		case am_ireg:
		case am_sreg:
		    g_code (op_sti, OP_INT, ap2, ap1);
		    break;
		case am_freg:
		    FATAL (
			   (__FILE__, "g_assign",
			    "Float mixed with integer"));
		    break;
		default:
		    /* we can not store memory to memory directly */
		    ap3 = temporary_register (F_XREG);
		    g_code (op_ldi, OP_INT, ap2, ap3);
		    g_code (op_sti, OP_INT, ap3, ap1);
		    freeop (ap3);
		    break;
		}
		break;
	    }
	    freeop (ap2);
	    return mk_legal (ap1, flags, OP_INT);
	default:
	    /* pass the right side as expr. value */
	    /* normally, this is more efficient */
	    ap1 = g_expr (ep->v.p[1], flagi);
	    ap2 = g_expr (ep->v.p[0], F_IALL);
	    validate (ap1);
	    switch (ap2->mode) {
	    case am_areg:
	    case am_dreg:
	    case am_ireg:
	    case am_sreg:
		g_code (op_ldi, OP_INT, ap1, ap2);
		break;
	    case am_freg:
		FATAL ((__FILE__, "g_assign", "Float mixed with integer"));
		break;
	    default:
		switch (ap1->mode) {
		case am_areg:
		case am_dreg:
		case am_ireg:
		case am_sreg:
		    g_code (op_sti, OP_INT, ap1, ap2);
		    break;
		case am_freg:
		    FATAL (
			   (__FILE__, "g_assign",
			    "Float mixed with integer"));
		    break;
		default:
		    /* we can not store memory to memory directly */
		    ap3 = temporary_register (F_XREG);
		    g_code (op_ldi, OP_INT, ap1, ap3);
		    g_code (op_sti, OP_INT, ap3, ap2);
		    freeop (ap3);
		    break;
		}
		break;
	    }
	    freeop (ap2);
	    return mk_legal (ap1, flags, type);
	}
    }
    return NIL_ADDRESS;
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
    case bt_struct:
    case bt_union:
	if (is_lvalue (ep)) {
	    ep = ep->v.p[0];
	}
	/* all other cases return a pointer to the struct anyway */
	/* allocate stack space */
	ap1 = address_register ();
	g_code (op_ldi, OP_INT, mk_reg (STACKPTR), ap1);
/*
 * Add 1 to pointer sinc sp points allways to top-of-stack
 * next free location on stack is sp-1
 */
	g_code (op_addi, OP_INT, mk_immed (1), ap1);

	g_immed (op_addi, size, mk_reg (STACKPTR));
	/*
	 * F_VOL was missing in the following line --
	 * it took a hard-core debugging session to find this error
	 */
	ap = g_expr (ep, (FLAGS) (F_AREG | F_VOL));
	validate (ap1);
	/* now, copy it on stack - the same as structassign */
	structassign (ap, ap1, size);
	freeop (ap);
	freeop (ap1);
	break;
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap = g_expr (ep, F_FREG);
	g_code (op_pushf, OP_FLOAT, ap, NIL_ADDRESS);
	freeop (ap);
	break;
    default:
	ap = g_expr (ep, F_XREG);
	g_code (op_push, OP_INT, ap, NIL_ADDRESS);
	freeop (ap);
	break;
    }
    return size;
}

/*
 * push a list of parameters onto the stack and return the number of
 * bytes that they occupy on the stack.
 */
static SIZE g_parms P1 (const EXPR *, plist)
{
    SIZE    size;

    is_parameter++;
    for (size = 0L; plist != NIL_EXPR; plist = plist->v.p[1]) {
	size += push_param (plist->v.p[0]);
    }
    is_parameter--;
    return size;
}

/*
   * saves a function call result in D0 it is assumed that flags contain
   * either F_DREG or F_AREG. return value is the addressing mode of the
   * result bytes is the number of bytes to pop off the stack
   *
   * This routine does not use mk_legal and takes care of the stuff itself.
 */
static ADDRESS *func_result P3 (FLAGS, flags, SIZE, bytes, TYP *, tp)
{
    ADDRESS *ap;

    stack_offset += bytes;
    if (is_parameter)
	g_stack (bytes);
    if (flags & F_NOVALUE)
	return NIL_ADDRESS;
    switch (tp->type) {
    case bt_float:
    case bt_double:
    case bt_longdouble:
	if (flags & F_FREG) {
	    ap = data_register (F_FREG);
	    g_code (op_ldf, OP_FLOAT, mk_freg (RESULT), ap);
	} else {
	    FATAL ((__FILE__, "func_result", "illegal addressing mode"));
	}
	break;
    default:
	if (flags & F_XREG) {
	    ap = temporary_register ((FLAGS) (flags & F_XREG));
	    g_code (op_ldi, OP_INT, mk_reg (RESULT), ap);
	} else {
	    FATAL ((__FILE__, "func_result", "illegal addressing mode"));
	}
	break;
    }
    return ap;
}

/* assignment operations with library calls */
/*
 * Attention, parameters and returnvalue are assumed to be of the
 * same type (either OP_FLOAT or OP_INT)
 *
 */
static ADDRESS *as_fcall P4 (const EXPR *, ep, FLAGS, flags, const CHAR *,
			     libnam1, ITYPE, type)
{
/*
 * example: libnam1 = ".ldiv"
 */
    ADDRESS *ap1, *ap2, *ap3, *ap4;
    EXPR   *lnode;
    UVAL    mask;
    int     width, offset;

    temp_inv ();
    switch (ep->v.p[0]->nodetype) {
    case en_register:
	ap2 = g_expr (ep->v.p[1], (type == OP_INT) ? F_IALL : F_FALL);

	/* ap1 cannot be destroyed, no problem */
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_XREG | F_FREG));
	validate (ap2);
	freeop (ap2);
	/* no problem at all, ap1 can't be in R1 (tempref!) */
	if (type == OP_INT) {
	    g_code (op_ldi, OP_INT, ap2, mk_reg (REG_R1));
	    g_code (op_ldi, OP_INT, ap1, mk_reg (REG_R0));
	} else {
	    g_code (op_ldf, OP_FLOAT, ap2, mk_freg (REG_R1));
	    g_code (op_ldf, OP_FLOAT, ap1, mk_freg (REG_R0));
	}
	call_library_r0_r1 (libnam1);
	/* ap1 is always valid and not equal to RESULT */
	if (type == OP_INT) {
	    g_code (op_ldi, OP_INT, mk_reg (RESULT), ap1);
	} else {
	    g_code (op_ldf, OP_FLOAT, mk_freg (RESULT), ap1);
	}
	break;
    case en_fieldref:
	lnode = ep->v.p[0];
	width = (int) lnode->v.bit.width;
	offset = (int) lnode->v.bit.offset;

	/* evaluate the address of the LHS */
	ap3 = g_deref (lnode->v.bit.p, lnode->etp, F_MEM);
	freeop (ap3);
	switch (ap3->mode) {
	case am_indx:
	    ap3 = copy_addr (ap3, am_areg);
	    ap2 = ap3;
	    if (!is_temporary_register (ap3->preg)) {
		/* is tempref register, dont overwrite */
		ap2 = address_register ();
		g_code (op_ldi, OP_INT, ap3, ap2);
	    }
	    g_immed (op_addi, ap3->u.offset->v.i, ap2);
	    break;
	case am_indx2:
	    ap3 = copy_addr (ap3, am_areg);
	    ap2 = ap3;
	    if (!is_temporary_register (ap3->preg)) {
		/* is tempref register, dont overwrite */
		ap2 = address_register ();
		g_code (op_ldi, OP_INT, ap3, ap2);
	    }
	    g_code (op_addi, OP_INT, mk_reg (ap3->sreg), ap2);
	    break;
	case am_indxs:
	    ap3 = copy_addr (ap3, am_areg);
	    ap2 = ap3;
	    if (!is_temporary_register (ap3->preg)) {
		/* is tempref register, dont overwrite */
		ap2 = address_register ();
		g_code (op_ldi, OP_INT, ap3, ap2);
	    }
	    g_code (op_subi, OP_INT, mk_reg (ap3->sreg), ap2);
	    break;
	case am_ind:
	case am_const_ind:
	    if (is_temporary_register (ap3->preg)) {
		ap3 = copy_addr (ap3, am_areg);
	    }
	    ap2 = ap3;
	    break;
	case am_direct:
	    ap3 = copy_addr (ap3, am_direct);
	    ap2 = ap3;
	    break;
	case am_const_direct:
	    ap3 = copy_addr (ap3, am_const_direct);
	    ap2 = ap3;
	    break;
	default:
	    FATAL ((__FILE__, "as_fcall", "illegal addressing mode"));
	}

	/* and save this address on the stack so that it can be used later */
	if ((ap3->mode != am_direct) && (ap3->mode != am_const_direct)
	    && (ap3->mode != am_ind) && (ap3->mode != am_const_ind)) {
	    /* a constant address must not be saved, it will not change */
	    /* during an functionscall :-) */
	    /* also a am_ind refering to a registervariable must not be saved */
	    g_code (op_push, OP_INT, ap2, NIL_ADDRESS);
	    ap2 = copy_addr (ap2, am_ind);
	}
	if (ap2 != ap3) {
	    freeop (ap2);
	}
	/* Now get the value of the LHS */
	ap1 = temporary_register (best_flags (flags, F_XREG));
	g_code (op_ldi, OP_INT, ap2, ap1);

	/* evaluate the RHS and push it onto the stack */
	ap2 = g_expr (ep->v.p[1], F_XREG);
	validate (ap1);
	/* rotate to position, mask out unwanted bits */
	g_rotate (ap1, offset, lnode->etp, width);
	freeop (ap2);
	/* do not freeop(ap1), we use it later to store result in it */
	/* so it doesn't matter if it is overwritten due functinscall */

	if (ap2->preg == REG_R0) {
	    /* bad luck, parameter is using space for ap1 */
	    if (ap1->preg == REG_R1) {
		/* its worse, a2 is in r0 and a1 is in r1 */
		/* could be solved with parallel addressing */
		g_code (op_ldi, OP_INT, ap2, mk_reg (REG_R2));
		g_code (op_ldi, OP_INT, ap1, mk_reg (REG_R0));
		g_code (op_ldi, OP_INT, mk_reg (REG_R2), mk_reg (REG_R0));
	    } else {
		g_code (op_ldi, OP_INT, ap2, mk_reg (REG_R1));
		g_code (op_ldi, OP_INT, ap1, mk_reg (REG_R0));
	    }
	} else {
	    g_code (op_ldi, OP_INT, ap1, mk_reg (REG_R0));
	    g_code (op_ldi, OP_INT, ap2, mk_reg (REG_R1));
	}
	/* now do the operation, masking the result back to the required size */
	call_library_r0_r1 (libnam1);

	/* ap1 points still to an register from above */
	g_code (op_ldi, OP_INT, mk_reg (RESULT), ap1);
	mask = bitmask ((BITSIZE) width);
	if (flags & F_NOVALUE) {
	    g_uimmed (op_and, mask, ap1);
	    ap4 = ap1;
	    if (offset != 0) {
		g_immed (op_lsh, offset, ap4);
	    }
	} else {
	    /* 
	     * mask out the wanted bits and shift to correct position 
	     * and make a copy with correct sign as returnvalue (ap1)
	     */
	    g_immed (op_lsh, (32 - width), ap1);
#ifdef NEW_BITFIELDSTUFF
	    ap4 = ap1;
	    ap1 = temporary_register (F_XREG);
	    g_code (op_ldi, OP_INT, ap4, ap1);
#else
	    ap4 = temporary_register (F_XREG);
	    g_code (op_ldi, OP_INT, ap1, ap4);
#endif
	    if (is_signed_type (lnode->etp)) {
		g_immed (op_ash, -(32 - width), ap1);
	    } else {
		g_immed (op_lsh, -(32 - width), ap1);
	    }
	    g_immed (op_lsh, -(32 - width - offset), ap4);
	}

	/* restore ap3 (ap3 is not used since push, so information */
	/* contained in it is still valid, only registers are */
	/* no more valid) */
	if ((ap3->mode != am_direct) && (ap3->mode != am_const_direct)
	    && (ap3->mode != am_ind) && (ap3->mode != am_const_ind)) {
	    ap3 = address_register ();
	    g_code (op_pop, OP_INT, NIL_ADDRESS, ap3);
	    ap3 = copy_addr (ap3, am_ind);
	}
	ap2 = temporary_register (F_XREG);
	g_code (op_ldi, OP_INT, ap3, ap2);
	/* rotate result back into position, and store */
	mask <<= offset;
	g_uimmed (op_andn, mask, ap2);
	g_code (op_or, OP_INT, ap4, ap2);
	g_code (op_sti, OP_INT, ap2, ap3);
	freeop (ap2);
	freeop (ap3);
	if (!(flags & F_NOVALUE)) {
#ifdef NEW_BITFIELDSTUFF
	    validate (ap4);
	    validate (ap1);
	    g_code (op_ldi, OP_INT, ap1, ap4);
	    freeop (ap1);
	    ap1 = ap4;
#else
	    freeop (ap4);
	    validate (ap1);
#endif
	}
	return mk_legal (ap1, flags, OP_INT);
    default:
	ap2 = g_expr (ep->v.p[1], (type == OP_INT) ? F_IALL : F_FALL);
	ap1 = g_expr (ep->v.p[0], (FLAGS) (F_IALL | F_USES));
	freeop (ap1);
	switch (ap1->mode) {
	case am_indx:
	    ap1 = copy_addr (ap1, am_areg);
	    ap3 = ap1;
	    if (!is_temporary_register (ap1->preg)) {
		/* is tempref register, dont overwrite */
		ap3 = address_register ();
		g_code (op_ldi, OP_INT, ap1, ap3);
		freeop (ap3);
	    }
	    g_immed (op_addi, ap1->u.offset->v.i, ap3);
	    break;
	case am_indx2:
	    ap1 = copy_addr (ap1, am_areg);
	    ap3 = ap1;
	    if (!is_temporary_register (ap1->preg)) {
		/* is tempref register, dont overwrite */
		ap3 = address_register ();
		g_code (op_ldi, OP_INT, ap1, ap3);
		freeop (ap3);
	    }
	    g_code (op_addi, OP_INT, mk_reg (ap1->sreg), ap3);
	    break;
	case am_indxs:
	    ap1 = copy_addr (ap1, am_areg);
	    ap3 = ap1;
	    if (!is_temporary_register (ap1->preg)) {
		/* is tempref register, dont overwrite */
		ap3 = address_register ();
		g_code (op_ldi, OP_INT, ap1, ap3);
		freeop (ap3);
	    }
	    g_code (op_subi, OP_INT, mk_reg (ap1->sreg), ap3);
	    break;
	case am_ind:
	case am_const_ind:
	    ap1 = copy_addr (ap1, am_areg);
	    ap3 = ap1;
	    break;
	case am_direct:
	    ap1 = copy_addr (ap1, am_direct);
	    ap3 = ap1;
	    break;
	case am_const_direct:
	    ap1 = copy_addr (ap1, am_const_direct);
	    ap3 = ap1;
	    break;
	default:
	    FATAL ((__FILE__, "as_fcall", "illegal addressing mode"));
	}
	/* and save this address on the stack so that it can be used later */
	if ((ap3->mode != am_direct) && (ap3->mode != am_const_direct)) {
	    /* a constant address must not be saved, it will not change */
	    /* during a function call :-) */
	    g_code (op_push, OP_INT, ap3, NIL_ADDRESS);
	    ap3 = copy_addr (ap3, am_ind);
	}
	validate (ap2);
	freeop (ap2);

	/* ap3 is an addressregister, so we can't overwrite it */
	if (type == OP_INT) {
	    g_code (op_ldi, OP_INT, ap2, mk_reg (REG_R1));
	    g_code (op_ldi, OP_INT, ap3, mk_reg (REG_R0));
	} else {
	    g_code (op_ldf, OP_FLOAT, ap2, mk_freg (REG_R1));
	    g_code (op_ldf, OP_FLOAT, ap3, mk_freg (REG_R0));
	}

	/* now do the operation  */
	call_library_r0_r1 (libnam1);


	/* restore ap1 */
	if ((ap1->mode != am_direct) && (ap1->mode != am_const_direct)) {
	    ap1 = address_register ();
	    g_code (op_pop, OP_INT, NIL_ADDRESS, ap1);
	    ap1 = copy_addr (ap1, am_ind);
	}
	if (type == OP_INT) {
	    g_code (op_sti, OP_INT, mk_reg (RESULT), ap1);
	} else {
	    g_code (op_stf, OP_FLOAT, mk_freg (RESULT), ap1);
	}
	freeop (ap1);
	break;
    }
    if (flags & F_NOVALUE)
	return NIL_ADDRESS;

    if (type == OP_INT) {
	ap1 = temporary_register (best_flags (flags, F_XREG));
	g_code (op_ldi, OP_INT, mk_reg (RESULT), ap1);
    } else {
	ap1 = temporary_register (F_FREG);
	g_code (op_ldf, OP_FLOAT, mk_freg (RESULT), ap1);
    }
    return mk_legal (ap1, flags, type);
}

/*
 * generate a function call node and return the address mode of the result.
 */
static ADDRESS *g_fcall P2 (const EXPR *, ep, FLAGS, flags)
{
    EXPR   *ep0 = ep->v.p[0];
    TYP    *tp = ep->etp;
    ADDRESS *ap, *ap2;
    SIZE    size;

    if (!is_parameter && ep->nodetype != en_call) {
	switch (stackopt_option) {

	case OPT_SAFE:
	    g_stack (stack_offset);
	    break;

	case OPT_MINIMUM:
	    /*
	     *       Perform a stack optimisation unless:
	     *       1.  The function call is via a variable
	     *       2.  The function starts with an underscore character
	     *       3.  The alloca() routine is called
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
    /* push any used addr&data temps */
    temp_inv ();
    size = g_parms (ep->v.p[1]);	/* generate parameters */
    if (is_structure_type (tp)) {
	/*
	 *   For functions returning a structure or a union, push
	 *   a pointer to the return value as additional argument.
	 *   The scratch space will be allocated in the stack frame
	 *   of the calling function.
	 */
	ap = mk_scratch (tp->size);
	ap = copy_addr (ap, am_areg);
	if (ap->u.offset->v.i != 0L) {
	    freeop (ap);
	    ap2 = temporary_register (F_XREG);
	    g_code (op_ldi, OP_INT, ap, ap2);
	    g_immed (op_addi, (long) ap->u.offset->v.i, ap2);
	    ap = ap2;
	}
	g_code (op_push, OP_INT, ap, NIL_ADDRESS);
	size += tp_pointer->size;
	freeop (ap);
    }
    if (ep->nodetype == en_call) {
	size = 0L;
    }
    /* call the function */
    switch (ep0->nodetype) {
    case en_nacon:
	/*
	 * if functionsname is __trap_nn, 00<=nn<=31
	 * we generate a trapinstruction instaed of
	 * call, may simplify interface to operatingsystem 
	 */
	ap = check_trap (ep0);
	if (ap != NIL_ADDRESS) {
	    g_code (op_trapu, OP_INT, ap, NIL_ADDRESS);
	    return func_result (flags, size, ep->etp);
	}
	/* FALLTHRU */
    case en_labcon:
	ap = mk_immediatelabel (ep0);
	break;
    default:
	ap = g_expr (ep0, F_XREG);
	freeop (ap);
	break;
    }
    if (ap->mode == am_immed) {
	g_code (op_call, OP_INT, ap, NIL_ADDRESS);
    } else {
	g_code (op_callu, OP_INT, ap, NIL_ADDRESS);
    }
    return func_result (flags, size, tp);
}

/*
 * generates code for a en_cast node
 *
 */
static ADDRESS *g_cast P4 (ADDRESS *, ap, TYP *, tp1, TYP *, tp2, FLAGS,
			   flags)
{
    ADDRESS *ap1;

    if (flags & F_NOVALUE) {
	freeop (ap);
	return NIL_ADDRESS;
    }
    if (is_same_type (tp1, tp2)) {
	/*
	 * this can happen in with the g_xmul stuff, where a cast from
	 * (u)short to long now casts from (u)short to (u)short for an 68000
	 * mulu or muls instruction.
	 * It is save to cut things short then.
	 * It should not happen with types other than (u)short, but
	 * it does not harm either.
	 */
	switch (tp1->type) {
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	    return mk_legal (ap, flags, OP_INT);
	default:
	    FATAL ((__FILE__, "g_cast", "tp1==tp2 (%d)", tp1->type));
	    break;
	}
    }
    switch (tp2->type) {
	/* switch: type to cast to */
    case bt_char:
    case bt_charu:
    case bt_schar:
    case bt_uchar:
    case bt_ushort:
    case bt_short:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	switch (tp1->type) {
	case bt_charu:
	case bt_uchar:
	case bt_char:
	case bt_schar:
	case bt_ushort:
	case bt_uint16:
	case bt_short:
	case bt_int16:
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	    return mk_legal (ap, flags, OP_INT);
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    freeop (ap);
	    ap1 = temporary_register (best_flags (flags, F_XREG));
	    g_code (op_fix, OP_FLOAT, ap, ap1);
	    ap = ap1;
	    return mk_legal (ap, flags, OP_INT);
	default:
	    break;
	}
	break;
    case bt_float:
    case bt_double:
    case bt_longdouble:
	switch (tp1->type) {
	case bt_char:
	case bt_charu:
	case bt_schar:
	case bt_uchar:
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_pointer32:
	    freeop (ap);
	    ap1 = data_register (F_FREG);
	    g_code (op_float, OP_INT, ap, ap1);
	    ap = ap1;
	    return mk_legal (ap, flags, OP_FLOAT);
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    return mk_legal (ap, flags, OP_FLOAT);
	default:
	    break;
	}
	break;
    default:
	break;
    }
    FATAL (
	   (__FILE__, "g_cast", "illegal combination type1=%d, type2=%d",
	    tp1->type, tp2->type));
    return NIL_ADDRESS;
}

#ifdef ASM
static ADDRESS *g_asm P1 (const EXPR *, ep)
{
    ADDRESS *ap = mk_expr (am_str, copynode (ep));

    g_code (op_asm, OP_INT, ap, NIL_ADDRESS);
    return NIL_ADDRESS;
}
#endif /* ASM */

/*
 * general expression evaluation. returns the addressing mode of the result.
 */
static ADDRESS *g_expr P2 (const EXPR *, ep, FLAGS, flags)
{
    ADDRESS *ap1;
    LABEL   lab0, lab1;
    ITYPE   size;
    TYP    *tp = ep->etp;
    OPCODE  op;

    if (ep == NIL_EXPR) {
	FATAL ((__FILE__, "g_expr", "ep == 0"));
    }
    if (tst_const (ep)) {
	/* check if node contains no label */
	/*
	 * F_DIRECT is set if immediate will be converted to direct
	 * so we can avoid non necessary doubleindirection
	 */
	if ((flags & F_DIRECT) || tst_iconst (ep)) {
	    /*
	     *  am_immediate is only allowed if:
	     *  -Value is short and will not be used as direct (we dont
	     *   know content of dp, so directaccess is forbidden) and
	     *   will not be used as unsigned immediate
	     *
	     *  -Value is unsigned short and will not be used as direct
	     *   (we dont know content of dp, so directaccess is forbidden)
	     *   and will not be used as signed immediate
	     *
	     *  -Value is a label or an expression containing a label
	     *   and will later be used for directacces (it is assumed
	     *   that dp points to the page of labels)
	     *
	     */

	    if ((is_short (ep) && ((flags & (F_DIRECT | F_UNSIGNED)) == 0))
		|| (is_ushort (ep)
		    && ((flags & (F_DIRECT | F_UNSIGNED)) == F_UNSIGNED))
		|| ((flags & F_DIRECT) && (!tst_iconst (ep)))) {
		ap1 = mk_expr (am_immed, copynode (ep));
	    } else {
		if ((flags & F_MEM) && (opt_const_in_ram != OPT_NO)) {
		    ap1 = mk_ilabel (copynode (ep));
		} else {
		    ap1 = temporary_register (best_flags (flags, F_XREG));
		    g_immed (op_ldi, (long) ep->v.i, ap1);
		}
	    }
	    return mk_legal (ap1, flags, OP_INT);
	} else {
	    ap1 = mk_ilabel (ep);
	    return mk_legal (ap1, flags, OP_INT);
	}

    }
#ifdef FLOAT_SUPPORT
    size = is_real_floating_type (tp) ? OP_FLOAT : OP_INT;
#else
    size = OP_INT;
#endif /* FLOAT_SUPPORT */

    switch (ep->nodetype) {
    case en_autocon:
	ap1 = address_register ();
	g_immed (op_ldi, (long) ep->v.i, ap1);
	g_code (op_addi, OP_INT, mk_reg (frameptr), ap1);
	return mk_legal (ap1, flags, size);
    case en_register:
	ap1 = mk_reg ((REG) (ep->v.r & (~FLOAT_REG)));
	if ((ep->v.r & FLOAT_REG) != 0) {
	    ap1->mode = am_freg;
	}
	return mk_legal (ap1, flags, size);
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
	if (is_structure_type (tp) || is_array_type (tp)) {
	    return mk_legal (ap1, flags, OP_INT);
	} else {
	    return mk_legal (ap1, flags, size);
	}
    case en_fieldref:
	return g_fderef (ep, flags);
    case en_uminus:
	return g_unary (ep, flags, op_negi);
    case en_compl:
	return g_unary (ep, flags, op_not);
    case en_add:
	return g_addsub (ep, flags, op_addi);
    case en_sub:
	return g_addsub (ep, flags, op_subi);
    case en_and:
	if (ep->v.p[1]->nodetype == en_compl) {
	    EXPR   *ep1 = (EXPR *) ep;

	    ep1->v.p[1] = ep1->v.p[1]->v.p[0];
	    return g_logic (ep1, flags, op_andn);
	} else {
	    return g_logic (ep, flags, op_and);
	}
    case en_or:
	return g_logic (ep, flags, op_or);
    case en_xor:
	return g_logic (ep, flags, op_xor);
    case en_mul:
	return g_mul (ep, flags);
    case en_div:
	return g_div (ep, flags, FALSE);
    case en_mod:
	return g_div (ep, flags, TRUE);
    case en_lsh:
	return g_shift (ep, flags, (is_unsigned_type (tp) ? op_lsh : op_ash));
    case en_asadd:
	return g_asadd (ep, flags, op_addi);
    case en_assub:
	return g_asadd (ep, flags, op_subi);
    case en_asand:
	if (ep->v.p[1]->nodetype == en_compl) {
	    EXPR   *ep1 = (EXPR *) ep;

	    ep1->v.p[1] = ep1->v.p[1]->v.p[0];
	    return g_aslogic (ep1, flags, op_andn);
	} else {
	    return g_aslogic (ep, flags, op_and);
	}
    case en_asor:
	return g_aslogic (ep, flags, op_or);
    case en_asxor:
	return g_aslogic (ep, flags, op_xor);
    case en_aslsh:
	return g_asshift (ep, flags,
			  (is_unsigned_type (tp) ? op_lsh : op_ash));
    case en_asmul:
	return g_asmul (ep, flags);
    case en_asdiv:
	return g_asdiv (ep, flags, FALSE);
    case en_asmod:
	return g_asdiv (ep, flags, TRUE);
    case en_assign:
	return g_assign (ep, flags);
    case en_ainc:
	return g_aincdec (ep, flags, op_addi);
    case en_adec:
	return g_aincdec (ep, flags, op_subi);
    case en_eq:
	VOIDCAST g_compare (ep);
	op = op_ldieq;
	goto cont1;
    case en_ne:
	VOIDCAST g_compare (ep);
	op = op_ldine;
	goto cont1;
    case en_lt:
	op = g_compare (ep) ? op_ldilo : op_ldilt;
	goto cont1;
    case en_le:
	op = g_compare (ep) ? op_ldils : op_ldile;
	goto cont1;
    case en_gt:
	op = g_compare (ep) ? op_ldihi : op_ldigt;
	goto cont1;
    case en_ge:
	op = g_compare (ep) ? op_ldihs : op_ldige;
	goto cont1;
    case en_test:
	g_test (ep->v.p[0]);
	op = op_ldine;
	goto cont1;
    case en_not:
	g_test (ep->v.p[0]);
	op = op_ldieq;
      cont1:
	ap1 = temporary_register (best_flags (flags, F_XREG));
	g_code (op_ldiu, OP_INT, mk_immed (0L), ap1);
	g_code (op, OP_INT, mk_immed (1L), ap1);
	return mk_legal (ap1, flags, size);
    case en_land:
    case en_lor:
	lab0 = nextlabel++;
	lab1 = nextlabel++;
	g_falsejp (ep, lab0);
	ap1 = temporary_register (best_flags (flags, F_XREG));
	g_code (op_ldi, OP_INT, mk_immed (1L), ap1);
	g_branch (lab1);
	g_label (lab0);
	g_code (op_ldi, OP_INT, mk_immed (0L), ap1);
	g_label (lab1);
	return mk_legal (ap1, flags, size);
    case en_cond:
	return g_hook (ep, flags);
    case en_comma:
	freeop (g_expr (ep->v.p[0], (FLAGS) (F_ALL | F_NOVALUE)));
	return g_expr (ep->v.p[1], flags);
    case en_fcall:
	return g_fcall (ep, flags);
    case en_cast:
	/*
	 * On the TMS320C30, do only casts when changing from
	 * floatingpoint to integertypes or vica versa
	 */
	switch (ep->etp->type) {
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_float:
	    case bt_double:
	    case bt_longdouble:
		return g_expr (ep->v.p[0], flags);
	    default:
		/*
		 * The cast really results in some work
		 */
		return g_cast (g_expr (ep->v.p[0], (FLAGS) (F_IALL | F_USES)),
			       ep->v.p[0]->etp, ep->etp, flags);
	    }
	    break;
	default:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_float:
	    case bt_double:
	    case bt_longdouble:
		/*
		 * The cast really results in some work
		 */
		return g_cast (g_expr (ep->v.p[0], (FLAGS) (F_FALL | F_USES)),
			       ep->v.p[0]->etp, ep->etp, flags);
	    default:
		return g_expr (ep->v.p[0], flags);
	    }
	    break;
	}
	CANNOT_REACH_HERE ();

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

    case en_fcon:
	ap1 = mk_expr (am_immed, copynode (ep));
	return mk_legal (ap1, flags, OP_FLOAT);
    default:
	FATAL ((__FILE__, "g_expr", "uncoded node %d", ep->nodetype));
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
 * generate code to do a comparison of the two operands of node. returns 1 if
 * it was an unsigned comparison
 */
static BOOL g_compare P1 (const EXPR *, ep)
{
    ADDRESS *ap1, *ap2;
    TYP    *tp = ep->v.p[0]->etp;

    switch (tp->type) {
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
	ap2 = g_expr (ep->v.p[1], F_IALL);
	ap1 = g_expr (ep->v.p[0], F_XREG);
	validate (ap2);
	/* sinc cmpi only reads 2 operands we use g_code3 and   */
	/* don't use dst ap, even if cmpi is in fact a 2-operand instr. */
	/* makes the job for the peephole-optimizer easier */
	g_code3 (op_cmpi, OP_INT, ap2, ap1, NIL_ADDRESS);
	freeop (ap1);
	freeop (ap2);
	return is_unsigned_type (tp);

    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap2 = g_expr (ep->v.p[1], F_FALL);
	ap1 = g_expr (ep->v.p[0], F_FREG);
	validate (ap2);
	/* sinc cmpf only reads 2 operands we use g_code3 and   */
	/* don't use dst ap, even if tszb is in fact a 2-operand instr. */
	/* makes the job for the peephole-optimizer easier */
	g_code3 (op_cmpf, OP_FLOAT, ap2, ap1, NIL_ADDRESS);
	freeop (ap1);
	freeop (ap2);
	return FALSE;
    default:
	break;
    }
    FATAL ((__FILE__, "g_compare", "illegal type %d", ep->v.p[0]->etp->type));
    return FALSE;
}


/*
 * Test the expression and set the condition codes accordingly
 */
static void g_test P1 (const EXPR *, ep)
{
    ADDRESS *ap;

    switch (ep->etp->type) {
#ifdef FLOAT_SUPPORT
    case bt_float:
    case bt_double:
    case bt_longdouble:
	ap = g_expr (ep, F_FREG);
	/* sinc cmpf only reads 2 operands we use g_code3 and   */
	/* don't use dst ap, even if tszb is in fact a 2-operand instr. */
	/* makes the job for the peephole-optimizer easier */

	g_code3 (op_cmpf, OP_FLOAT, mk_immedfloat (0.0), ap, NIL_ADDRESS);
	freeop (ap);
	/* note that the condition codes are tested,
	   not the return value */
	break;
#endif /* FLOAT_SUPPORT */
    default:
	ap = g_expr (ep, (FLAGS) (F_IALL & ~F_IMMED));
	/* tstb allows only [reg | ind | indx2] <-> [reg | ind | indx2] */
	if ((ap->mode == am_direct) || (ap->mode == am_const_direct)
	    || (ap->mode == am_indx)) {
	    ap = mk_legal (ap, F_XREG, OP_INT);
	}
	if ((ap->mode == am_ind) || (ap->mode == am_const_ind)
	    || (ap->mode == am_indx2) || (ap->mode == am_indxs)) {
	    g_code3 (op_tstb3, OP_INT, ap, ap, NIL_ADDRESS);
	} else {
	    /* sinc testb only reads 2 operands we use g_code3 and   */
	    /* don't use dst ap, even if tszb is in fact a 2-operand instr. */
	    /* makes the job for the peephole-optimizer easier */

	    g_code3 (op_tstb, OP_INT, ap, ap, NIL_ADDRESS);
	}
	freeop (ap);
    }
}

/*
 * generate a jump to label if the node passed evaluates to a true condition.
 */
static void g_truejp P2 (const EXPR *, ep, LABEL, label)
{
    LABEL   lab0;
    OPCODE  op;

    if (ep == NIL_EXPR)
	FATAL ((__FILE__, "g_truejp", "ep == 0"));
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
	op = g_compare (ep) ? op_blo : op_blt;
	g_cbranch (op, label);
	break;
    case en_le:
	op = g_compare (ep) ? op_bls : op_ble;
	g_cbranch (op, label);
	break;
    case en_gt:
	op = g_compare (ep) ? op_bhi : op_bgt;
	g_cbranch (op, label);
	break;
    case en_ge:
	op = g_compare (ep) ? op_bhs : op_bge;
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
	/*g_test(ep->v.p[0]);
	 *g_cbranch(op_beq, label);
	 */
	g_falsejp (ep->v.p[0], label);
	break;
    case en_test:
	g_test (ep->v.p[0]);
	g_cbranch (op_bne, label);
	break;
    case en_call:		/* library routine which sets the flags */
	freeop (g_expr (NIL_EXPR, (FLAGS) -1) /*ep, F_ALL) */ );
	g_cbranch (op_bne, label);
	break;
    default:
	/* only happens when called from g_falsejp in case en_not */
	g_test (ep);
	g_cbranch (op_bne, label);
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
	op = g_compare (ep) ? op_bhs : op_bge;
	g_cbranch (op, label);
	break;
    case en_le:
	op = g_compare (ep) ? op_bhi : op_bgt;
	g_cbranch (op, label);
	break;
    case en_gt:
	op = g_compare (ep) ? op_bls : op_ble;
	g_cbranch (op, label);
	break;
    case en_ge:
	op = g_compare (ep) ? op_blo : op_blt;
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
	/*g_test(ep->v.p[0]);
	 *g_cbranch(op_bne, label);
	 */
	g_truejp (ep->v.p[0], label);
	break;
    case en_test:
	g_test (ep->v.p[0]);
	g_cbranch (op_beq, label);
	break;
    case en_call:		/* library routine which sets the flags */
	freeop (g_expr (NIL_EXPR, (FLAGS) -1 /*ep, F_ALL */ ));
	g_cbranch (op_beq, label);
	break;
    default:
	/* only happens when called from g_falsejp in case en_not */
	g_test (ep);
	g_cbranch (op_beq, label);
	break;
    }
}

/*
 * generate a conditional load.
 */
PRIVATE ADDRESS *g_conditionalload P5 (const EXPR *, ep, ADDRESS *, truesrc,
				       ADDRESS *, falsesrc, ITYPE, type,
				       FLAGS, flags)
{
    OPCODE  op, op2;
    ADDRESS *ap1;

    if (ep == NIL_EXPR)
	FATAL ((__FILE__, "g_conditionalload", "ep == 0"));
    switch (ep->nodetype) {
    case en_icon:
	op = (type == OP_INT) ? op_ldi : op_ldf;
	ap1 =
	    (type ==
	     OP_INT) ? temporary_register (best_flags (flags,
						       F_XREG)) :
	    data_register (F_FREG);
	freeop (truesrc);
	freeop (falsesrc);
	if (ep->v.i) {
	    g_code (op, type, truesrc, ap1);
	} else {
	    g_code (op, type, falsesrc, ap1);
	}
	break;

    case en_eq:
	VOIDCAST g_compare (ep);
	op = (type == OP_INT) ? op_ldieq : op_ldfeq;
	op2 = (type == OP_INT) ? op_ldine : op_ldfne;
	goto common;

    case en_ne:
	VOIDCAST g_compare (ep);
	op = (type == OP_INT) ? op_ldine : op_ldfne;
	op2 = (type == OP_INT) ? op_ldieq : op_ldfeq;
	goto common;

    case en_lt:
	if (g_compare (ep)) {
	    op = (type == OP_INT) ? op_ldilo : op_ldflo;
	    op2 = (type == OP_INT) ? op_ldihs : op_ldfhs;
	} else {
	    op = (type == OP_INT) ? op_ldilt : op_ldflt;
	    op2 = (type == OP_INT) ? op_ldige : op_ldfge;
	}
	goto common;

    case en_le:
	if (g_compare (ep)) {
	    op = (type == OP_INT) ? op_ldils : op_ldfls;
	    op2 = (type == OP_INT) ? op_ldihi : op_ldfhi;
	} else {
	    op = (type == OP_INT) ? op_ldile : op_ldfle;
	    op2 = (type == OP_INT) ? op_ldigt : op_ldfgt;
	}
	goto common;

    case en_gt:
	if (g_compare (ep)) {
	    op = (type == OP_INT) ? op_ldihi : op_ldfhi;
	    op2 = (type == OP_INT) ? op_ldils : op_ldfls;
	} else {
	    op = (type == OP_INT) ? op_ldigt : op_ldfgt;
	    op2 = (type == OP_INT) ? op_ldile : op_ldfle;
	}
	goto common;

    case en_ge:
	if (g_compare (ep)) {
	    op = (type == OP_INT) ? op_ldihs : op_ldfhs;
	    op2 = (type == OP_INT) ? op_ldilo : op_ldflo;
	} else {
	    op = (type == OP_INT) ? op_ldige : op_ldfge;
	    op2 = (type == OP_INT) ? op_ldilt : op_ldflt;
	}
	goto common;

    case en_test:
	g_test (ep->v.p[0]);
	op = (type == OP_INT) ? op_ldieq : op_ldfeq;
	op2 = (type == OP_INT) ? op_ldine : op_ldfne;

      common:validate (truesrc);
	validate (falsesrc);
	freeop (truesrc);
	freeop (falsesrc);
	ap1 =
	    (type ==
	     OP_INT) ? temporary_register (best_flags (flags,
						       F_XREG)) :
	    data_register (F_FREG);
	if (falsesrc->mode == am_immed) {
	    g_code ((type == OP_INT) ? op_ldiu : op_ldfu, type, truesrc, ap1);
	    g_code (op2, type, falsesrc, ap1);
	} else {
	    g_code ((type == OP_INT) ? op_ldiu : op_ldfu, type, falsesrc,
		    ap1);
	    g_code (op, type, truesrc, ap1);
	}
	break;

    case en_land:
    case en_lor:
	FATAL ((__FILE__, "g_conditionalload", "en_land/en_lor"));
	break;
    case en_not:
	ap1 = g_conditionalload (ep->v.p[1], falsesrc, truesrc, type, flags);
	break;
    default:
	FATAL ((__FILE__, "g_conditionalload", "ilegal nodetype"));
	CANNOT_REACH_HERE ();
	/*g_test (ep);
	   validate (truesrc);
	   validate (falsesrc);
	   freeop (truesrc);
	   freeop (falsesrc);
	   ap1 = (type == OP_INT) ? temporary_register (F_XREG) : data_register (F_FREG);
	   op = (type == OP_INT) ? op_ldieq : op_ldfeq;
	   op2 = (type == OP_INT) ? op_ldine : op_ldfne;
	   if (falsesrc->mode == am_immed) {
	   g_code ((type == OP_INT) ? op_ldiu : op_ldfu, type, truesrc, ap1);
	   g_code (op2, type, falsesrc, ap1);
	   } else {
	   g_code ((type == OP_INT) ? op_ldiu : op_ldfu, type, falsesrc, ap1);
	   g_code (op, type, truesrc, ap1);
	   }
	   break; */
    }
    return (ap1);
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

    initstack ();
    ap = g_expr (ep, (FLAGS) (F_XREG | F_VOL));
    /*
     * move the interval
     */
    if (min_caselabel != (UVAL) 0) {
	g_immed (op_subi, (IVAL) min_caselabel, ap);
    }
    /* sinc cmpi only reads 2 operands we use g_code3 and   */
    /* don't use dst ap, even if tszb is in fact a 2-operand instr. */
    /* makes the job for the peephole-optimizer easier */
    g_code3 (op_cmpi, OP_INT, mk_immed (max_caselabel - min_caselabel), ap,
	     NIL_ADDRESS);
    g_cbranch (op_bhi, sw->deflab);
    ap1 = address_register ();
    g_code (op_ldi, OP_INT, mk_ilabel (mk_lcon (sw->tablab)), ap1);
    /* jump table contains 4 byte address of case branches */
    g_code (op_addi, OP_INT, ap, ap1);
    freeop (ap1);
    freeop (ap);
    ap1 = copy_addr (ap1, am_const_ind);
    ap2 = temporary_register (F_XREG);
    g_code (op_ldi, OP_INT, ap1, ap2);

    /* to have a connection between jumptable and switchjump */
    /* sinc ap2 is am_areg, this 'extra-offset' will be ignored */
    /* by the rest of the compiler */
    ap2->u.offset = mk_lcon (sw->tablab);

    g_code (op_bu, OP_INT, ap2, NIL_ADDRESS);
    freeop (ap2);
    checkstack ();
}

/*
 * Generate the body of a switch statement by comparing each case value
 * in turn.   The comparision is infact done by using subtraction as this
 * actually generates more efficient code (and would work best if the
 * labels were sorted!)
 */
#if 0
PRIVATE void g_switch_compare P2 (const EXPR *, ep, STMT *, stmt)
{
    IVAL    min_value;
    ADDRESS *ap;

    initstack ();
    ap = g_expr (ep, (FLAGS) (F_XREG | F_VOL));
    for (min_value = 0; stmt != NIL_STMT; stmt = stmt->s1) {
	if (stmt->stype != st_default) {
	    g_immed (op_subi, (long) (stmt->v2.i - min_value), ap);
	    min_value = stmt->v2.i;
	    stmt->v2.l = nextlabel++;
	    g_cbranch (op_beq, stmt->v2.l);
	}
    }
    freeop (ap);
    checkstack ();
}

#else
/* 
 * New variant, finds out if there is a conected valuerange to
 * the same label and uses a cmp-instruction instead of a chain of
 * subi 1, rx
 */
PRIVATE void g_switch_compare P2 (const EXPR *, ep, STMT *, stmt)
{
    IVAL    min, max;
    IVAL    min_value = 0L;
    LABEL   label;
    ADDRESS *ap;

    initstack ();
    ap = g_expr (ep, (FLAGS) (F_XREG | F_VOL));
    while (stmt != NIL_STMT) {
	if (stmt->stype != st_default) {
	    for (min = stmt->v2.i; stmt != NIL_STMT; stmt = stmt->s1) {
		max = stmt->v2.i;
		stmt->v2.l = label = nextlabel++;
		if ((stmt->s1 == NIL_STMT) ||
		    (stmt->s1->stype == st_default) ||
		    (stmt->s1 != stmt->v1.s) || (stmt->s1->v2.i != max + 1l)) {
		    stmt = stmt->s1;
		    break;
		}
	    }
	    if ((min - min_value) < 0L) {
		g_immed (op_addi, min_value - min, ap);
	    } else {
		g_immed (op_subi, min - min_value, ap);
	    }
	    min_value = min;
	    switch (max - min) {
	    case 1:
		g_cbranch (op_beq, label);
		g_immed (op_subi, 1L, ap);
		min_value++;
		/*FALLTHRU */
	    case 0:
		g_cbranch (op_beq, label);
		break;
	    default:
		g_immed (op_cmpi, max - min_value, ap);
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
#endif
/*
 *   Generate the code for setting up any local variables and
 *   the saving of any registers used.    This code is actually
 *   generated at the end of the function when the amount of
 *   stackspace actually required is known .... the peephole
 *   optimiser will move it back to the start of the function.
 */
PRIVATE void g_entry P1 (SIZE, frame_size)
{
#ifdef STACK_CHECK
    if ((stackcheck_option) && (interrupt_option == OPT_NO)) {
	SYM    *sp = internal_symbol (SUP_STACKCHECK, NIL_TYP);

	/*
	 * trashes a temporary dataregister, but this is no
	 * problem, since stackcheck is not allowed in
	 * interruptroutines and otherwise temporarys must not
	 * be saved
	 */

	ADDRESS *ap1 = data_register (F_DREG);

	g_immed (op_ldi, frame_size + max_stack_adjust, ap1);
	g_code (op_push, OP_INT, ap1, NIL_ADDRESS);
	g_code (op_call, OP_INT, mk_strlab (sp->name), NIL_ADDRESS);
    }
#endif /* STACK_CHECK */
    if (frame_size < 32768L) {
	g_code (op_push, OP_INT, mk_reg (frameptr), NIL_ADDRESS);
	g_code (op_ldi, OP_INT, mk_reg (STACKPTR), mk_reg (frameptr));
	if (frame_size != 0L) {
	    /* 
	     * addi is also allowed in interruptroutines since 
	     * addi n,sp does not change statusregister        
	     * (and SR is saved after the addi-instr.)          
	     */
	    g_immed (op_addi, frame_size, mk_reg (STACKPTR));
	}
    } else {
	FATAL ((__FILE__, "g_entry", "framesize > 32768"));
    }

/* Save used registers onto stack */
    if (restore_mask != (REGMASK) 0) {
	push_registers (restore_mask, floatrestore_mask);
    }
    max_stack_adjust = 0L;

}

/*
 *   Generate the code for a "return" statement.  This ensures
 *   that any returned result is loaded into the appropriate
 *   register.
 */
PRIVATE void g_return P2 (const EXPR *, stmtexp, TYP *, tp)
{
    EXPR   *ep, *ep1;
    ADDRESS *ap;

    return_register_mask = MEMBER (RESULT);
    initstack ();
    switch (tp->type) {
    case bt_struct:
    case bt_union:
	/* assign structure */
	ep = mk_autocon ((SIZE) -2L);
	ep = mk_ref (ep, tp_pointer);
	ep1 = mk_ref (ep, tp);
	ep1 = mk_node (en_assign, ep1, copynode (stmtexp), tp);
	VOIDCAST g_expr (ep1, (FLAGS) (F_ALL | F_NOVALUE));

	ap = g_expr (ep, F_IALL);
	g_code (op_ldi, OP_INT, ap, mk_reg (RESULT));
	freeop (ap);
	break;

    case bt_longdouble:
    case bt_double:
    case bt_float:
	ap = g_expr (stmtexp, F_FALL);
	g_code (op_ldf, OP_FLOAT, ap, mk_freg (RESULT));
	freeop (ap);
	break;

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
	ap = g_expr (stmtexp, F_IALL);
	g_code (op_ldi, OP_INT, ap, mk_reg (RESULT));
	freeop (ap);
	break;
    default:
	FATAL ((__FILE__, "g_return", "illegal type %d", tp->type));
    }
}

/*
 *   Generate the code required at the end of a function to
 *   restore any used registers and the return instruction itself.
 */
PRIVATE void g_epilogue P0 (void)
{
    ADDRESS *ap;
    SIZE    stackoffset;

    /*
     * Check if more registers are to be saved than used by the function
     * due to options set by user
     */

    if ((interrupt_option != OPT_NO)
	|| ((forced_save_mask | forced_floatsave_mask) != 0)) {
	REG     r;

	/* Mask out unwanted registers */
	/* which should not be pushed by force (SP, ST and IF) */
	forced_save_mask &=
	    ~(MEMBER (REG_ST) | MEMBER (REG_SP) | MEMBER (REG_IF) |
	      MEMBER (REG_IE) | MEMBER (REG_IOF));
	/* If we are forced be an option to save more registers */
	restore_mask |= forced_save_mask;
	floatrestore_mask |= forced_floatsave_mask;

	if (interrupt_option != OPT_NO) {
	    if (is_leaf_function) {

		/* Save only those temporaryregisters additionally wich are really used */
		interrupt_restore_mask &= ALL_TEMPORARYS |
		    MEMBER (REG_RC) | MEMBER (REG_RS) | MEMBER (REG_RE);

		interrupt_restore_mask |= MEMBER (REG_ST);
		restore_mask |= interrupt_restore_mask;

		interrupt_floatrestore_mask &=
		    (MEMBER (REG_R0) | MEMBER (REG_R1) | MEMBER (REG_R2));

		floatrestore_mask |= interrupt_floatrestore_mask;

	    } else {
		/* 
		 * Save at least all temporarys since this is not a leaf function
		 * and we have no way to tell which rgisters are used by the called functions
		 */
		restore_mask |= ALL_TEMPORARYS |
		    MEMBER (REG_ST) |
		    MEMBER (REG_RC) | MEMBER (REG_RS) | MEMBER (REG_RE);

		floatrestore_mask |=
		    MEMBER (REG_R0) | MEMBER (REG_R1) | MEMBER (REG_R2);
	    }
	}
	/* Mask out registers we really do not want to save */
	restore_mask &= ~MEMBER (frameptr);
	restore_mask &= ~return_register_mask;
	floatrestore_mask &= ~return_register_mask;

	/* count the saved registers (to calculate space on stack) */
	regs_used = 0;
	address_reg_used = 0;

	for (r = REG_R0; r <= MAX_REG; r++) {
	    if ((MEMBER (r) & (restore_mask)) != 0) {
		regs_used++;
		if (is_address_register (r)) {
		    address_reg_used++;
		}
	    }
	}
	for (r = REG_R0; r <= REG_R7; r++) {
	    if ((MEMBER (r) & (floatrestore_mask)) != 0) {
		regs_used++;
	    }
	}
    }
    /* Clean up for the next function (reinit) */
    interrupt_restore_mask = 0;
    interrupt_floatrestore_mask = 0;
    return_register_mask = 0;

    if (regs_used > 0) {
	if (is_leaf_function) {
	    pop_registers (restore_mask, floatrestore_mask);
	} else {
	    if (lc_auto > lc_auto_max) {
		lc_auto_max = lc_auto;
	    }
	    stackoffset = lc_auto_max + max_scratch + regs_used;
	    if ((stackoffset > 255) || (address_reg_used > 2)) {
		ap = mk_reg (REG_AR0);
		g_immed (op_ldi, stackoffset, ap);
		g_code3 (op_addi3, OP_INT, mk_reg (frameptr), ap,
			 mk_reg (STACKPTR));
		pop_registers (restore_mask, floatrestore_mask);
	    } else {
		pop_fast_registers (restore_mask, floatrestore_mask,
				    stackoffset);
	    }
	}
    }
    g_code (op_ldi, OP_INT, mk_reg (frameptr), mk_reg (STACKPTR));
    g_code (op_pop, OP_INT, NIL_ADDRESS, mk_reg (frameptr));
    if (interrupt_option == OPT_NO) {
	g_code (op_retsu, OP_INT, NIL_ADDRESS, NIL_ADDRESS);
    } else {
	g_code (op_retiu, OP_INT, NIL_ADDRESS, NIL_ADDRESS);
    }
}

/*
 * allocate will allocate registers for the expressions that have a high
 * enough desirability.
 */
PRIVATE void g_allocate P1 (CSE *, olist)
{
    CSE    *csp;
    REG     datareg = REG_R7;
    REG     addreg = (REG) ((int) frameptr - 1);
    REGMASK mask = (REGMASK) 0;
    REGMASK floatmask = (REGMASK) 0;

    regs_used = 0;
    address_reg_used = 0;
    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	/*
	 * If reg_option is not true, the 'desire' value must be at least
	 * 5000, which I hope can only be achieved by the 'register' attribute
	 */
	if (desire (csp) < 3 || (!reg_option && desire (csp) < 5000)
	    || (is_function_type (csp->exp->etp))) {
	    csp->reg = NO_REG;
/*
 * code gets more inefficient when enabling following statements
 * was thought to supress loading the address of a static or global
 * into an addressregister and use the addressregister instead directaddressing
 * but use of addressregs allows 3-operand-instructions and leads thus
 * to much better code.     
 *       } else if (  (opt_peep_test > 5)&&(opt_peep_test < 15) 
 *                 &&(  (csp->exp->nodetype == en_labcon)
 *                    ||(csp->exp->nodetype == en_nacon))) { 
 *          csp->reg = NO_REG;
 */
	} else if (csp->duses > (csp->uses / (unsigned) 3)
		   && !is_temporary_register (addreg)
		   /*
		    * integer constants may have different types
		    */
		   && csp->exp->nodetype != en_icon
		   /*
		    * the types which are fine in address registers
		    * allow only 32-bit integral and signed 16-bit integral types
		    */
		   && (csp->exp->etp->type == bt_short ||
		       csp->exp->etp->type == bt_int16 ||
		       csp->exp->etp->type == bt_int32 ||
		       csp->exp->etp->type == bt_uint32 ||
		       csp->exp->etp->type == bt_long ||
		       csp->exp->etp->type == bt_ulong ||
		       csp->exp->etp->type == bt_pointer32)) {
	    csp->reg = addreg--;
	    address_reg_used++;
	} else if (!is_temporary_register (datareg)
		   && !is_short (csp->exp)
		   && (csp->exp->etp->type != bt_pointer32)) {

	    /*
	     * the types which are fine in data registers:
	     * allow all types except pointers
	     */
	    /* Floatnumber need some special handling */
	    if (csp->exp->etp->type == bt_float
		|| csp->exp->etp->type == bt_double
		|| csp->exp->etp->type == bt_longdouble) {
		/*  numbers with bit FLOAT_REG set indicate floatingregisters */
		floatmask |= MEMBER (datareg);
		csp->reg = FLOAT_REG | datareg--;
		/* regs used must be incremented twice for float    */
		/* sinc pushed floatregisters need 2 words on stack */
		regs_used++;
	    } else {
		csp->reg = datareg--;
	    }
	} else {
	    csp->reg = NO_REG;
	}
	if (csp->reg != NO_REG) {
	    regs_used++;
	    mask |= MEMBER ((~FLOAT_REG) & (csp->reg));
	}
    }
    if (!is_temporary_register (addreg)) {
	/* maybe we can allocate some of the address registers */
	for (csp = olist; csp != NIL_CSE && !is_temporary_register (addreg);
	     csp = csp->next) {
	    if (csp->reg != NO_REG ||	/* already allocated to a register */
		desire (csp) < 3 ||	/* not desirable in a register */
		(!reg_option && desire (csp) < 5000)
		|| (is_function_type (csp->exp->etp))) {
		continue;
	    }
/*
 * code gets more inefficient when enabling following statements
 * was thought to supress loading the address of a static or global
 * into an addressregister and use the addressregister instead directaddressing
 * but use of addressregs allows 3-operand-instructions and leads thus
 * to much better code.     
 *          if (  (opt_peep_test > 5)&&(opt_peep_test < 15) 
 *              &&(  (csp->exp->nodetype == en_labcon)
 *                 ||(csp->exp->nodetype == en_nacon))) { 
 *              continue;
 *          }   
 */
	    if ((csp->exp->nodetype != en_icon)
		&& (csp->exp->etp->type == bt_short ||
		    csp->exp->etp->type == bt_int16 ||
		    csp->exp->etp->type == bt_int32 ||
		    csp->exp->etp->type == bt_uint32 ||
		    csp->exp->etp->type == bt_long ||
		    csp->exp->etp->type == bt_ulong ||
		    csp->exp->etp->type == bt_pointer32)) {
		csp->reg = addreg--;
		address_reg_used++;
		regs_used++;
		mask |= MEMBER (csp->reg);
	    }
	}
    }
/*
 * is moved now to g_entry
 *   if (mask != 0) {
 *       push_registers(mask, floatmask);
 *   }
 */
    restore_mask = mask;
    floatrestore_mask = floatmask;
}

PRIVATE void g_preload P1 (CSE *, olist)
{
    CSE    *csp;
    EXPR   *ep;
    ADDRESS *ap, *ap2;

    for (csp = olist; csp != NIL_CSE; csp = csp->next) {
	if (csp->reg != NO_REG) {	/* see if preload needed */
	    ep = csp->exp;
	    if (!is_lvalue (ep) || (ep->v.p[0]->v.i < 0L)) {
		initstack ();
		if ((csp->reg & FLOAT_REG) != 0) {
		    ap = g_expr (ep, F_FALL);
		    ap2 = mk_freg ((REG) ((~FLOAT_REG) & (csp->reg)));
		    g_code (op_ldf, OP_FLOAT, ap, ap2);
		} else {
		    ap = g_expr (ep, F_IALL);
		    ap2 = mk_reg (csp->reg);
		    g_code (op_ldi, OP_INT, ap, ap2);
		}
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
    SIZE    default_alignment = AL_DEFAULT;

    if (lc_auto_max % default_alignment != 0L) {
	lc_auto_max += default_alignment - (lc_auto_max % default_alignment);
    }
}

PRIVATE BOOL g_is_bigendian P0 (void)
{
/* We set it to false to avoid 'bitfield optimisation' in optimice.c */
/* (it tries to move referencepointer bytewise to save shifts        */
/* -> TMS cant move pointers bytewise, so let it be                  */
    return FALSE;
}

/*
 *  Defines if the systemstack is growing from low to high addresses (TRUE)
 *  or if it is going down from high to low addresses (more usual) (FALSE)
 *
 */

PRIVATE BOOL g_is_ascending_stack P0 (void)
{
    return TRUE;
}


/*
 *      This routine does any code generator specific transformations
 *      on the expression tree.
 *
 *      For example it can replace operator nodes with calls to runtime
 *      routines.   This allows the global optimiser to perform optimisations
 *      on such calls which wouldn't be possible if the calls were
 *      generated in the code generator routines themselves.
 */

PRIVATE EXPR *g_transform P1 (EXPR *, ep)
{
    if (ep == NIL_EXPR)
	return ep;
    switch (ep->nodetype) {
#ifdef FLOAT_SUPPORT
	TYP    *tp;

    case en_fcon:
	tp = ep->etp;
	if (!is_short_float (ep->v.f, tp->type)) {
	    ep = mk_lcon (mk_flabel (&(ep->v.f), tp));
	    ep = mk_ref (ep, tp);
	}
	return ep;
#endif /* FLOAT_SUPPORT */
    case en_icon:
    case en_nacon:
    case en_labcon:
	break;
    case en_autocon:
#if 1
	/* Just for test purposes, should be deleted */
	VOIDCAST printf ("astonishing, an autocon here ?\n");
	/*FALLTHRU */
#endif
    case en_sym:
    case en_register:
    case en_size:
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
    case en_aslsh:
    case en_asmul:
    case en_asdiv:
    case en_asmod:
    case en_mul:
    case en_div:
    case en_mod:
    case en_lsh:
	ep->v.p[1] = g_transform (ep->v.p[1]);
	/*FALLTHRU */
    case en_uminus:
    case en_test:
    case en_not:
    case en_compl:
    case en_deref:
	ep->v.p[0] = g_transform (ep->v.p[0]);
	break;
    case en_fieldref:
	ep->v.bit.p = g_transform (ep->v.bit.p);
	break;
    case en_ref:

	/* 
	 * is a quick and dirty try to avoid compilcated 
	 * dereferencing of autocons due subexpressionoptimizing 
	 * 
	 * void dummy(int a, int b) 
	 * {
	 *    *((&a)+32) = b; * repeated multiple times *
	 * }
	 * then &a gets the common subexpression and is 
	 * allocated to a register, leading to something like
	 * reg = fp+offset;
	 *
	 * and in later use to
	 *
	 * *(reg+32) = b
	 *
	 * but it would be better to put it
	 *
	 * *(fp+offset+32) = b, 
	 *
	 * this is cheaper, faster and saves an register
	 * (works of course only if offset+register is in range
	 *  of indexed adressingmode)
	 */
#if 0
	if (ep->v.p[0]->nodetype == en_add) {
	    if (is_icon (ep->v.p[0]->v.p[1])
		&& (ep->v.p[0]->v.p[0]->nodetype == en_sym)
		&& ((storageof (ep->v.p[0]->v.p[0]->v.sp) == sc_auto)
		    || (storageof (ep->v.p[0]->v.p[0]->v.sp) == sc_parms))) {
		int     i =

		    ep->v.p[0]->v.p[1]->v.i +
		    ep->v.p[0]->v.p[0]->v.sp->value.i;

		if ((i < 255) && (i > -255)) {
		    SYM    *sp = mk_sym ("CompilerGeneratedC30", sc_parms,
					 typeof (ep->v.p[0]->v.p[0]->v.sp));

		    sp->value.i = i;
		    ep->v.p[0] =
			mk_node (en_sym, NIL_EXPR, NIL_EXPR, ep->v.p[0]->etp);
		    ep->v.p[0]->v.sp = sp;
		    break;
		}
	    }
	}
#endif
	ep->v.p[0] = g_transform (ep->v.p[0]);
	break;

    case en_cast:

	/* do only cast conversions between integers and floats
	 */
	switch (ep->etp->type) {
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_float:
	    case bt_double:
	    case bt_longdouble:
		return g_transform (ep->v.p[0]);
	    default:
		break;
	    }
	    break;
	default:
	    switch (ep->v.p[0]->etp->type) {
	    case bt_float:
	    case bt_double:
	    case bt_longdouble:
		break;
	    default:
		return g_transform (ep->v.p[0]);
	    }
	    break;
	}
	ep->v.p[0] = g_transform (ep->v.p[0]);
	break;

    case en_rsh:
	ep->v.p[0] = g_transform (ep->v.p[0]);
	ep->v.p[1] = g_transform (ep->v.p[1]);
	ep->v.p[1] =
	    mk_node (en_uminus, ep->v.p[1], NIL_EXPR, ep->v.p[1]->etp);
	ep->nodetype = en_lsh;
	return ep;
    case en_asrsh:
	ep->v.p[0] = g_transform (ep->v.p[0]);
	ep->v.p[1] = g_transform (ep->v.p[1]);
	ep->v.p[1] =
	    mk_node (en_uminus, ep->v.p[1], NIL_EXPR, ep->v.p[1]->etp);
	ep->nodetype = en_aslsh;
	return ep;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    return ep;
}

/*
 *      This routine is called after the global optimizer has done it's
 *      work re-organizing the expression tree.  This allows a code
 *      generator to make code generator specific changes to the expression
 *      tree which will result in better code generation.
 */

PRIVATE EXPR *g_order P1 (EXPR *, ep)
{
    switch (ep->nodetype) {
    case en_and:
	/*
	 * Take advantage of the andn instruction
	 * (And with the complement of the second operand
	 */
	switch (ep->v.p[0]->nodetype) {
	case en_icon:
	    if ((ep->v.p[0]->v.i < 0)
		&& (~ep->v.p[0]->v.i < 0xFFFF)) {
		ep->v.p[0]->v.i = ~ep->v.p[0]->v.i;
		ep->v.p[0] =
		    mk_node (en_compl, ep->v.p[0], NIL_EXPR, ep->v.p[0]->etp);
		swap_nodes (ep);
		return (ep);
	    }
	    break;
	case en_compl:
	    if (!is_icon (ep->v.p[1])) {
		swap_nodes (ep);
		return (ep);
	    }
	default:
	    break;
	}
	/* FALLTHRU */

    case en_asand:
	/*
	 * Take advantage of the andn instruction
	 * (And with the complement of the second operand)
	 */
	switch (ep->v.p[1]->nodetype) {
	case en_icon:
	    if ((ep->v.p[1]->v.i < 0)
		&& (~ep->v.p[1]->v.i < 0xFFFF)) {
		ep->v.p[1]->v.i = ~ep->v.p[1]->v.i;
		ep->v.p[1] =
		    mk_node (en_compl, ep->v.p[1], NIL_EXPR, ep->v.p[1]->etp);
	    }
	    break;
	default:
	    break;
	}
	break;

    case en_lt:
    case en_gt:
    case en_le:
    case en_ge:
    case en_eq:
    case en_ne:
	switch (ep->v.p[0]->nodetype) {
	case en_register:
	    break;
	case en_icon:
	    if (is_short (ep->v.p[0]))
		swap_nodes (ep);
	    break;
	default:
	    switch (ep->v.p[1]->nodetype) {
	    case en_register:
		swap_nodes (ep);
		break;
	    case en_icon:
		if (!is_short (ep->v.p[1]))
		    /* will be in an register if not short, so do swapp */
		    swap_nodes (ep);
		break;
	    default:
		break;
	    }
	    break;
	}
	/*FALLTHRU */
    default:
	break;
    }
    return ep;
}

PRIVATE void g_initialize P0 (void)
{
    tp_pointer->size =
	tp_string->size =
	tp_wstring->size =
	tp_func->size =
	tp_short->size =
	tp_ushort->size =
	tp_int->size =
	tp_uint->size =
	tp_long->size =
	tp_ulong->size =
	tp_float->size =
	tp_double->size =
	tp_longdouble->size =
	tp_size->size = tp_ptrdiff->size = tp_wchar->size = 1L;
    bits_in_sizeunit = 32;
    init_peep ();

}

/*
 *   This routine is called when the compiler is closing down.
 */
PRIVATE void g_terminate P0 (void)
{
#ifdef VERBOSE
    c30_peep_report ();
#endif
}

/*
 *   Returns the current register usage.
 */
PRIVATE REGUSAGE *g_regusage P0 (TYP *tp)
{
    UNUSEDARG (tp);
    return reg_usage;
}


#ifdef MULTIPLE_PROCESSORS
#define	MCC30_FUNCS	(void *)&mcc30_funcs
#else
#define	MCC30_FUNCS	(void *)NULL
#endif /* MULTIPLE_PROCESSORS */

static OPTENUM yesnoopts[] = {
    {(const CHAR *) "yes", OPT_YES},
    {(const CHAR *) "no", OPT_NO},
    {(const CHAR *) NULL, 0}
};

static OPTENUM branchopts[] = {
    {(const CHAR *) "none", OPT_NONE},
    {(const CHAR *) "low", OPT_LEVEL1},
    {(const CHAR *) "medium", OPT_LEVEL2},
    {(const CHAR *) "hard", OPT_LEVEL3},
    {(const CHAR *) NULL, 0}
};

static OPTSET peepset[] = {
    {(const CHAR *) "none", EMPTY_SET},
    {(const CHAR *) "flow", MEMBER (PEEP_FLOW)},
    {(const CHAR *) "pipeline", MEMBER (PEEP_PIPELINE)},
    {(const CHAR *) "3operand", MEMBER (PEEP_3OPERAND)},
    {(const CHAR *) "parallel", MEMBER (PEEP_PARALLEL)},

    {(const CHAR *) "more_parallel",
     MEMBER (PEEP_PARALLEL) | MEMBER (PEEP_PARALLEL_ALL)},
    {(const CHAR *) "remap", MEMBER (PEEP_REMAP)},
    {(const CHAR *) "minimal", MEMBER (PEEP_MINIMAL)},
    {(const CHAR *) "redundant", MEMBER (PEEP_REDUNDANT)},
    {(const CHAR *) "store", MEMBER (PEEP_STORE)},
    {(const CHAR *) "switch", MEMBER (PEEP_SWITCH)},
    {(const CHAR *) "hardremap", MEMBER (PEEP_HARD_REMAP)},
    {(const CHAR *) "forceswitchremap", MEMBER (PEEP_VERY_HARD_REMAP)},
    {(const CHAR *) "standard", PEEP_STANDARD},
    {(const CHAR *) "all", PEEP_ALL},
    {(const CHAR *) NULL, EMPTY_SET}
};

static OPTENUM stackoptions[] = {
    {(const CHAR *) "safest", OPT_SAFE},
    {(const CHAR *) "minimum", OPT_MINIMUM},
    {(const CHAR *) "average", OPT_AVERAGE},
    {(const CHAR *) "maximum", OPT_MAXIMUM},
    {(const CHAR *) NULL, 0}
};

static OPTSET registeroptions[] = {
    {(const CHAR *) "none", EMPTY_SET},

    {(const CHAR *) "r0", MEMBER (REG_R0)},
    {(const CHAR *) "r1", MEMBER (REG_R1)},
    {(const CHAR *) "r2", MEMBER (REG_R2)},
    {(const CHAR *) "r3", MEMBER (REG_R3)},
    {(const CHAR *) "r4", MEMBER (REG_R4)},
    {(const CHAR *) "r5", MEMBER (REG_R5)},
    {(const CHAR *) "r6", MEMBER (REG_R6)},
    {(const CHAR *) "r7", MEMBER (REG_R7)},
    {(const CHAR *) "rn", MASK_REG_DATA},

    {(const CHAR *) "ar0", MEMBER (REG_AR0)},
    {(const CHAR *) "ar1", MEMBER (REG_AR1)},
    {(const CHAR *) "ar2", MEMBER (REG_AR2)},
    {(const CHAR *) "ar3", MEMBER (REG_AR3)},
    {(const CHAR *) "ar4", MEMBER (REG_AR4)},
    {(const CHAR *) "ar5", MEMBER (REG_AR5)},
    {(const CHAR *) "ar6", MEMBER (REG_AR6)},
    {(const CHAR *) "ar7", MEMBER (REG_AR7)},
    {(const CHAR *) "arn", MASK_REG_ADDRESS},

    {(const CHAR *) "dp", MEMBER (REG_DP)},
    {(const CHAR *) "ir0", MEMBER (REG_IR0)},
    {(const CHAR *) "ir1", MEMBER (REG_IR1)},
    {(const CHAR *) "bk", MEMBER (REG_BK)},
    {(const CHAR *) "sp", MEMBER (REG_SP)},
    {(const CHAR *) "st", MEMBER (REG_ST)},
    {(const CHAR *) "ie", MEMBER (REG_IE)},
    {(const CHAR *) "if", MEMBER (REG_IF)},
    {(const CHAR *) "iof", MEMBER (REG_IOF)},
    {(const CHAR *) "rs", MEMBER (REG_RS)},
    {(const CHAR *) "re", MEMBER (REG_RE)},
    {(const CHAR *) "rc", MEMBER (REG_RC)},
    {(const CHAR *) "all", MASK_REG_ALL},
    {(const CHAR *) NULL, EMPTY_SET}
};

static OPTION opts[] = {
    {
     (const CHAR *) "optbranch=", enumeration_option,
     {&opt_branches},
     {&branchopts[0]}
     },
    {
     (const CHAR *) "delayed=", numeric_option,
     {&opt_delayed_branches},
     {NULL}
     },
    {
     (const CHAR *) "mul32=", enumeration_option,
     {&opt_true_long},
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
     (const CHAR *) "pseq=", string_option,
     {(void *) &opt_peep_sequence},
     {NULL}
     },
    {
     (const CHAR *) "ramconst=", enumeration_option,
     {&opt_const_in_ram},
     {&yesnoopts[0]}
     },
    {
     (const CHAR *) "reg=", enumeration_option,
     {&reg_option},
     {&yesnoopts[0]}
     },
    {
     (const CHAR *) "shortfloat=", numeric_option,
     {&opt_shortfloat},
     {NULL}
     },
    {
     (const CHAR *) "interrupt=", enumeration_option,
     {&interrupt_option},
     {&yesnoopts[0]}
     },
    {
     (const CHAR *) "trap=", enumeration_option,
     {&opt_traps},
     {&yesnoopts[0]}
     },
    {
     (const CHAR *) "forcedsave=", set_option,
     {&forced_save_mask},
     {&registeroptions[0]}
     },
#ifdef DEBUG
    {
     (const CHAR *) "peeptest=", numeric_option,
     {&opt_peep_test},
     {NULL}
     },
#endif
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
#ifdef TARGET_ROSSIN
    {
     (const CHAR *) "rosc30", chip_option,
     {&rosc30_funcs},
     {MCC30_FUNCS}
     },
#endif /* TARGET_ROSSIN */
#else
#ifdef TARGET_ROSSIN
    {
     (const CHAR *) "rosc30", chip_option,
     {NULL},
     {MCC30_FUNCS}
     },
#endif /* TARGET_ROSSIN */
#endif /* MULTIPLE_ASSEMBLERS */
    {
     (const CHAR *) NULL, NULL,
     {NULL},
     {NULL}
     }
};

OPTIONS optsc30 = {
    (const CHAR *) "TI TMS320C30 ",
    opts
};

#ifdef MULTIPLE_PROCESSORS
struct genfuncs mcc30_funcs = {
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
    &alignments_c30[0]
};

#undef GEN_MODULE
#endif /* MULTIPLE_PROCESSORS */
#endif /* TMS320C30 */
