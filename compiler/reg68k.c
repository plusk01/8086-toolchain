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
 *
 *****************************************************************************/

#include "config.h"

#ifdef MC680X0

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "gen68k.h"

/*********************************************** Static Function Definitions */

static void g_push P_ ((REG, DEEP));
static void g_pop P_ ((REG, DEEP));
static REG next_reg P_ ((REG, REGTYPE));

/********************************************************** Static Variables */

/*
 *   This data structure is used to keep track of registers which
 *   have been pushed onto the stack.
 */
static struct
{
    REG     reg;
    DEEP    depth;
} reg_stack[(int) MAX_REG_STACK + 1];
static DEEP stack_depth;

/*
 *   This data structure is used to keep track of register which
 *   have been allocated.
 */
static struct
{
    REG     reg;
    BOOL    pushed;
} reg_alloc[(int) MAX_REG_STACK + 1];
static DEEP alloc_depth;

/*
 *   Define the registers which must be saved by a function if
 *   they are used.
 *   If the register is used to return a value then it needn't be
 *   saved.
 *   If the register is used to pass a parameter then it needn't be
 *   saved.
 */
static REG saved_registers[] = {
    D3, D4, D5, D6, D7,
    A2, A3, A4, A5, A6, A7
#ifdef FLOAT_IEEE
	, FP2, FP3, FP4, FP5, FP6, FP7
#endif				/* FLOAT_IEEE */
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
    D0, D1, D2
};
static REGLIST result_list = {
    (int) (sizeof (result_registers) / sizeof (REG)),
    &result_registers[0]
};

static REGUSAGE rusage = {
    &saved_list,
    &result_list
};
REGUSAGE *reg_usage = &rusage;

REGTYPE regtype[NUM_REGS] = {
    D_REG,			/* D0 */
    D_REG,			/* D1 */
    D_REG,			/* D2 */
    D_REG,			/* D3 */
    D_REG,			/* D4 */
    D_REG,			/* D5 */
    D_REG,			/* D6 */
    D_REG,			/* D7 */
    A_REG,			/* A0 */
    A_REG,			/* A1 */
    A_REG,			/* A2 */
    A_REG,			/* A3 */
    A_REG,			/* A4 */
    A_REG,			/* A5 */
    A_REG,			/* A6 */
    A_REG,			/* A7 */
#ifdef FLOAT_IEEE
    F_REG,			/* FP0 */
    F_REG,			/* FP1 */
    F_REG,			/* FP2 */
    F_REG,			/* FP3 */
    F_REG,			/* FP4 */
    F_REG,			/* FP5 */
    F_REG,			/* FP6 */
    F_REG /* FP7 */  
#else	/*  */
    0, 0, 0, 0, 0, 0, 0, 0
#endif				/* FLOAT_IEEE */
};

static DEEP reg_in_use[NUM_REGS];
static REGMASK associated_regs[NUM_REGS];
static ADDRESS apush = {
    am_adec, STACKPTR, (REG) 0, EMPTY,
    {NIL_EXPR}	    /*lint !e708*/  /* union initialization */
};
static ADDRESS apop = {
    am_ainc, STACKPTR, (REG) 0, EMPTY,
    {NIL_EXPR}	    /*lint !e708*/  /* union initialization */
};

static REG next_data;		/* next temporary data register */
static REG start_data;		/* initial temporary data register */
static REG next_addr;		/* next temporary address register */
static REG start_addr;		/* initial temporary address register */

#ifdef FLOAT_IEEE
static REG next_float;		/* next temporary floating point register */
static REG start_float;		/* initial temporary floating point register */

#endif /* FLOAT_IEEE */

/*****************************************************************************/

/*
 *   This routine generates code to push a register onto the stack.
 */
