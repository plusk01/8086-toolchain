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

/******************************************************************************
 *
 * Register allocation (for the expression evaluation)
 * This modules handles the management of scratch registers.
 * It keeps track of the allocated registers and of the stack
 * Although large parts are identical to the 68000 version,
 * the diffs are so big that I maintain two different files
 *
 *****************************************************************************/

#include "config.h"

#ifdef INTEL

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genx86.h"

/********************************************************* Macro Definitions */

#define	MAX_REG_STACK	((DEEP)40)

/*********************************************** Static Function Definitions */

static void g_push P_ ((REG, DEEP));
static void g_pop P_ ((REG, DEEP));

/********************************************************** Static Variables */

static DEEP reg_in_use[NUM_REGS];
static REG next_dreg;		/* next temporary register */
static REG next_areg;		/* next temporary register */
static REG next_freg;		/* next temporary register */

/*
 *   When a register is already in use then it is necessary to
 *   save the current contents on the stack.  The reg_stack
 *   array is used to keep track of those registers which have
 *   been pushed onto the stack.
 */
static struct
{
    REG     reg;
    DEEP    depth;
} reg_stack[(int) MAX_REG_STACK + 1];
static DEEP stack_depth;

/*
 *   When registers are alloced it is necessary to track the
 *   order in which they were allocated and whether an item
 *   which should be in a register has been temporarily
 *   pushed onto the stack.
 */
static struct
{
    REG     reg;
    REG     next_reg;
    REGTYPE regtype;
    BOOL    pushed;
} reg_alloc[(int) MAX_REG_STACK + 1];
static DEEP alloc_depth;

/*
 *   Define the registers which can be used to pass parameters.
 */
static REG parameter_registers[] = {
    EAX, EBX, ECX, EDX, ESI, EDI
};
static REGLIST parameter_list = {
    (int) (sizeof (parameter_registers) / sizeof (REG)),
    &parameter_registers[0]
};

/*
 *   Define the registers which must be saved by a function if
 *   they are used.
 *   If the register is used to return a value then it needn't be
 *   saved.
 *   If the register is used to pass a parameter then it needn't be
 *   saved.
 */
static REG saved_registers[] = {
    EAX, EBX, ECX, EDX, ESI, EDI
};
static REGLIST saved_list = {
    (int) (sizeof (saved_registers) / sizeof (REG)),
    &saved_registers[0]
};

/*
 *   Define the registers which are used to return the results
 *   from a function call.
 */
static REG result_registers[] = {
    EAX, EDX
};
static REGLIST result_list = {
    (int) (sizeof (result_registers) / sizeof (REG)),
    &result_registers[0]
};

static REGUSAGE rusage = {
    &parameter_list,
    &saved_list,
    &result_list
};
REGUSAGE *reg_usage = &rusage;

REGTYPE *regtypes;

/*****************************************************************************/

/*
 * this routine generates code to push a register onto the stack
 */
static void g_push P2 (REG, reg, DEEP, depth)
{
    ADDRESS *ap;

#ifdef FLOAT_IEEE
    ADDRESS *ap2;

#endif /* FLOAT_IEEE */
    sync_stack ();
    ap = mk_reg (reg);
    switch (reg_alloc[depth].regtype) {
    case D_REG | T_REG:
    case A_REG | T_REG:
    case X_REG | T_REG:
	g_code (op_push, small_option ? IL2 : IL4, ap, NIL_ADDRESS);
	break;
#ifdef FLOAT_IEEE
    case F_REG | T_REG:
	ap2 = mk_reg (ESP);
	g_code (op_sub, small_option ? IL2 : IL4, mk_immed ((IVAL) 8L), ap2);
	ap2 = mk_reg (ESP);
	ap2->mode = am_ind;
	g_fcode (op_fstp, IL8, ap2, NIL_ADDRESS);
	break;
#endif /* FLOAT_IEEE */
    default:
	CANNOT_REACH_HERE ();
    }
    reg_stack[stack_depth].reg = reg;
    reg_stack[stack_depth].depth = depth;

    /* is already pushed */
    if (reg_alloc[depth].pushed) {
	FATAL ((__FILE__, "g_push", "reg %d already pushed", (int) reg));
    }
    reg_alloc[depth].pushed = TRUE;

    /* check on stack overflow */
    if (++stack_depth > MAX_REG_STACK) {
	FATAL ((__FILE__, "g_push", "register stack overflow"));
    }
}

/*
 * generate code to pop a register from the stack.
 */
