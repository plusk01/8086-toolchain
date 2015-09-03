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
 *  Examines dataflow and checks if data is already in a register
 *  when read-accesses to external ram occurs
 *  needs multiple-passes to work
 */

/*****************************************************************************/

#include "config.h"

#ifdef PEEPFLOW
#ifdef MC680X0
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "gen68k.h"
#include "outproto.h"

/********************************************************* Macro Definitions */


/*****************************************************************************/

#ifdef DEBUG
/*
 * generate a register name from a tempref number.
 */
static void putreg P1 (REG, r)
{
    switch (r) {
    case D0:
    case D1:
    case D2:
    case D3:
    case D4:
    case D5:
    case D6:
    case D7:
	dprintf (DEBUG_FLOW, "d%d", (int) r);
	break;
    case A0:
    case A1:
    case A2:
    case A3:
    case A4:
    case A5:
    case A6:
    case A7:
	dprintf (DEBUG_FLOW, "a%d", (int) r - (int) A0);
	break;
    case FP0:
    case FP1:
    case FP2:
    case FP3:
    case FP4:
    case FP5:
    case FP6:
    case FP7:
	dprintf (DEBUG_FLOW, "fp%d", (int) r - (int) FP0);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}

/*
 * generate a register mask.
 */
static void put_mask P1 (REGMASK, mask)
{
    REG     reg;
    BOOL    pending = FALSE;

    for (reg = D0; reg <= FP7; reg++) {
	if (mask & (REGMASK) 1) {
	    if (pending) {
		dprintf (DEBUG_FLOW, "/");
	    }
	    putreg (reg);
	    pending = TRUE;
	}
	mask >>= 1;
    }
}

/*
 * generate a register mask for save.
 */
static void put_smask P1 (REGMASK, mask)
{
    put_mask (mask);
}

/*
 * generate a register mask for restore.
 */
static void put_rmask P1 (REGMASK, mask)
{
    put_mask (mask);
}

/*
 * put a constant to the output file.
 */
static void putconst P1 (const EXPR *, ep)
{
    switch (ep->nodetype) {
    case en_autocon:
    case en_icon:
	dprintf (DEBUG_FLOW, "%ld", ep->v.i);
	break;
#ifndef FLOAT_BOOTSTRAP
#ifdef FLOAT_MFFP
    case en_fcon:
	dprintf (DEBUG_FLOW, "0x%lx", genffp (ep->v.f));
	break;
#endif /* FLOAT_MFFP */
#endif /* FLOAT_BOOTSTRAP */
    case en_labcon:
	dprintf (DEBUG_FLOW, "I%u", (unsigned) ep->v.l);
	break;
    case en_nacon:
	dprintf (DEBUG_FLOW, "%s", ep->v.str);
	break;
    case en_sym:
	dprintf (DEBUG_FLOW, "%s", nameof (ep->v.sp));
	break;
    case en_add:
	putconst (ep->v.p[0]);
	dprintf (DEBUG_FLOW, "+");
	putconst (ep->v.p[1]);
	break;
    case en_sub:
	putconst (ep->v.p[0]);
	dprintf (DEBUG_FLOW, "-");
	putconst (ep->v.p[1]);
	break;
    case en_uminus:
	dprintf (DEBUG_FLOW, "-");
	/*FALLTHRU */
    case en_cast:
	putconst (ep->v.p[0]);
	break;
    case en_str:
	dprintf (DEBUG_FLOW, "%s", ep->v.str);
	break;
    default:
	FATAL (
	       (__FILE__, "putconst", "illegal constant node %d",
		ep->nodetype));
	break;
    }
}

/*
 * output a general addressing mode.
 */
static void putamode P1 (const ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_immed:
	dprintf (DEBUG_FLOW, "#");
	if (is_icon (ap->u.offset)) {
	    dprintf (DEBUG_FLOW, "%ld", ap->u.offset->v.i);
	    break;
	}
	/*FALLTHRU */
    case am_direct:
	putconst (ap->u.offset);
	break;
    case am_areg:
	dprintf (DEBUG_FLOW, "a%d", (int) ap->preg - (int) A0);
	break;
    case am_dreg:
	dprintf (DEBUG_FLOW, "d%d", (int) ap->preg);
	break;
    case am_ind:
	dprintf (DEBUG_FLOW, "(a%d)", (int) ap->preg - (int) A0);
	break;
    case am_ainc:
	dprintf (DEBUG_FLOW, "(a%d)+", (int) ap->preg - (int) A0);
	break;
    case am_adec:
	dprintf (DEBUG_FLOW, "-(a%d)", (int) ap->preg - (int) A0);
	break;
    case am_indx:
	/* allow 32-bit offsets */
	putconst (ap->u.offset);
	dprintf (DEBUG_FLOW, "(a%d)", (int) ap->preg - (int) A0);
	break;
    case am_indx2:
	/* allow 32-bit offsets */
	putconst (ap->u.offset);
	dprintf (DEBUG_FLOW, "(a%d,d%d.%c)", (int) ap->preg - (int) A0,
		 (int) ap->sreg, 'l');
	break;
    case am_indx3:
	/* allow 32-bit offsets */
	putconst (ap->u.offset);
	dprintf (DEBUG_FLOW, "(a%d,a%d.l)", (int) ap->preg - (int) A0,
		 (int) ap->sreg - (int) A0);
	break;
    case am_indx4:
	/* allow 32-bit offsets */
	putconst (ap->u.offset);
	dprintf (DEBUG_FLOW, "(a%d,d%d.%c)", (int) ap->preg - (int) A0,
		 (int) ap->sreg, 'w');
	break;
    case am_indxpc:
	putconst (ap->u.offset);
	dprintf (DEBUG_FLOW, "(pc)");
	break;
    case am_indx2pc:
	putconst (ap->u.offset);
	dprintf (DEBUG_FLOW, "(a%d,pc)", (int) ap->preg - (int) A0);
	break;
    case am_rmask:
	put_rmask (ap->u.mask);
	break;
    case am_smask:
	put_smask (ap->u.mask);
	break;
    case am_freg:
	dprintf (DEBUG_FLOW, "fp%d", (int) ap->preg - (int) FP0);
	break;
    case am_line:
    case am_str:
	putconst (ap->u.offset);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}

/*
 *  To print out the regentry list into assembler-output
 *  Just to check if everything works as I thought
 */

static void print_regentry P1 (REGENTRY *, rp)
{
    int     seperator = ' ';

    for (; rp != NIL_REGENTRY; rp = rp->next) {
	dprintf (DEBUG_FLOW, "%c", seperator);
	putamode (rp->ap);
	seperator = ',';
    }
}


/*
 * Print a single regusage
 */

static void print_regusage P2 (REG, reg, REGENTRY **, regusage)
{
    if (regusage[reg] != NIL_REGENTRY) {
	putreg (reg);
	dprintf (DEBUG_FLOW, ": ");
	print_regentry (regusage[reg]);
	dprintf (DEBUG_FLOW, "\n");
    }
}

/*
 *  To print out the Regusages into assembler-output
 *  Just to check if everything works as I thought
 */

static void print_regusages P1 (REGENTRY **, regusage)
{
    REG     reg;

    for (reg = D0; reg <= MAX_REG; reg++) {
	print_regusage (reg, regusage);
    }
}
#endif /* DEBUG */

/*****************************************************************************/

/*
 * Create a REGENTRY and insert it before 'rp'.
 */
static REGENTRY *mk_regentry P2 (ADDRESS *, ap, REGENTRY *, rp)
{
    REGENTRY *new_rp;

    new_rp = (REGENTRY *) xalloc (sizeof (REGENTRY));
    new_rp->ap = ap;
    new_rp->next = rp;
    return new_rp;
}

/*
 * Searchs for the address 'ap' in the regentry 'rp' and
 * returns a pointer to it (or NIL_REGENTRY if not found)
 */

static REGENTRY *search_regentry P2 (REGENTRY *, rp, ADDRESS *, ap)
{
    for (; rp != NIL_REGENTRY; rp = rp->next) {
	if (is_equal_address (rp->ap, ap)) {
	    return rp;
	}
    }
    return NIL_REGENTRY;
}

/*
 * Adds an entry for 'ap' in the list 'rp' if it is not already in
 * the list.
 */

static REGENTRY *add_regentry P2 (REGENTRY *, rp, ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_ainc:
    case am_adec:
	return rp;
    default:
	/*
	 * check if equivalent entry is already in list
	 */
	if (search_regentry (rp, ap) == NIL_REGENTRY) {
	    rp = mk_regentry (ap, rp);
	}
	break;
    }
    return rp;
}