static void g_push P2 (REG, reg, DEEP, depth)
{
    ADDRESS *ap;

    sync_stack ();
    ap = mk_reg (reg);
    switch (reg) {
    case D0:
    case D1:
    case D2:
    case D3:
    case D4:
    case D5:
    case D6:
    case D7:
    case A0:
    case A1:
    case A2:
    case A3:
    case A4:
    case A5:
    case A6:
    case A7:
	g_move (IL4, ap, &apush);
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
	g_fcode (op_fmove, IL12, ap, &apush);
	break;
#endif /* FLOAT_IEEE */
    default:
	CANNOT_REACH_HERE ();
    }
    reg_stack[stack_depth].reg = reg;
    reg_stack[stack_depth].depth = depth;

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
 *   Generate code to pop a register from the stack.
 */
static void g_pop P2 (REG, reg, DEEP, depth)
{
    ADDRESS *ap;

    /* check on stack underflow */
    if (stack_depth-- == EMPTY) {
	FATAL ((__FILE__, "g_pop", "register stack empty"));
    }
    /* check if the desired register really is on stack */
    if (reg_stack[stack_depth].depth != depth) {
	FATAL (
	       (__FILE__, "g_pop", "register order (%d,%d)", (int) depth,
		(int) reg_stack[stack_depth].depth));
    }
    /* check if the register which is restored is really void */
    if (reg_in_use[reg] != UNUSED) {
	FATAL ((__FILE__, "g_pop", "register %d in use", (int) reg));
    }
    reg_in_use[reg] = depth;
    sync_stack ();
    ap = mk_reg (reg);
    switch (reg) {
    case D0:
    case D1:
    case D2:
    case D3:
    case D4:
    case D5:
    case D6:
    case D7:
    case A0:
    case A1:
    case A2:
    case A3:
    case A4:
    case A5:
    case A6:
    case A7:
	g_move (IL4, &apop, ap);
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
	g_fcode (op_fmove, IL12, &apop, ap);
	break;
#endif /* FLOAT_IEEE */
    default:
	CANNOT_REACH_HERE ();
    }

    /* clear the push_flag */
    reg_alloc[depth].pushed = FALSE;
}


/*
 *   This routine should be called before each expression is
 *   evaluated to make sure the stack is balanced and all of
 *   the registers are marked free.
 *
 *   This is also a good place to free all 'pseudo' registers
 *   in the stack frame by setting act_scratch to zero.
 */
void initstack P0 (void)
{
    REG     reg;

    start_data = next_data = next_reg (D7, D_REG);
    start_addr = next_addr = next_reg (A7, A_REG);
#ifdef FLOAT_IEEE
    start_float = next_float = next_reg (FP7, F_REG);
#endif /* FLOAT_IEEE */
    for (reg = D0; reg <= MAX_REG; reg++) {
	reg_in_use[reg] = UNUSED;
	associated_regs[reg] = (REGMASK) 0;
    }
    stack_depth = EMPTY;
    alloc_depth = EMPTY;
    act_scratch = 0L;
}


/*
 *   This routines checks if all allocated registers were freed.
 */
void checkstack P0 (void)
{
    REG     reg;

    if (next_data != start_data) {
	FATAL (
	       (__FILE__, "checkstack", "D%d not next temporary",
		(int) start_data - (int) D0));
    }
    if (next_addr != start_addr) {
	FATAL (
	       (__FILE__, "checkstack", "A%d not next temporary",
		(int) start_addr - (int) A0));
    }
#ifdef FLOAT_IEEE
    if (next_float != start_float) {
	FATAL (
	       (__FILE__, "checkstack", "FP%d not next temporary",
		(int) start_float - (int) FP0));
    }
#endif
    for (reg = D0; reg <= MAX_REG; reg++) {
	if ((regtype[reg] & T_REG) && (reg_in_use[reg] != UNUSED)) {
	    FATAL (
		   (__FILE__, "checkstack", "register %d still in use",
		    (int) reg));
	}
    }
    if (stack_depth != EMPTY) {
	FATAL ((__FILE__, "checkstack", "register stack not empty"));
    }
    if (alloc_depth != EMPTY) {
	FATAL (
	       (__FILE__, "checkstack",
		"allocated register stack not empty"));
    }
}


/*
 *   Validate will make sure that if a register within an address
 *   mode has been pushed onto the stack that it is popped back
 *   at this time.
 */
void validate P1 (const ADDRESS *, ap)
{
    REG     reg;

    assert (ap != NULL);

    switch (ap->mode) {
    case am_xreg:
	reg = ap->u.xreg;
	if (is_temporary_data_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, (DEEP) ((int) ap->deep + 2));
	}
	/*lint -fallthrough */
    case am_mreg:
	reg = ap->sreg;
	if (is_temporary_data_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, (DEEP) ((int) ap->deep + 1));
	}
	/*lint -fallthrough */
    case am_dreg:
	reg = ap->preg;
	if (is_temporary_data_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    case am_indx2:
    case am_indx4:
	reg = ap->sreg;
	if (is_temporary_data_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	goto common;	    /*lint !e801*/  /* use of goto deprecated */
    case am_indx3:
	reg = ap->sreg;
	if (is_temporary_address_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	goto common;	    /*lint !e801*/  /* use of goto deprecated */
    case am_areg:
    case am_ind:
    case am_indx:
    case am_ainc:
    case am_adec:
      common:
	reg = ap->preg;
	if (is_temporary_address_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
#ifdef FLOAT_IEEE
    case am_freg:
	reg = ap->preg;
	if (is_temporary_float_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
#endif /* FLOAT_IEEE */
    default:
	break;
    }
}

/*
 *   Return the next register of type 'kind'
 */
static REG next_reg P2 (REG, reg, REGTYPE, kind)
{
    int     count;

    for (count = 64; count; count--) {
	reg = (reg == FP7) ? D0 : (REG) ((int) reg + 1);
	if (((regtype[reg] & T_REG)) && (regtype[reg] & kind)) {
	    return reg;
	}
    }
    return NO_REG;
}

/*
 *   Return the previous register of type 'kind'
 */
static REG prev_reg P2 (REG, reg, REGTYPE, kind)
{
    int     count;

    for (count = 64; count; count--) {
	reg = (reg == D0) ? FP7 : (REG) ((int) reg - 1);
	if (((regtype[reg] & T_REG)) && (regtype[reg] & kind)) {
	    return reg;
	}
    }
    return NO_REG;
}

/*
 *   Allocate a temporary register.    Returns the next register
 *   that will be allocated.
 */
static REG allocate_register P2 (REG, reg, REGTYPE, kind)
{
    if (reg_in_use[reg] != UNUSED) {
	/*
	 *   The next available register is already in use.
	 *   It (and any associated registers) must be pushed.
	 */
	REG     r;
	REGMASK rmask = associated_regs[reg];

	for (r = 0; r < NUM_REGS; r++) {
	    if (rmask & REGBIT (r)) {
		g_push (r, reg_in_use[r]);
		reg_in_use[r] = UNUSED;
	    }
	}
    }
    reg_in_use[reg] = alloc_depth;
    associated_regs[reg] = REGBIT (reg);
    reg_alloc[alloc_depth].reg = reg;
    reg_alloc[alloc_depth].pushed = FALSE;

    if (alloc_depth++ == MAX_REG_STACK) {
	FATAL ((__FILE__, "allocate_register", "register stack overflow"));
    }
    return next_reg (reg, kind);
}

/*
 *   Allocate a temporary data register and return
 *   it's addressing mode.
 */
ADDRESS *data_register P0 (void)
{
    ADDRESS *ap = mk_reg (next_data);

    next_data = allocate_register (next_data, D_REG);
    ap->deep = reg_in_use[ap->preg];
    return ap;
}


/*
 *   Allocate 2 temporary data registers and return
 *   it's addressing mode.
 */
ADDRESS *mdata_register P0 (void)
{
    REG     reg1;
    REG     reg2;
    ADDRESS *ap;

    reg1 = next_data;
    next_data = allocate_register (next_data, D_REG);
    reg2 = next_data;
    next_data = allocate_register (next_data, D_REG);
    ap = mk_mreg (reg1, reg2);
    ap->deep = reg_in_use[reg1];
    associated_regs[reg1] |= REGBIT (reg2);
    associated_regs[reg2] |= REGBIT (reg1);
    return ap;
}

/*
 *   Allocate 3 temporary data registers and return
 *   it's addressing mode.
 */
ADDRESS *xdata_register P0 (void)
{
    REG     reg1;
    REG     reg2;
    REG     reg3;
    ADDRESS *ap;

    reg1 = next_data;
    next_data = allocate_register (next_data, D_REG);
    reg2 = next_data;
    next_data = allocate_register (next_data, D_REG);
    reg3 = next_data;
    next_data = allocate_register (next_data, D_REG);
    ap = mk_xreg (reg1, reg2, reg3);
    ap->deep = reg_in_use[reg1];
    associated_regs[reg1] |= (REGBIT (reg2) | REGBIT (reg3));
    associated_regs[reg2] |= (REGBIT (reg1) | REGBIT (reg3));
    associated_regs[reg3] |= (REGBIT (reg1) | REGBIT (reg2));
    return ap;
}


/*
 *   Allocate a temporary addr register and return it's addressing mode.
 */
ADDRESS *address_register P0 (void)
{
    ADDRESS *ap = mk_reg (next_addr);

    next_addr = allocate_register (next_addr, A_REG);
    ap->deep = reg_in_use[ap->preg];
    return ap;
}

#ifdef FLOAT_IEEE
/*
 *   Allocate a temporary floating point register and return it's
 *   addressing mode.
 */
ADDRESS *float_register P0 (void)
{
    ADDRESS *ap = mk_reg (next_float);

    next_float = allocate_register (next_float, F_REG);
    ap->deep = reg_in_use[ap->preg];
    return ap;
}
#endif /* FLOAT_IEEE */

/*
 *   Returns TRUE if a data register is available at ,,no cost'' (no push).
 *   Used to determine e.g. whether cmp.w #0,An or move.l An,Dm is better
 */
BOOL is_free_data P0 (void)
{
    return (reg_in_use[next_data] == UNUSED);
}


/*
 *   returns TRUE if an address register is available at
 *   ,,no cost'' (no push).
 */
BOOL is_free_addr P0 (void)
{
    return (reg_in_use[next_addr] == UNUSED);
}


/*
 *   Allocates a data or addressing register (whichever is free).
 *   Otherwise allocates the first register which matches flags.
 */
ADDRESS *temp_reg P1 (FLAGS, flags)
{
    if (is_free_data () && (flags & F_DREG)) {
	return data_register ();
    }
    if (is_free_addr () && (flags & F_AREG)) {
	return address_register ();
    }
    if (flags & F_DREG) {
	return data_register ();
    }
    if (flags & F_AREG) {
	return address_register ();
    }
#ifdef FLOAT_IEEE
    if (flags & F_FREG) {
	return float_register ();
    }
#endif /* FLOAT_IEEE */
    return NIL_ADDRESS;
}


/*
 *   Deallocate the specified register.
 */
static void deallocate_register P1 (REG, reg)
{
    DEEP    depth;

    if (!is_temporary_register (reg)) {
	return;
    }
    if (is_data_register (reg)) {
	next_data = prev_reg (next_data, D_REG);
    } else if (is_address_register (reg)) {
	next_addr = prev_reg (next_addr, A_REG);
#ifdef FLOAT_IEEE
    } else if (is_float_register (reg)) {
	next_float = prev_reg (next_float, F_REG);
#endif /* FLOAT_IEEE */
    }
    depth = reg_in_use[reg];
    reg_in_use[reg] = UNUSED;

    /* we should only free the most recently allocated register */
    if (alloc_depth-- == EMPTY) {
	FATAL ((__FILE__, "deallocate_register", "register stack empty"));
    }
    if (alloc_depth != depth) {
	FATAL ((__FILE__, "deallocate_register", "register stack order"));
    }
    /* the just freed register should not be on stack */
    if (reg_alloc[depth].pushed) {
	FATAL ((__FILE__, "deallocate_register", "register pushed"));
    }
}


/*
 *   Release any temporary registers used in an addressing mode.
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
    case am_xreg:
	deallocate_register (ap->u.xreg);
	/*lint -fallthrough */
    case am_mreg:
    case am_indx2:
    case am_indx3:
    case am_indx4:
	deallocate_register (ap->sreg);
	/*lint -fallthrough */
    case am_dreg:
    case am_areg:
    case am_ainc:
    case am_adec:
    case am_ind:
    case am_indx:
#ifdef FLOAT_IEEE
    case am_freg:
#endif /* FLOAT_IEEE */
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
	FATAL ((__FILE__, "freeop", "1"));
    }
}


/*
 *   Push any used temporary registers.
 *
 *   This is necessary across function calls
 *   The reason for this hacking is actually that temp_inv()
 *   should dump the registers in the correct order,
 *
 *   The least recently allocate register first.
 *   The most recently allocated register last.
 */
void temp_inv P1 (REGUSAGE *, regusage)
{
    DEEP    deep;

    UNUSEDARG (regusage);

    for (deep = EMPTY; deep < alloc_depth; deep++)
	if (!reg_alloc[deep].pushed) {
	    g_push (reg_alloc[deep].reg, deep);
	    /* mark the register void */
	    reg_in_use[reg_alloc[deep].reg] = UNUSED;
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


/*
 *   Find a register of type 'kind' for a register variable.
 *   The 'mask' parameter specifies those registers which are not to be
 *   allocated.   If there are no register suitable then
 *   NO_REG is returned.
 */
REG find_register P2 (REGTYPE, kind, REGMASK, mask)
{
    REG     reg;

    for (reg = D0; reg <= MAX_REG; reg++) {
	if (mask & REGBIT (reg)) {
	    continue;
	}
	if (regtype[reg] & kind) {
	    return reg;
	}
    }
    return NO_REG;
}

/*
 *   Count the number of registers that have been specified in the
 *   register mask.
 */
int count_registers P1 (REGMASK, mask)
{
    int     count;
    REG     reg;

    for (count = 0, reg = D0; reg <= MAX_REG; reg++) {
	if (mask & REGBIT (reg)) {
	    count++;
	}
    }
    return count;
}
#endif /* MC680X0 */