static void g_pop P2 (REG, reg, DEEP, depth)
{
    ADDRESS *ap;

#ifdef FLOAT_IEEE
    ADDRESS *ap2;

#endif /* FLOAT_IEEE */

    /* check on stack underflow */
    if (stack_depth-- == EMPTY) {
	FATAL ((__FILE__, "g_pop", "register %d stack empty", (int) reg));
    }
    /* check if the desired register really is on stack */
    if (reg_stack[stack_depth].depth != depth) {
	FATAL (
	       (__FILE__, "g_pop", "register %d order %d, %d", (int) reg,
		(int) reg_stack[stack_depth].depth, (int) depth));
    }
    /* check if the register which is restored is really void */
    if (reg_in_use[reg] != UNUSED) {
	FATAL ((__FILE__, "g_pop", "register %d in use", (int) reg));
    }
    reg_in_use[reg] = depth;
    sync_stack ();
    ap = mk_reg (reg);
    switch (reg_alloc[depth].regtype) {
    case A_REG | T_REG:
    case D_REG | T_REG:
    case X_REG | T_REG:
	g_code (op_pop, small_option ? IL2 : IL4, ap, NIL_ADDRESS);
	break;
#ifdef FLOAT_IEEE
    case F_REG | T_REG:
	ap2 = mk_reg (ESP);
	ap2->mode = am_ind;
	g_fcode (op_fld, IL8, ap2, NIL_ADDRESS);
	ap2 = mk_reg (ESP);
	g_code (op_add, small_option ? IL2 : IL4, mk_immed ((IVAL) 8L), ap2);
	break;
#endif /* FLOAT_IEEE */
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    /* clear the push_flag */
    reg_alloc[depth].pushed = FALSE;
}

/*
 * this routine should be called before each expression is evaluated to make
 * sure the stack is balanced and all of the registers are marked free.
 * This is also a good place to free all 'pseudo' registers in the stack frame
 * by clearing act_scratch.
 */
void initstack P0 (void)
{
    REG     reg;

    for (reg = EAX; reg <= ST7; reg++)
	reg_in_use[reg] = UNUSED;
    next_dreg = EAX;
    next_areg = ESI;
    next_freg = ST0;
    stack_depth = EMPTY;
    alloc_depth = EMPTY;
    act_scratch = 0;
}

/*
 * this routine checks if all allocated registers were freed correctly
 */
void checkstack P0 (void)
{
    REG     reg;

    for (reg = EAX; reg <= ST7; reg++) {
	if (!is_temporary_register (reg)) {
	    continue;
	}
	if (reg_in_use[reg] != UNUSED) {
	    FATAL ((__FILE__, "checkstack", "register %d in use", (int) reg));
	}
    }
    if (next_dreg != EAX) {
	FATAL (
	       (__FILE__, "checkstack",
		"not all data registers deallocated (%d)", (int) next_dreg));
    }
    if (next_areg != ESI) {
	FATAL (
	       (__FILE__, "checkstack",
		"not all address registers deallocated (%d)",
		(int) next_areg));
    }
    if (next_freg != ST0) {
	FATAL (
	       (__FILE__, "checkstack",
		"not all float registers deallocated (%d)", (int) next_areg));
    }
    if (stack_depth != EMPTY) {
	FATAL (
	       (__FILE__, "checkstack", "register stack not empty (%d)",
		(int) stack_depth));
    }
    if (alloc_depth != EMPTY) {
	FATAL (
	       (__FILE__, "checkstack",
		"register allocation stack not empty (%d)",
		(int) alloc_depth));
    }
}

/*
 *   validate() will make sure that if a register within an address
 *   mode has been pushed onto the stack that it is popped back at
 *   this time.
 */
void validate P1 (const ADDRESS *, ap)
{
    REG     reg;

    switch (ap->mode) {
    case am_mreg:
	reg = ap->sreg;
	if (is_temporary_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, (DEEP) ((int) ap->deep + 1));
	}
	/*lint -fallthrough */
    case am_dreg:
    case am_freg:
	reg = ap->preg;
	if (is_temporary_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    case am_areg:
    case am_ind:
	reg = ap->preg;
	if (is_temporary_address_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    case am_indx2:
	reg = ap->sreg;
	if (is_data_or_address_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    case am_indx:
	reg = ap->preg;
	if (is_data_or_address_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    default:
	return;
    }
}


/*
 *   Return the next register of type 'regtype'
 */
static REG next_reg P2 (REG, reg, REGTYPE, regtype)
{
    for (;;) {
	if (reg > ST7) {
	    reg = EAX;
	}
	if ((regtypes[reg] & regtype) == regtype) {
	    return reg;
	}
	reg++;
    }
}

static REG allocate_register P3 (REG, reg, REG, nreg, REGTYPE, regtype)
{
    reg = next_reg (reg, regtype);
    /*
     *       if the register is in use, push it to the stack
     */
    if (reg_in_use[reg] != UNUSED) {
	g_push (reg, reg_in_use[reg]);
    }
    reg_in_use[reg] = alloc_depth;
    reg_alloc[alloc_depth].reg = reg;
    reg_alloc[alloc_depth].next_reg = nreg;
    reg_alloc[alloc_depth].regtype = regtype;
    reg_alloc[alloc_depth].pushed = FALSE;

    if (alloc_depth++ >= MAX_REG_STACK) {
	FATAL ((__FILE__, "allocate_register", "register stack overflow"));
    }
    return reg;
}

static void deallocate_register P1 (REG, reg)
{
    DEEP    depth;

    if (!is_temporary_register (reg)) {
	return;
    }
    depth = reg_in_use[reg];
    if (reg_alloc[depth].reg != reg) {
	FATAL (
	       (__FILE__, "deallocate_register", "register order (%d,%d)",
		(int) reg_alloc[depth].reg, (int) reg));
    }
    switch (reg_alloc[depth].regtype) {
    case D_REG | T_REG:
    case X_REG | T_REG:
    case C_REG | T_REG:
    case Z_REG | T_REG:
    case D_REG | N_REG | T_REG:
	next_dreg = reg_alloc[depth].next_reg;
	break;
    case A_REG | T_REG:
	next_areg = reg_alloc[depth].next_reg;
	break;
#ifdef FLOAT_IEEE
    case F_REG | T_REG:
	next_freg = reg_alloc[depth].next_reg;
	break;
#endif /* FLOAT_IEEE */
    default:
	CANNOT_REACH_HERE ();
    }

    reg_in_use[reg] = UNUSED;

    /* we should only free the most recently allocated register */
    if (alloc_depth-- == EMPTY) {
	FATAL (
	       (__FILE__, "deallocate_register", "register %d stack empty",
		(int) reg));
    }
    if (alloc_depth != depth) {
	FATAL (
	       (__FILE__, "deallocate_register",
		"register %d stack order (%d,%d)", (int) reg,
		(int) alloc_depth, (int) depth));
    }
    /* the just freed register should not be on stack */
    if (reg_alloc[depth].pushed) {
	FATAL (
	       (__FILE__, "deallocate_register", "register %d pushed",
		(int) reg));
    }
}

/*
 *   Allocates a temporary register which is appropriate to the
 *   regtype parameter.
 */
static ADDRESS *temp_register P1 (REGTYPE, regtype)
{
    REG     reg, reg2;
    ADDRESS *ap;

    switch (regtype) {
    case D_REG | T_REG:
	reg = allocate_register (next_dreg, next_dreg, regtype);
	ap = mk_reg (reg);
	next_dreg = (REG) ((int) reg + 1);
	ap->mode = am_dreg;
	break;
    case D_REG | N_REG | T_REG:
	if (next_dreg == ECX) {
	    next_dreg++;
	}
	reg = allocate_register (next_dreg, next_dreg, regtype);
	ap = mk_reg (reg);
	next_dreg = (REG) ((int) reg + 1);
	ap->mode = am_dreg;
	break;
    case A_REG | T_REG:
	reg = allocate_register (next_areg, next_areg, regtype);
	ap = mk_reg (reg);
	next_areg = (REG) ((int) reg + 1);
	ap->mode = am_areg;
	break;
    case M_REG | T_REG:
	reg = allocate_register (next_dreg, next_dreg, D_REG | T_REG);
	next_dreg = (REG) ((int) reg + 1);
	reg2 = allocate_register (next_dreg, next_dreg, D_REG | T_REG);
	ap = mk_mreg (reg, reg2);
	next_dreg = (REG) ((int) reg2 + 1);
	break;
    case X_REG | T_REG:
	reg = allocate_register (EAX, next_dreg, regtype);
	next_dreg = (REG) ((int) reg + 1);
	reg2 = allocate_register (next_dreg, next_dreg, regtype);
	ap = mk_mreg (reg, reg2);
	next_dreg = (REG) ((int) reg2 + 1);
	break;
    case F_REG | T_REG:
	reg = allocate_register (next_freg, next_freg, regtype);
	ap = mk_reg (reg);
	next_freg = (REG) ((int) reg + 1);
	ap->mode = am_freg;
	break;
    case C_REG | T_REG:
	reg = allocate_register (ECX, next_dreg, regtype);
	ap = mk_reg (reg);
	next_dreg = (REG) ((int) reg + 1);
	ap->mode = am_dreg;
	break;
    case Z_REG | T_REG:
	reg = allocate_register (EAX, next_dreg, regtype);
	ap = mk_reg (reg);
	next_dreg = (REG) ((int) reg + 1);
	ap->mode = am_dreg;
	break;
    default:
	CANNOT_REACH_HERE ();
    }
    ap->deep = reg_in_use[ap->preg];
    return ap;
}

/*
 *   Allocate a data register
 */
ADDRESS *data_register P0 (void)
{
    return temp_register (D_REG | T_REG);
}

ADDRESS *data_register_no_cx P0 (void)
{
    return temp_register (D_REG | N_REG | T_REG);
}

ADDRESS *ax_register P0 (void)
{
    return temp_register (Z_REG | T_REG);
}

ADDRESS *axdx_register P0 (void)
{
    return temp_register (X_REG | T_REG);
}

ADDRESS *cx_register P0 (void)
{
    return temp_register (C_REG | T_REG);
}

/*
 *   Allocate an index register
 */
ADDRESS *address_register P0 (void)
{
    return temp_register (A_REG | T_REG);
}

/*
 *   Allocate 2 registers
 */
ADDRESS *mdata_register P0 (void)
{
    return temp_register (M_REG | T_REG);
}

#ifdef FLOAT_IEEE
ADDRESS *float_register P0 (void)
{
    return temp_register (F_REG | T_REG);
}
#endif /* FLOAT_IEEE */

/*
 *   Returns TRUE is the specified register is not available at "no cost"
 *   (no push).
 */
BOOL is_register_used P1 (REG, reg)
{
    return (reg_in_use[reg] != UNUSED);
}

/*
 * tells if an address mode uses a scratch register
 */
BOOL uses_temp P1 (const ADDRESS *, ap)
{
    if (ap == NIL_ADDRESS) {
	FATAL ((__FILE__, "uses_temp", ""));
    }
    switch (ap->mode) {
    case am_dreg:
    case am_areg:
    case am_freg:
    case am_ind:
    case am_indx:
	return (is_temporary_register (ap->preg));
    case am_indx2:
	return (is_temporary_register (ap->sreg) &&
		is_temporary_register (ap->preg));
    default:
	return FALSE;
    }
}

/*
 * release any temporary registers used in an addressing mode.
 */
void freeop P1 (const ADDRESS *, ap)
{
    DEEP    depth;
    REG     reg;

    if (ap == NIL_ADDRESS) {
	/* This can happen freeing a NOVALUE result */
	return;
    }
    switch (ap->mode) {
    case am_mreg:
    case am_indx2:
	deallocate_register (ap->sreg);
	/*lint -fallthrough */
    case am_dreg:
    case am_areg:
    case am_freg:
    case am_ind:
    case am_indx:
	reg = ap->preg;
	break;
    default:
	return;
    }

    if (!is_temporary_register (reg)) {
	return;
    }
    depth = reg_in_use[reg];
    deallocate_register (reg);

    /* some consistency checks */
    if (depth != ap->deep) {
	FATAL (
	       (__FILE__, "freeop", "(%d, %d, %d)", (int) reg, (int) depth,
		(int) ap->deep));
    }
}

/*
 *   push any used temporary registers.
 *
 *   This is necessary across function calls.
 *   The reason for this hacking is actually that temp_inv should dump
 *   the registers in the correct order,
 *   the least recently allocate register first.
 *   the most recently allocated register last.
 */
void temp_inv P0 (void)
{
    DEEP    deep;

    for (deep = EMPTY; deep < alloc_depth; deep++) {
	if (!reg_alloc[deep].pushed) {
	    g_push (reg_alloc[deep].reg, deep);
	    /* mark the register void */
	    reg_in_use[reg_alloc[deep].reg] = UNUSED;
	}
    }
}

/*
 *   Converts a list of registers into a register mask
 */
REGMASK reglist_to_mask P1 (const REGLIST *, rp)
{
    REGMASK mask = (REGMASK) 0;
    int     num;

    for (num = 0; num < rp->number; num++) {
	mask |= REGBIT (rp->reg[num]);
    }
    return mask;
}
#endif /* INTEL */