/*
 * adds the regentry list 'rp2' to the regentry list 'rp1' and
 * returns the combined list.
 */

static REGENTRY *add_regentries P2 (REGENTRY *, rp1, REGENTRY *, rp2)
{
    for (; rp2 != NIL_REGENTRY; rp2 = rp2->next) {
	rp1 = add_regentry (rp1, rp2->ap);
    }
    return rp1;
}

/*
 * Clears the ap-list of the register ap2->preg and adds the entry ap
 * if it is suitable (not if it refers to our own register)
 *
 * autoincrements are also not suitable as they never point
 * to the same value the next time
 */

static void new_regentry P3 (REGENTRY **, regusage, ADDRESS *, ap2, ADDRESS *,
			     ap)
{
    REG     reg;

    switch (ap2->mode) {
    case am_dreg:
    case am_areg:
    case am_freg:
	reg = ap2->preg;
	break;
    default:
	return;
    }

    switch (ap->mode) {
    case am_indx2:
	if (ap->sreg == reg) {
	    regusage[reg] = NIL_REGENTRY;
	    return;
	}
	/*FALLTHRU */
    case am_ind:
    case am_indx:
	if (ap->preg == reg) {
	    regusage[reg] = NIL_REGENTRY;
	    return;
	}
	break;
    case am_ainc:
    case am_adec:
	regusage[reg] = NIL_REGENTRY;
	return;
    default:
	break;
    }

    regusage[reg] = mk_regentry (ap, NIL_REGENTRY);
}

