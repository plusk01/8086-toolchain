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
 * 1995   Ivo Oesch, Started to build a codegenerator for the
 *        signalprocessor TMS320C30 (December)
 */

/******************************************************************************
 *
 * Register allocation (for the expression evaluation)
 * This modules handles the management of scratch registers.
 * It keeps track of the allocated registers and of the stack
 *
 *****************************************************************************/

#include "config.h"

#ifdef TMS320C30

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genc30.h"
#include "outproto.h"

/*********************************************** Static Function Definitions */

static void g_push P_ ((REG, REGTYPE, DEEP));
static void g_pop P_ ((REG, DEEP));
static REG next_reg P_ ((REG, REGTYPE));

/********************************************************** Static Variables */

/*
 *      This data structure is used to keep track of registers which
 *      have been pushed onto the stack.
 */
static struct
{
    REG     reg;
    REGTYPE kind;
    DEEP    depth;
} reg_stack[(int) MAX_REG_STACK + 1];
static DEEP stack_depth;

/*
 *      This data structure is used to keep track of register which
 *      have been allocated.
 */
static struct
{
    REG     reg;
    REGTYPE kind;
    BOOL    pushed;
} reg_alloc[(int) MAX_REG_STACK + 1];
static DEEP alloc_depth;

/*
 *      Define the registers which can be used to pass parameters.
 */
static REG parameter_registers[] = {
    REG_R0, REG_R1, REG_R2, REG_AR0, REG_AR1
};
static REGLIST parameter_list = {
    (int) (sizeof (parameter_registers) / sizeof (REG)),
    &parameter_registers[0]
};

/*
 *      Define the registers which must be saved by a function if
 *      they are used.
 *      If the register is used to return a value then it needn't be
 *      saved.
 *      If the register is used to pass a parameter then it needn't be
 *      saved.
 */
static REG saved_registers[] = {
    REG_R3, REG_R4, REG_R5, REG_R6, REG_R7,
    REG_AR2, REG_AR3, REG_AR4, REG_AR5, REG_AR6, REG_AR7
};

static REGLIST saved_list = {
    (int) (sizeof (saved_registers) / sizeof (REG)),
    &saved_registers[0]
};

/*
 *      Define the registers which are used to return the results
 *      from a function call.
 */
static REG result_registers[] = {
    REG_R0
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
    D_REG | F_REG | T_REG,	/* R0  */
    D_REG | F_REG | T_REG,	/* R1  */
    D_REG | F_REG | T_REG,	/* R2  */
    D_REG | F_REG,		/* R3  */
    D_REG | F_REG,		/* R4  */
    D_REG | F_REG,		/* R5  */
    D_REG | F_REG,		/* R6  */
    D_REG | F_REG,		/* R7  */
    A_REG | T_REG,		/* AR0 */
    A_REG | T_REG,		/* AR1 */
    A_REG,			/* AR2 */
    A_REG,			/* AR3 */
    A_REG,			/* AR4 */
    A_REG,			/* AR5 */
    A_REG,			/* AR6 */
    A_REG,			/* AR7 */
    S_REG,			/* DP  */
    I_REG | T_REG,		/* IR0 */
    I_REG | T_REG,		/* IR1 */
    S_REG,			/* BK  */
    S_REG,			/* SP  */
    S_REG,			/* ST  */
    S_REG,			/* IE  */
    S_REG,			/* IF  */
    S_REG,			/* IOF */
    S_REG,			/* RS  */
    S_REG,			/* RE  */
    S_REG,			/* RC  */
};

static DEEP reg_in_use[MAX_REG + 1];

static REG next_data;		/* next temporary data register */
static REG next_addr;		/* next temporary address register */
static REG next_index;		/* next temporary index register */

/*****************************************************************************/

/*
 * this routine generates code to push a register onto the stack
 */
static void g_push P3 (REG, reg, REGTYPE, kind, DEEP, depth)
{
    ADDRESS *ap;

    sync_stack ();
    ap = mk_reg (reg);
    if (kind == F_REG)
	g_code (op_pushf, OP_FLOAT, ap, NIL_ADDRESS);
    else
	g_code (op_push, OP_INT, ap, NIL_ADDRESS);

    reg_stack[stack_depth].kind = kind;
    reg_stack[stack_depth].reg = reg;
    reg_stack[stack_depth].depth = depth;

    if (reg_alloc[depth].pushed)
	FATAL ((__FILE__, "g_push", "reg %d already pushed", (int) reg));
    reg_alloc[depth].pushed = TRUE;

    /* check on stack overflow */
    if (++stack_depth > MAX_REG_STACK)
	FATAL ((__FILE__, "g_push", "register stack overflow"));
}

/*
 * generate code to pop a register from the stack.
 */
