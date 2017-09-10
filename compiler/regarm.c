/*
 * C compiler
 * ==========
 *
 * Copyright 1994 Keith Walker
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 *
 * History:
 *
 *   1994       Started ARM code generator
 */

/******************************************************************************
 *
 * Register allocation (for the expression evaluation)
 * This modules handles the management of scratch registers.
 * It keeps track of the allocated registers and of the stack
 *
 *****************************************************************************/

#include "config.h"

#ifdef ARM

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genarm.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define	MAX_REG_STACK	((DEEP)30)
#define	UNUSED		((DEEP)-1)
#define	EMPTY		((DEEP)0)

/*********************************************** Static Function Definitions */

static void g_push P_ ((REG, DEEP));
static void g_pop P_ ((REG, DEEP));

/********************************************************** Static Variables */

static DEEP reg_in_use[NUM_REGS];
static REG next_reg;		/* next temporary register */
REG     max_reg = MAX_REG;	/* maximum temporary register */

static struct
{
    REG     reg;
    DEEP    depth;
} reg_stack[(int) MAX_REG_STACK + 1];
static DEEP reg_stack_ptr;

static struct
{
    REG     reg;
    BOOL    pushed;
} reg_alloc[(int) MAX_REG_STACK + 1];
static DEEP reg_depth;

/*
 *   Define the registers which can be used to pass parameters.
 */
static REG parameter_registers[] = {
    R0, R1, R2, R3
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
    R4, R5, R6, R7, R8, R9
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
    R0
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

/*****************************************************************************/

/*
 * this routine generates code to push a register onto the stack
 */
static void g_push P2 (REG, reg, DEEP, number)
{
    ADDRESS *ap;

    ap = mk_reg (reg);
    g_code (op_mov, cc_al, ap, NIL_ADDRESS, NIL_ADDRESS);
    reg_stack[reg_stack_ptr].reg = reg;
    reg_stack[reg_stack_ptr].depth = number;

    /* is already pushed */
    if (reg_alloc[number].pushed) {
	FATAL ((__FILE__, "g_push", "1"));
    }
    reg_alloc[number].pushed = TRUE;

    /* check on stack overflow */
    if (++reg_stack_ptr > MAX_REG_STACK) {
	FATAL ((__FILE__, "g_push", "2"));
    }
}

/*
 * generate code to pop a register from the stack.
 */
static void g_pop P2 (REG, reg, DEEP, number)
{
    ADDRESS *ap;

    /* check on stack underflow */
    if (reg_stack_ptr-- == EMPTY) {
	FATAL ((__FILE__, "g_pop", "1"));
    }
    /* check if the desired register really is on stack */
    if (reg_stack[reg_stack_ptr].depth != number) {
	FATAL ((__FILE__, "g_pop", "2"));
    }
    /* check if the register which is restored is really void */
    if (reg_in_use[reg] != UNUSED) {
	FATAL ((__FILE__, "g_pop", "3"));
    }
    reg_in_use[reg] = number;
    ap = mk_reg (reg);
    g_code (op_mov, cc_al, ap, NIL_ADDRESS, NIL_ADDRESS);

    /* clear the push_flag */
    reg_alloc[number].pushed = FALSE;
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

    for (reg = R0; reg <= max_reg; reg++)
	reg_in_use[reg] = UNUSED;
    next_reg = R0;
    reg_stack_ptr = EMPTY;
    reg_depth = EMPTY;
    act_scratch = 0;
}

/*
 * this routine checks if all allocated registers were freed correctly
 */
void checkstack P0 (void)
{
    REG     reg;

    for (reg = R0; reg <= max_reg; reg++)
	if (reg_in_use[reg] != UNUSED) {
	    FATAL ((__FILE__, "checkstack", "1"));
	}
    if (next_reg != R0) {
	FATAL ((__FILE__, "checkstack", "2"));
    }
    if (reg_stack_ptr != EMPTY) {
	FATAL ((__FILE__, "checkstack", "3"));
    }
    if (reg_depth != EMPTY) {
	FATAL ((__FILE__, "checkstack", "4"));
    }
}

/*
 * validate will make sure that if a register within an address mode has been
 * pushed onto the stack that it is popped back at this time.
 */
void validate P1 (const ADDRESS *, ap)
{
    REG     reg;

    switch (ap->mode) {
    case am_reg:
	reg = ap->preg;
	break;
    default:
	return;
    }
    if (reg > max_reg) {
	return;
    }
    if (reg_alloc[ap->deep].pushed) {
	g_pop (reg, ap->deep);
    }
}


/*
 * allocate a register
 */
ADDRESS *data_register P0 (void)
{
    ADDRESS *ap;

    /*
     * if the register is in use, push it to the stack
     */
    if (reg_in_use[next_reg] != UNUSED) {
	g_push (next_reg, reg_in_use[next_reg]);
    }
    reg_in_use[next_reg] = reg_depth;
    ap = mk_reg (next_reg);
    ap->deep = reg_depth;
    reg_alloc[reg_depth].reg = next_reg;
    reg_alloc[reg_depth].pushed = FALSE;

    if (next_reg++ == max_reg) {
	next_reg = R0;		/* wrap around */
    }
    if (reg_depth++ == MAX_REG_STACK) {
	FATAL ((__FILE__, "data_register", ""));
    }
    return ap;
}

#if 0
/*
 * tells if an address mode uses a scratch register
 */
BOOL uses_temp P1 (const ADDRESS *, ap)
{
    if (ap == NIL_ADDRESS) {
	FATAL ((__FILE__, "uses_temp", ""));
    }
    switch (ap->mode) {
    case am_reg:
	return (ap->preg <= max_reg);
    default:
	return FALSE;
    }
}
#endif

/*
 * release any temporary registers used in an addressing mode.
 */
void freeop P1 (const ADDRESS *, ap)
{
    DEEP    number;
    REG     reg;

    if (ap == NIL_ADDRESS) {
	/* This can happen freeing a NOVALUE result */
	return;
    }
    switch (ap->mode) {
    case am_reg:
	reg = ap->preg;
	if (reg <= max_reg) {
	    if (next_reg-- == R0) {
		next_reg = max_reg;
	    }
	    number = reg_in_use[reg];
	    reg_in_use[reg] = UNUSED;
	    break;
	}
	return;
    default:
	return;
    }

    /* some consistency checks */
    if (number != ap->deep) {
	FATAL (
	       (__FILE__, "freeop", "1 (%d, %d, %d)", (int) reg, (int) number,
		(int) ap->deep));
    }
    /* we should only free the most recently allocated register */
    if (reg_depth-- == EMPTY) {
	FATAL ((__FILE__, "freeop", "2"));
    }
    if (reg_depth != number) {
	FATAL ((__FILE__, "freeop", "3"));
    }
    /* the just freed register should not be on stack */
    if (reg_alloc[number].pushed) {
	FATAL ((__FILE__, "freeop", "4"));
    }
}

/*
 * push any used temporary registers.
 * This is necessary across function calls
 * The reason for this hacking is actually that temp_inv should dump
 * the registers in the correct order,
 * the least recently allocate register first.
 * the most recently allocated register last.
 */
void temp_inv P0 (void)
{
    DEEP    deep;

    for (deep = EMPTY; deep < reg_depth; deep++)
	if (!reg_alloc[deep].pushed) {
	    g_push (reg_alloc[deep].reg, deep);
	    /* mark the register void */
	    reg_in_use[reg_alloc[deep].reg] = UNUSED;
	}
}
#endif /* ARM */