/* 
 * Generates a duplicate of the delivered regentry
 * and returns a pointer to it
 */
static REGENTRY *copy_regentry P1 (REGENTRY *, rp)
{
    REGENTRY *new_rp;

    for (new_rp = NIL_REGENTRY; rp != NIL_REGENTRY; rp = rp->next) {
	new_rp = mk_regentry (rp->ap, new_rp);
    }
    return new_rp;
}

/*
 * Only entries contained in both lists are delivered back in dest
 * (the biggest common set is returned in dest)
 *
 * Returns the amount of changes which have been done.
 * Is needed for the multipass, we are looping through the code
 * until no more changes are done
 */


static int merge_regentry P2 (REGENTRY **, dest, REGENTRY *, src)
{
    REGENTRY *last = NIL_REGENTRY;
    REGENTRY *p;
    int     changes = 0;

    for (p = *dest; p != NIL_REGENTRY; p = p->next) {
	if (search_regentry (src, p->ap) == NIL_REGENTRY) {
	    changes++;
	    if (last == NIL_REGENTRY) {
		*dest = p->next;
	    } else {
		last->next = p->next;
	    }
	}
	last = p;
    }
    return changes;
}

/*
 * removes all entries from dest-regentry which references
 * register reg in any way
 * REG_MEMORY is a pseudo-reg and means any memory-access
 */
