/*
 * C compiler
 * ==========
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

/******************************************************************************
 *
 * Register allocation (for the expression evaluation)
 * This modules handles the management of scratch registers.
 * It keeps track of the allocated registers and of the stack
 *
 *****************************************************************************/

#include "config.h"

#ifdef POWERPC

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genppc.h"
#include "outproto.h"

/********************************************************** Static Variables */

/*
 *    This data structure is used to keep track of registers which
 *      have been pushed onto the stack.
 */
static struct
{
    REG     reg;
    DEEP    depth;
} reg_stack[(int) MAX_REG_STACK + 1];
static DEEP stack_depth;

/*
 *    This data structure is used to keep track of register which
 *      have been allocated.
 */
static struct
{
    REG     reg;
    BOOL    pushed;
} reg_alloc[(int) MAX_REG_STACK + 1];
static DEEP alloc_depth;

/*
 *    Define the registers which can be used to pass parameters.
 */
static REG parameter_registers[] = {
    GPR0, GPR1, GPR2, GPR3, GPR4
};
static REGLIST parameter_list = {
    (int) (sizeof (parameter_registers) / sizeof (REG)),
    &parameter_registers[0]
};

/*
 *    Define the registers which must be saved by a function if
 *      they are used.
 *      If the register is used to return a value then it needn't be
 *      saved.
 *      If the register is used to pass a parameter then it needn't be
 *      saved.
 */
static REG saved_registers[] = {
    GPR5, GPR6, GPR7, GPR8, GPR9
};
static REGLIST saved_list = {
    (int) (sizeof (saved_registers) / sizeof (REG)),
    &saved_registers[0]
};

/*
 *    Define the registers which are used to return the results
 *      from a function call.
 */