static void g_pop P2 (REG, reg, DEEP, depth)
{
    ADDRESS *ap;

    /* check on stack underflow */
    if (stack_depth-- == EMPTY)
	FATAL ((__FILE__, "g_pop", "register stack empty"));

    /* check if the desired register really is on stack */
    if (reg_stack[stack_depth].depth != depth)
	FATAL (
	       (__FILE__, "g_pop", "register order (%d, %d)", (int) depth,
		(int) reg_stack[stack_depth].depth));

    /* check if the register which is restored is really void */
    if (reg_in_use[reg] != UNUSED)
	FATAL ((__FILE__, "g_pop", "register %d in use", (int) reg));

    reg_in_use[reg] = depth;
    sync_stack ();
    ap = mk_reg (reg);
    if (reg_stack[stack_depth].kind == F_REG)
	g_code (op_popf, OP_INT, NIL_ADDRESS, ap);
    else
	g_code (op_pop, OP_INT, NIL_ADDRESS, ap);

    /* clear the push_flag */
    reg_alloc[depth].pushed = FALSE;
}

/*
 * this routine should be called before each expression is evaluated to make
 * sure the stack is balanced and all of the registers are marked free.
 * This is also a good place to free all 'pseudo' registers in the
 * stack frame by setting act_scratch to zero
 */
void initstack P0 (void)
{
    REG     reg;

    next_data = REG_R0;
    next_addr = REG_AR0;
    next_index = REG_IR0;
    for (reg = REG_R0; reg <= MAX_REG; reg++)
	reg_in_use[reg] = UNUSED;
    stack_depth = EMPTY;
    alloc_depth = EMPTY;
    act_scratch = 0;
}

/*
 * this routines checks if all allocated registers were freed
 */