static void remove_regentry P2 (REGENTRY **, dest, REG, reg)
{
    REGENTRY *last = NIL_REGENTRY;
    REGENTRY *p;

    if (reg == REG_MEMORY) {
	for (p = *dest; p != NIL_REGENTRY; p = p->next) {
	    if ((p->ap->mode == am_indx)
		|| (p->ap->mode == am_ind)
		|| (p->ap->mode == am_direct)
		|| (p->ap->mode == am_ainc)
		|| (p->ap->mode == am_adec)
		|| (p->ap->mode == am_indx2)) {
		if (last == NIL_REGENTRY) {
		    *dest = p->next;
		} else {
		    last->next = p->next;
		}
	    }
	    last = p;
	}
    } else {
	for (p = *dest; p != NIL_REGENTRY; p = p->next) {
	    if (is_register_used (reg, p->ap)) {
		if (last == NIL_REGENTRY) {
		    *dest = p->next;
		} else {
		    last->next = p->next;
		}
	    }
	    last = p;
	}
    }
}

/*****************************************************************************/

/*
 * clears the whole regusage-list
 */
static void clear_regusage P1 (REGENTRY **, regusage)
{
    REG     reg;

    for (reg = D0; reg <= MAX_REG; reg++) {
	regusage[reg] = NIL_REGENTRY;
    }
}

/*
 * Searches for any register already containing the value corresponding
 * to ap.  If a register is found it is returned, otherwise
 * NO_REG is returned
 */

static REG search_regusage P2 (REGENTRY **, regusage, ADDRESS *, ap)
{
    REG     reg;

    for (reg = D0; reg <= MAX_REG; reg++) {
	if (search_regentry (regusage[reg], ap) != NIL_REGENTRY) {
	    return reg;
	}
    }
    return NO_REG;
}

/*
 * Produces a copy of an registerusage-list and returns a pointer to it
 */

static REGENTRY **copy_regusage P1 (REGENTRY **, regusage)
{
    REGENTRY **new_regusage;
    REG     reg;

    new_regusage = (REGENTRY **) xalloc (sizeof (REGENTRY *[MAX_REG + 1]));
    for (reg = D0; reg <= MAX_REG; reg++) {
	new_regusage[reg] = copy_regentry (regusage[reg]);
    }
    return new_regusage;
}

/*
 * Merges the largest common registerusage-set from dst and src 
 * into src
 *
 * Returns the number of changes which have been done.
 * Is needed for the multipass, we are looping through the code
 * until no more changes are done
 */

static int merge_regusage P2 (REGENTRY **, dest, REGENTRY **, src)
{
    REG     reg;
    int     changes = 0;

    for (reg = D0; reg <= MAX_REG; reg++) {
	changes += merge_regentry (&(dest[reg]), src[reg]);
    }
    return changes;
}

/*
 * Removes all references to and from register reg
 */

static void remove_regusage P2 (REGENTRY **, regusage, REG, reg)
{
    REG     r;

    if (reg != REG_MEMORY) {
	regusage[reg] = NIL_REGENTRY;
    }
    for (r = D0; r <= MAX_REG; r++) {
	remove_regentry (&(regusage[r]), reg);
    }
}

/*
 * Removes all references to and from all temporary registers
 */

static void clear_tempreg_usage P1 (REGENTRY **, regusage)
{
    REG     reg;

    for (reg = D0; reg <= MAX_REG; reg++) {
	if (is_temporary_register (reg)) {
	    regusage[reg] = NIL_REGENTRY;
	    remove_regusage (regusage, reg);
	}
    }
}

/*
 * Attachs the Regusagelist to the Label which is the destination of
 * ip (ip must be jump or branch).
 * The usagelist of the label is corrected to the biggest common set
 * of regusage and the old list of the label.
 * 
 * Returns the number of changes which have been done.
 * Is needed for the multipass, we are looping through the code
 * until no more changes are done
 */

static int attach_regusage_to_label P2 (REGENTRY **, regusage, CODE *, ip)
{
    LABEL   label;
    CODE   *target;

    assert (ip->oper1->mode == am_direct);
    label = ip->oper1->u.offset->v.l;
    target = find_label (label);
    assert (target != NIL_CODE);
    if (target->oper2 == NIL_ADDRESS) {
	target->oper2 = (ADDRESS *) copy_regusage (regusage);
	return 1;
    } else {
	return merge_regusage ((REGENTRY **) target->oper2, regusage);
    }
}