static REG result_registers[] = {
    GPR0, GPR1, GPR2
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

REGTYPE regtype[] = {
    D_REG | T_REG,		/* D0 */
    D_REG | T_REG,		/* D1 */
    D_REG | T_REG,		/* D2 */
    D_REG,			/* D3 */
    D_REG,			/* D4 */
    D_REG,			/* D5 */
    D_REG,			/* D6 */
    D_REG,			/* D7 */
    A_REG | T_REG,		/* A0 */
    A_REG | T_REG,		/* A1 */
    A_REG,			/* A2 */
    A_REG,			/* A3 */
    A_REG,			/* A4 */
    A_REG,			/* A5 */
    A_REG,			/* A6 */
    A_REG,			/* A7 */
};

static DEEP reg_in_use[NUM_REGS];
static REG next_data;		/* next temporary data register */
static REG next_addr;		/* next temporary address register */

#ifdef FLOAT_IEEE
static REG next_float;		/* next temporary floating point register */

#endif /* FLOAT_IEEE */

/*****************************************************************************/

/*
 *   This routine generates code to push a register onto the stack.
 */
static void g_push P2 (REG, reg, DEEP, depth)
{
}

/*
 *    This routine should be called before each expression is
 *      evaluated to make sure the stack is balanced and all of
 *      the registers are marked free.
 *
 *      This is also a good place to free all 'pseudo' registers
 *      in the stack frame by setting act_scratch to zero.
 */
void initstack P0 (void)
{
    REG     reg;

    next_data = GPR0;
    for (reg = GPR0; reg < FPR31; reg++)
	reg_in_use[reg] = UNUSED;
    stack_depth = EMPTY;
    alloc_depth = EMPTY;
    act_scratch = 0;
}


/*
 *    This routines checks if all allocated registers were freed.
 */
void checkstack P0 (void)
{
    REG     reg;

    if (next_data != GPR0) {
	FATAL ((__FILE__, "checkstack", "GPR0 not next temporary"));
    }
    for (reg = GPR0; reg <= FPR31; reg++) {
	if (!(regtype[reg] & T_REG)) {
	    continue;
	}
	if (reg_in_use[reg] != UNUSED) {
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
 *    Validate will make sure that if a register within an address
 *      mode has been pushed onto the stack that it is popped back
 *      at this time.
 */
void validate P1 (const ADDRESS *, ap)
{
    REG     reg;

    switch (ap->mode) {
    }
}

/*
 *    Return the next register of type 'kind'
 */
static REG next_reg P2 (REG, reg, REGTYPE, kind)
{
    for (;;) {
	reg = (reg == FPR31) ? GPR0 : (REG) ((int) reg + 1);
	if (((regtype[reg] & T_REG)) && (regtype[reg] & kind)) {
	    return reg;
	}
    }
}

/*
 *    Return the previous register of type 'kind'
 */
static REG prev_reg P2 (REG, reg, REGTYPE, kind)
{
    for (;;) {
	reg = (reg == GPR0) ? FPR31 : (REG) ((int) reg - 1);
	if (((regtype[reg] & T_REG)) && (regtype[reg] & kind)) {
	    return reg;
	}
    }
}

/*
 *    Allocate a temporary register.    Returns the next register
 *      that will be allocated.
 */
static REG allocate_register P2 (REG, reg, REGTYPE, kind)
{
    if (reg_in_use[reg] != UNUSED) {
	/*
	 *    The next available register is already in use.
	 *      It must be pushed.
	 */
	g_push (reg, reg_in_use[reg]);
    }
    reg_in_use[reg] = alloc_depth;
    reg_alloc[alloc_depth].reg = reg;
    reg_alloc[alloc_depth].pushed = FALSE;

    if (alloc_depth++ == MAX_REG_STACK) {
	FATAL ((__FILE__, "allocate_register", "register stack overflow"));
    }
    return next_reg (reg, kind);
}

/*
 *    Allocate a temporary data register and return
 *      it's addressing mode.
 */
ADDRESS *data_register P0 (void)
{
    ADDRESS *ap = mk_reg (next_data);

    next_data = allocate_register (next_data, D_REG);
    ap->deep = reg_in_use[ap->preg];
    return ap;
}

/*
 *    Allocate a temporary addr register and return it's addressing mode.
 */
ADDRESS *address_register P0 (void)
{
    ADDRESS *ap = mk_reg (next_addr);

    next_addr = allocate_register (next_addr, A_REG);
    ap->deep = reg_in_use[ap->preg];
    return ap;
}

/*
 *    Returns TRUE if a data register is available at ,,no cost'' (no push).
 *      Used to determine e.g. whether cmp.w #0,An or move.l An,Dm is better
 */
BOOL is_free_data P0 (void)
{
    return (reg_in_use[next_data] == UNUSED);
}


/*
 *    returns TRUE if an address register is available at
 *      ,,no cost'' (no push).
 */
BOOL is_free_addr P0 (void)
{
    return (reg_in_use[next_addr] == UNUSED);
}


/*
 *    Allocates a data or addressing register (whichever is free).
 *      Otherwise allocates the first register which matches flags.
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
    return NIL_ADDRESS;
}


/*
 *    Deallocate the specified register.
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
 *    Release any temporary registers used in an addressing mode.
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
 *    Push any used temporary registers.
 *
 *      This is necessary across function calls
 *      The reason for this hacking is actually that temp_inv()
 *      should dump the registers in the correct order,
 *
 *      The least recently allocate register first.
 *      The most recently allocated register last.
 */
void temp_inv P0 (void)
{
    DEEP    deep;

    for (deep = EMPTY; deep < alloc_depth; deep++)
	if (!reg_alloc[deep].pushed) {
	    g_push (reg_alloc[deep].reg, deep);
	    /* mark the register void */
	    reg_in_use[reg_alloc[deep].reg] = UNUSED;
	}
}


/*
 *    Converts a list of registers into a register mask
 */
REGMASK reglist_to_mask P1 (const REGLIST *, rp)
{
    REGMASK mask = (REGMASK) 0;
    int     num;

    for (num = 0; num < rp->number; num++) {
	mask |= (REGMASK) (1 << (int) rp->reg[num]);
    }
    return mask;
}
#endif /* POWERPC */