void checkstack P0 (void)
{
    REG     reg;

    if (next_data != REG_R0)
	FATAL ((__FILE__, "checkstack", "R0 not next temporary"));
    if (next_addr != REG_AR0)
	FATAL ((__FILE__, "checkstack", "AR0 not next temporary"));
    if (next_index != REG_IR0)
	FATAL ((__FILE__, "checkstack", "IR0 not next temporary"));
    if (stack_depth != EMPTY)
	FATAL ((__FILE__, "checkstack", "register stack not empty"));
    if (alloc_depth != EMPTY)
	FATAL (
	       (__FILE__, "checkstack",
		"allocated register stack not empty"));
    for (reg = REG_R0; reg <= MAX_REG; reg++) {
	if (!(regtype[reg] & T_REG))
	    continue;
	if (reg_in_use[reg] != UNUSED)
	    FATAL (
		   (__FILE__, "checkstack", "register %d still in use",
		    (int) reg));
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
    case am_dreg:
	reg = ap->preg;
	if (is_temporary_data_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    case am_freg:
	reg = ap->preg;
	if (is_temporary_data_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    case am_ireg:
	reg = ap->preg;
	if (is_temporary_index_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    case am_indx2:
    case am_indxs:
	reg = ap->sreg;
	if (is_temporary_index_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	/*lint -fallthrough */
    case am_areg:
    case am_ind:
    case am_const_ind:
    case am_indx:
    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
	reg = ap->preg;
	if (is_temporary_address_register (reg) && reg_alloc[ap->deep].pushed) {
	    g_pop (reg, ap->deep);
	}
	break;
    default:
	break;
    }
}

/*
 *      Return the next register of type 'kind'
 */
static REG next_reg P2 (REG, reg, REGTYPE, kind)
{
    for (;;) {
	reg = (reg >= MAX_REG) ? REG_R0 : (REG) ((int) reg + 1);
	if (((regtype[reg] & T_REG)) && (regtype[reg] & kind))
	    return reg;
    }
}

/*
 *      Return the previous register of type 'kind'
 */
static REG prev_reg P2 (REG, reg, REGTYPE, kind)
{
    for (;;) {
	reg = (reg <= REG_R0) ? MAX_REG : (REG) ((int) reg - 1);
	if (((regtype[reg] & T_REG)) && (regtype[reg] & kind))
	    return reg;
    }
}

/*
 *      Allocate a temporary register.    Returns the next register
 *      that will be allocated.
 */

static REG allocate_register P2 (REG, reg, REGTYPE, kind)
{
    if (reg_in_use[reg] != UNUSED) {
	/*
	 *      The next available register is already in use.
	 *      It must be pushed.
	 */
	DEEP    depth = reg_in_use[reg];

	g_push (reg, reg_alloc[depth].kind, depth);
    }
    reg_in_use[reg] = alloc_depth;
    reg_alloc[alloc_depth].reg = reg;
    reg_alloc[alloc_depth].pushed = FALSE;
    reg_alloc[alloc_depth].kind = kind;

    if (alloc_depth++ == MAX_REG_STACK)
	FATAL ((__FILE__, "allocate_register", "register stack overflow"));
    return next_reg (reg, kind);
}

/*
 *      Allocate a temporary data register and return
 *      it's addressing mode.
 */
ADDRESS *data_register P1 (FLAGS, flags)
{
    ADDRESS *ap = mk_reg (next_data);

    next_data =
	allocate_register (next_data,
			   (REGTYPE) ((flags == F_FREG) ? F_REG : D_REG));
    ap->mode = (flags == F_FREG) ? am_freg : am_dreg;
    ap->deep = reg_in_use[ap->preg];
    return ap;
}


/*
 *      Allocate a temporary addr register and return it's addressing mode.
 */
ADDRESS *address_register P0 (void)
{
    ADDRESS *ap = mk_reg (next_addr);

    next_addr = allocate_register (next_addr, A_REG);
    ap->deep = reg_in_use[ap->preg];
    return ap;
}


/*
 *      Allocate a temporary index register and return it's addressing mode.
 */
ADDRESS *index_register P0 (void)
{
    ADDRESS *ap = mk_reg (next_index);

    next_index = allocate_register (next_index, I_REG);
    ap->deep = reg_in_use[ap->preg];
    return ap;
}


/*
 * returns TRUE if a data register is available at ,,no cost'' (no push).
 * Used to determine e.g. whether cmp.w #0,An or move.l An,Dm is better
 */
BOOL is_free_data P0 (void)
{
    return (reg_in_use[next_data] == UNUSED);
}

/*
 * returns TRUE if an address register is available at ,,no cost'' (no push).
 */
BOOL is_free_addr P0 (void)
{
    return (reg_in_use[next_addr] == UNUSED);
}

/*
 * returns TRUE if an address register is available at ,,no cost'' (no push).
 */
BOOL is_free_ireg P0 (void)
{
    return (reg_in_use[next_index] == UNUSED);
}

ADDRESS *temporary_register P1 (FLAGS, flags)
{
    if (is_free_data () && (flags & F_DREG))
	return data_register (F_DREG);
    if (is_free_data () && (flags & F_FREG))
	return data_register (F_FREG);
    if (is_free_addr () && (flags & F_AREG))
	return address_register ();
    if (is_free_ireg () && (flags & F_IREG))
	return index_register ();
    if (flags & F_DREG)
	return data_register (F_DREG);
    if (flags & F_AREG)
	return address_register ();
    if (flags & F_IREG)
	return index_register ();
    if (flags & F_FREG)
	return data_register (F_FREG);
    FATAL ((__FILE__, "temporary_register", "no free registers"));
    /* NOTREACHED */
    return NIL_ADDRESS;
}


/*
 *      Deallocate the specified register.
 */
static void deallocate_register P1 (REG, reg)
{
    DEEP    depth;

    if (!is_temporary_register (reg))
	return;
    if (is_data_register (reg)) {
	next_data = prev_reg (next_data, D_REG);
    } else if (is_address_register (reg)) {
	next_addr = prev_reg (next_addr, A_REG);
    } else if (is_index_register (reg)) {
	next_index = prev_reg (next_index, I_REG);
    }
    depth = reg_in_use[reg];
    reg_in_use[reg] = UNUSED;

    /* we should only free the most recently allocated register */
    if (alloc_depth-- == EMPTY)
	FATAL ((__FILE__, "deallocate_register", "register stack empty"));

    if (alloc_depth != depth)
	FATAL ((__FILE__, "deallocate_register", "register stack order"));

    /* the just freed register should not be on stack */
    if (reg_alloc[depth].pushed)
	FATAL ((__FILE__, "deallocate_register", "register pushed"));
}

/*
 * release any temporary registers used in an addressing mode.
 */
void freeop P1 (const ADDRESS *, ap)
{
    DEEP    depth;
    REG     reg;

    if (ap == NIL_ADDRESS)
	/* This can happen freeing a NOVALUE result */
	return;
    switch (ap->mode) {
    case am_indx2:
    case am_indxs:
	deallocate_register (ap->sreg);
	/*lint -fallthrough */
    case am_freg:
    case am_dreg:
    case am_ireg:
    case am_areg:
    case am_ind:
    case am_const_ind:
    case am_indx:
    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
	reg = ap->preg;
	break;
    default:
	return;
    }
    if (!is_temporary_register (reg))
	return;

    depth = reg_in_use[reg];
    deallocate_register (reg);

    /* some consistency checks */
    if (depth != ap->deep)
	FATAL ((__FILE__, "freeop", "1"));
}

#if 0
void save_blockrepeat P0 (void)
{
}

void restore_blockrepeat P0 (void)
{
}

#endif


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
    DEEP    depth;

    for (depth = EMPTY; depth < alloc_depth; depth++) {
	if (!reg_alloc[depth].pushed) {
	    g_push (reg_alloc[depth].reg, reg_alloc[depth].kind, depth);
	    /* mark the register void */
	    reg_in_use[reg_alloc[depth].reg] = UNUSED;
	}
    }
}

#if 0
/*
 *   Find a register of type 'kind' for a register variable.
 *   The 'mask' parameter specifies those registers which are not to be
 *   allocated.   If there are no register suitable then
 *   NO_REG is returned.
 */
REG find_register P2 (REGTYPE, kind, REGMASK, mask)
{
    REG     reg;

    for (reg = REG_R0; reg <= MAX_REG; reg++) {
	if (mask & (REGMASK) (1 << (int) reg)) {
	    continue;
	}
	if (regtype[reg] & kind) {
	    return reg;
	}
    }
    return NO_REG;
}
#endif

#endif /* TMS320C30 */