/* 
 * Merges the Regusage list from the Label ip (ip must be a label)
 * and the Regusage list (result is the largest common set of both
 * lists.)
 *
 * Both lists are overwritten with the newly generated common list.
 *
 * If the label is referenced in an switch-table, an empty-list
 * is provided, since we dont know anything about register usage
 * after a switch. 
 *
 * Returns the number of changes which have been done.
 * Is needed for the multipass, we are looping through the code
 * until no more changes have been done
 */

static int merge_regusage_from_label P2 (REGENTRY **, regusage, CODE *, ip)
{
    LABEL   label;
    SWITCH *sw;
    BOOL    is_found = FALSE;

    assert (ip->opcode == op_label);
    label = ip->oper1->u.offset->v.l;

    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	LABEL   i;

	for (i = 0; i < sw->numlabs; i++) {
	    if (sw->labels[i] == label) {
		is_found = TRUE;;
	    }
	}
    }

    if (is_found) {
	clear_regusage (regusage);
	if (ip->oper2 == NIL_ADDRESS) {
	    ip->oper2 = (ADDRESS *) copy_regusage (regusage);
	    return 1;
	} else {
	    clear_regusage (regusage);
	    return merge_regusage ((REGENTRY **) ip->oper2, regusage);
	}
    } else {
	if (ip->oper2 == NIL_ADDRESS) {
	    ip->oper2 = (ADDRESS *) copy_regusage (regusage);
	    return 1;
	} else {
	    int     count;

	    count = merge_regusage ((REGENTRY **) ip->oper2, regusage);
	    VOIDCAST merge_regusage (regusage, (REGENTRY **) ip->oper2);

	    return count;
	}
    }
}

/* 
 * Copies the Regusage list from the label ip (ip must be a label)
 * to the Regusage list (old contents are overwritten).
 * If the label is referenced in an switch-table, an empty-list
 * is provided, since we dont know anything about register usage
 * after an switch. 
 */
static void copy_regusage_from_label P2 (REGENTRY **, regusage, CODE *, ip)
{
    LABEL   label;
    SWITCH *sw;
    BOOL    is_found = FALSE;

    assert (ip->opcode == op_label);
    label = ip->oper1->u.offset->v.l;

    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	LABEL   i;

	for (i = 0; i < sw->numlabs; i++) {
	    if (sw->labels[i] == label) {
		is_found = TRUE;
	    }
	}
    }

    if (is_found) {
	clear_regusage (regusage);
	if (ip->oper2 == NIL_ADDRESS) {
	    ip->oper2 = (ADDRESS *) copy_regusage (regusage);
	} else {
	    clear_regusage ((REGENTRY **) ip->oper2);
	}
    } else {
	if (ip->oper2 == NIL_ADDRESS) {
	    clear_regusage (regusage);
	} else {
	    REG     r;
	    REGENTRY **p;
	    REGENTRY **src = (REGENTRY **) ip->oper2;

	    for (r = D0, p = regusage; r < MAX_REG; r++, p++) {
		*p = copy_regentry (src[r]);
	    }
	}
    }
}


/*
 * Invalidates (throws out) all references in registers and
 * to registers, which are changed by the instruction ip, 
 * including memory references
 */

static void validate_regusage P2 (REGENTRY **, regusage, CODE *, ip)
{
    REGMAP *map;
    REGBITMAP changed;
    REGBITMAP current = 1;
    REG     reg;

    if (ip->opcode == op_label)
	return;
    map = build_regmap (ip);
    changed = map->write | map->modified | map->updated;
    for (reg = D0; reg <= MAX_REG; reg++) {
	if ((changed & current) != 0) {
	    remove_regusage (regusage, reg);
	}
	current <<= 1;
    }
    if ((changed & MEMBER (REG_MEMORY)) != 0) {
	remove_regusage (regusage, REG_MEMORY);
    }
}

/*****************************************************************************/

/* Is called if an amode could be exchanged with register reg;
 * that is if both contain the same value
 * 
 * We only do the replace if we think it is useful
 *
 * We replace following cases
 *
 *   Old               New
 *   Temporary D-Reg   D-Reg
 *   Temporary A-Reg   Not A-Reg  Reduces Pipelineconflicts
 *   Memory            Any Reg    Avoids Memory-access
 *                                frees busbandwith
 * 
 */

static BOOL is_exchangable P2 (ADDRESS **, apptr, REG, reg)
{
    ADDRESS *ap = *apptr;

    switch (ap->mode) {
    case am_dreg:
    case am_freg:
	if (is_temporary_data_register (ap->preg) && (is_data_register (reg))) {
	    /*
	     * replace only if temporary
	     */
	    *apptr = mk_reg (reg);
	    (*apptr)->mode = ap->mode;
	    return TRUE;
	}
	break;
    case am_areg:
	if (!is_address_register (reg)
	    || is_temporary_address_register (ap->preg)) {
	    *apptr = mk_reg (reg);
	    return TRUE;
	}
	break;
    default:
	*apptr = mk_reg (reg);
	return TRUE;
    }
    return FALSE;
}

/*
 *  Examines dataflow and checks if data is already in a register
 *  when read-accesses to external ram occurs
 *  needs multiple-passes to work
 */
int flow_dataflow P1 (CODE *, peep_head)
{
    REGENTRY *regusage[MAX_REG + 1];
    int     label_changes;
    BOOL    is_headless;
    REG     reg;
    register CODE *ip;
    int     replacecount = 0;
    static int totreplace = 0;

#ifdef DEBUG
    int     Round = 1;

#endif /* DEBUG */

    do {
	DPRINTF ((DEBUG_FLOW, "Dataflow, round %d...", Round++));
	label_changes = 0;
	is_headless = FALSE;
	clear_regusage (regusage);
	for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	    validate_regusage (regusage, ip);
	    switch (ip->opcode) {
	    case op_move:
	    case op_movea:
	    case op_moveq:
		new_regentry (regusage, ip->oper2, ip->oper1);
		switch (ip->oper2->mode) {
		case am_dreg:
		case am_areg:
		case am_freg:
		    reg = ip->oper2->preg;
		    regusage[reg] = add_regentry (regusage[reg], ip->oper1);
#ifdef DEBUG
		    DPRINTF ((DEBUG_FLOW, "ADD: "));
		    if (is_debugging (DEBUG_FLOW)) {
			print_regusage (reg, regusage);
		    }
#endif /* DEBUG */
		    switch (ip->oper1->mode) {
			REG     reg2;

		    case am_dreg:
		    case am_areg:
		    case am_freg:
			reg2 = ip->oper1->preg;
			regusage[reg2] =
			    add_regentries (regusage[reg], regusage[reg2]);
#ifdef DEBUG
			DPRINTF ((DEBUG_FLOW, "ADD: "));
			if (is_debugging (DEBUG_FLOW)) {
			    print_regusage (reg2, regusage);
			}
#endif /* DEBUG */
			break;
		    default:
			break;
		    }
		    break;
		default:
		    break;
		}
		switch (ip->oper1->mode) {
		case am_dreg:
		case am_areg:
		case am_freg:
		    reg = ip->oper1->preg;
		    regusage[reg] = add_regentry (regusage[reg], ip->oper2);
#ifdef DEBUG
		    DPRINTF ((DEBUG_FLOW, "ADD: "));
		    if (is_debugging (DEBUG_FLOW)) {
			print_regusage (reg, regusage);
		    }
#endif /* DEBUG */
		    break;
		default:
		    break;
		}
		break;
	    case op_blo:
	    case op_bls:
	    case op_bhi:
	    case op_bhs:
	    case op_beq:
	    case op_bne:
	    case op_blt:
	    case op_ble:
	    case op_bgt:
	    case op_bge:
		label_changes += attach_regusage_to_label (regusage, ip);
		break;

	    case op_label:
		if (is_headless) {
		    copy_regusage_from_label (regusage, ip);
		    is_headless = FALSE;
		} else {
		    label_changes += merge_regusage_from_label (regusage, ip);
		}
#ifdef DEBUG
		DPRINTF (
			 (DEBUG_FLOW, "LABEL: I_%d\n",
			  ip->oper1->u.offset->v.l));
		if (is_debugging (DEBUG_FLOW)) {
		    print_regusages (regusage);
		}
#endif /* DEBUG */
		break;

	    case op_bra:
		label_changes += attach_regusage_to_label (regusage, ip);
		/* FALLTHRU */

	    case op_rts:
	    case op_rte:
		clear_regusage (regusage);
		is_headless = TRUE;
		break;

#ifdef ASM
	    case op_asm:
		clear_regusage (regusage);
		break;
#endif /* ASM */

	    case op_jsr:
		clear_tempreg_usage (regusage);
		/*
		 * we do not know what called routine does
		 * with memory
		 */
		remove_regusage (regusage, REG_MEMORY);
		/* FALLTHRU */

	    default:
		break;

	    }
	}
	DPRINTF ((DEBUG_FLOW, " %d changes \n", label_changes));
    } while (label_changes > 0);

    DPRINTF ((DEBUG_FLOW, "%s\n", "Dataflow, REPLACEMENT ROUND"));
    label_changes = 0;
    is_headless = FALSE;
    clear_regusage (regusage);
    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	/*
	 * invalidate all entries with references to registers
	 * modified in this instruction
	 */
	validate_regusage (regusage, ip);

	switch (ip->opcode) {
	case op_label:
#ifdef DEBUG
	    DPRINTF ((DEBUG_FLOW, "LABEL: I_%d\n", ip->oper1->u.offset->v.l));
	    if (is_debugging (DEBUG_FLOW)) {
		print_regusages (regusage);
	    }
#endif /* DEBUG */
	    if (is_headless) {
#ifdef DEBUG
		DPRINTF ((DEBUG_FLOW, "Headless\n"));
		if (is_debugging (DEBUG_FLOW) && ip->oper2 != NIL_ADDRESS) {
		    print_regusages ((REGENTRY **) ip->oper2);
		}
#endif /* DEBUG */
		copy_regusage_from_label (regusage, ip);
		is_headless = FALSE;
	    } else {
		label_changes += merge_regusage_from_label (regusage, ip);
	    }
#ifdef DEBUG
	    DPRINTF ((DEBUG_FLOW, "-----------\n"));
	    if (is_debugging (DEBUG_FLOW)) {
		print_regusages (regusage);
	    }
#endif /* DEBUG */
	    break;

	case op_bra:
	    /* FALLTHRU */
	case op_rts:
	case op_rte:
	    clear_regusage (regusage);
	    is_headless = TRUE;
	    break;

#ifdef ASM
	case op_asm:
	    clear_regusage (regusage);
	    break;
#endif /* ASM */

	case op_jsr:
	    clear_tempreg_usage (regusage);
	    /*
	     * we do not know what the called routine does with memory
	     */
	    remove_regusage (regusage, REG_MEMORY);
	    regusage[A7] = NIL_REGENTRY;
	    /* FALLTHRU */

	default:
	    if (ip->oper1 != NIL_ADDRESS) {
		reg = search_regusage (regusage, ip->oper1);
		if (reg != NO_REG) {
		    DPRINTF (
			     (DEBUG_FLOW, "Found replacement for oper1 %d\n",
			      (int) reg));
		    if (is_exchangable (&(ip->oper1), reg)) {
			replacecount++;
			totreplace++;
		    }
		}
	    }
	    if ((ip->oper2 != NIL_ADDRESS) && (ip->oper2->mode == am_areg)) {
		regusage[ip->oper2->preg] = NIL_REGENTRY;
	    }
	    break;

	}
#ifdef DEBUG
	if (is_debugging (DEBUG_FLOW)) {
	    print_regusages (regusage);
	}
#endif /* DEBUG */
    }
    DPRINTF (
	     (DEBUG_FLOW, "Replaced %d Total %d\n", replacecount,
	      totreplace));
    return replacecount;
}
#endif /* MC680X0 */
#endif /* PEEPFLOW */
